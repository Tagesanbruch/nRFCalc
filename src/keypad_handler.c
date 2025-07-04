/*
 * Keypad Handler Implementation
 * 
 * This module provides unified key input handling, supporting both SDL simulation
 * (for native_sim) and hardware GPIO inputs (for real hardware).
 */

#include "keypad_handler.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_ARCH_POSIX
#include <SDL2/SDL.h>
#endif

LOG_MODULE_REGISTER(keypad_handler, LOG_LEVEL_INF);

// Message queue for key events
K_MSGQ_DEFINE(key_msgq, sizeof(key_code_t), 16, 4);

#ifdef CONFIG_ARCH_POSIX
// SDL simulation variables
static SDL_Window *keypad_window = NULL;
static SDL_Renderer *keypad_renderer = NULL;
static bool sdl_initialized = false;

// Key layout for virtual keypad
struct key_button {
    int x, y, w, h;
    key_code_t key;
    const char *label;
    uint32_t color;
};

// Simple calculator keypad layout (4x5 grid)
static struct key_button keypad_buttons[] = {
    // Row 1
    {10, 10, 60, 40, KEY_CLEAR, "C", 0xFF6666},
    {80, 10, 60, 40, KEY_BACKSPACE, "Del", 0xFF6666},
    {150, 10, 60, 40, KEY_DIVIDE, "/", 0xFFAA66},
    {220, 10, 60, 40, KEY_MULTIPLY, "*", 0xFFAA66},
    
    // Row 2  
    {10, 60, 60, 40, KEY_7, "7", 0xCCCCCC},
    {80, 60, 60, 40, KEY_8, "8", 0xCCCCCC},
    {150, 60, 60, 40, KEY_9, "9", 0xCCCCCC},
    {220, 60, 60, 40, KEY_MINUS, "-", 0xFFAA66},
    
    // Row 3
    {10, 110, 60, 40, KEY_4, "4", 0xCCCCCC},
    {80, 110, 60, 40, KEY_5, "5", 0xCCCCCC},
    {150, 110, 60, 40, KEY_6, "6", 0xCCCCCC},
    {220, 110, 60, 40, KEY_PLUS, "+", 0xFFAA66},
    
    // Row 4
    {10, 160, 60, 40, KEY_1, "1", 0xCCCCCC},
    {80, 160, 60, 40, KEY_2, "2", 0xCCCCCC},
    {150, 160, 60, 40, KEY_3, "3", 0xCCCCCC},
    {220, 160, 60, 90, KEY_EQUAL, "=", 0x66FF66}, // Taller button
    
    // Row 5
    {10, 210, 130, 40, KEY_0, "0", 0xCCCCCC}, // Wider button
    {150, 210, 60, 40, KEY_DOT, ".", 0xCCCCCC},
};

#define NUM_BUTTONS (sizeof(keypad_buttons) / sizeof(keypad_buttons[0]))

// SDL thread function
static void keypad_sdl_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    LOG_INF("SDL keypad thread started");
    
    SDL_Event event;
    bool running = true;
    bool need_render = true;
    
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int mouse_x = event.button.x;
                    int mouse_y = event.button.y;
                    
                    // Check which button was clicked
                    for (int i = 0; i < NUM_BUTTONS; i++) {
                        struct key_button *btn = &keypad_buttons[i];
                        if (mouse_x >= btn->x && mouse_x < btn->x + btn->w &&
                            mouse_y >= btn->y && mouse_y < btn->y + btn->h) {
                            
                            LOG_INF("Key pressed: %s", btn->label);
                            
                            // Send key message to main thread
                            k_msgq_put(&key_msgq, &btn->key, K_NO_WAIT);
                            break;
                        }
                    }
                }
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_EXPOSED ||
                    event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    need_render = true;
                }
                break;
            }
        }
        
        // Only render when needed and when SDL is fully initialized
        if (need_render && sdl_initialized) {
            // Clear background
            SDL_SetRenderDrawColor(keypad_renderer, 40, 40, 40, 255);
            SDL_RenderClear(keypad_renderer);
            
            // Draw all buttons
            for (int i = 0; i < NUM_BUTTONS; i++) {
                struct key_button *btn = &keypad_buttons[i];
                
                // Draw button background
                SDL_Rect rect = {btn->x, btn->y, btn->w, btn->h};
                SDL_SetRenderDrawColor(keypad_renderer, 
                    (btn->color >> 16) & 0xFF,
                    (btn->color >> 8) & 0xFF,
                    btn->color & 0xFF, 255);
                SDL_RenderFillRect(keypad_renderer, &rect);
                
                // Draw button border
                SDL_SetRenderDrawColor(keypad_renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(keypad_renderer, &rect);
                
                // Draw button text (simplified - you can improve this later)
                // For now, we'll just draw a simple text representation
            }
            
            SDL_RenderPresent(keypad_renderer);
            need_render = false;
        }
        
        // Reduce CPU usage - only check events every 33ms (~30 FPS)
        k_msleep(33);
    }
    
    LOG_INF("SDL keypad thread ended");
}

// Define the SDL thread
K_THREAD_DEFINE(sdl_thread, 2048, keypad_sdl_thread, NULL, NULL, NULL, 
                K_PRIO_COOP(7), 0, 0);

static int init_sdl_keypad(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERR("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return -EIO;
    }
    
    keypad_window = SDL_CreateWindow("Calculator Keypad",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        290, 260, SDL_WINDOW_SHOWN);
        
    if (!keypad_window) {
        LOG_ERR("Window could not be created! SDL_Error: %s", SDL_GetError());
        SDL_Quit();
        return -EIO;
    }
    
    keypad_renderer = SDL_CreateRenderer(keypad_window, -1, SDL_RENDERER_ACCELERATED);
    if (!keypad_renderer) {
        LOG_ERR("Renderer could not be created! SDL_Error: %s", SDL_GetError());
        SDL_DestroyWindow(keypad_window);
        SDL_Quit();
        return -EIO;
    }
    
    sdl_initialized = true;
    LOG_INF("SDL keypad initialized successfully");
    return 0;
}

#else // Hardware GPIO implementation

static int init_gpio_keypad(void)
{
    // TODO: Implement hardware GPIO keypad initialization
    // This would involve:
    // 1. Getting GPIO devices from device tree
    // 2. Configuring pins as inputs with pull-ups
    // 3. Setting up interrupts or scanning matrix
    // 4. Registering callbacks that send messages to key_msgq
    
    LOG_INF("GPIO keypad initialized (placeholder)");
    return 0;
}

#endif // CONFIG_ARCH_POSIX

int keypad_init(void)
{
    LOG_INF("Initializing keypad handler");
    
#ifdef CONFIG_ARCH_POSIX
    return init_sdl_keypad();
#else
    return init_gpio_keypad();
#endif
}

key_code_t keypad_get_key(void)
{
    key_code_t key;
    
    if (k_msgq_get(&key_msgq, &key, K_NO_WAIT) == 0) {
        return key;
    }
    
    return KEY_NONE;
}

key_code_t keypad_wait_key(int timeout_ms)
{
    key_code_t key;
    k_timeout_t timeout = (timeout_ms == 0) ? K_FOREVER : K_MSEC(timeout_ms);
    
    if (k_msgq_get(&key_msgq, &key, timeout) == 0) {
        return key;
    }
    
    return KEY_NONE;
}
