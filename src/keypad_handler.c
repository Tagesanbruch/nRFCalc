/*
 * Keypad Handler Implementation
 * 
 * This module provides unified key input handling, supporting both FIFO simulation
 * (for native_sim) and hardware GPIO inputs (for real hardware).
 */

#include "keypad_handler.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_ARCH_POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

LOG_MODULE_REGISTER(keypad_handler, LOG_LEVEL_INF);

// Message queue for key events
K_MSGQ_DEFINE(key_msgq, sizeof(key_code_t), 16, 4);

#ifdef CONFIG_ARCH_POSIX
// FIFO-based input for native_sim
#define FIFO_PATH "/tmp/calculator_keypad_fifo"
static int fifo_fd = -1;

// FIFO reader thread
static void fifo_reader_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    LOG_INF("FIFO reader thread started");
    
    while (1) {
        if (fifo_fd >= 0) {
            key_code_t key;
            ssize_t bytes_read = read(fifo_fd, &key, sizeof(key));
            
            if (bytes_read == sizeof(key)) {
                LOG_INF("Key received from simulator: %d", key);
                k_msgq_put(&key_msgq, &key, K_NO_WAIT);
            } else if (bytes_read < 0) {
                // FIFO might be closed, try to reopen
                close(fifo_fd);
                fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
            }
        } else {
            // Try to open FIFO
            fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
        }
        
        // Small delay to avoid busy waiting
        k_msleep(10);
    }
}

// Thread definition
K_THREAD_DEFINE(fifo_reader, 1024, fifo_reader_thread, NULL, NULL, NULL,
                K_PRIO_COOP(10), 0, 0);

static int init_fifo_keypad(void)
{
    LOG_INF("Initializing FIFO-based keypad handler");
    
    // Create FIFO if it doesn't exist
    mkfifo(FIFO_PATH, 0666);
    
    // Try to open FIFO (non-blocking)
    fifo_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifo_fd < 0) {
        LOG_WRN("Could not open FIFO immediately. Will retry in background thread.");
    }
    
    LOG_INF("FIFO keypad initialized. Waiting for simulator at: %s", FIFO_PATH);
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
    return init_fifo_keypad();
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
