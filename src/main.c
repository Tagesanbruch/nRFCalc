/*
 * Scientific Calculator Application
 * 
 * A modular calculator implementation using Zephyr RTOS with:
 * - Abstract display engine for various pixel formats
 * - Unified keypad handler supporting SDL simulation and hardware GPIO
 * - Mathematical expression evaluator with basic operations
 *
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Modified for calculator application
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(calculator_main, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include "display_engine.h"
#include "keypad_handler.h" 
#include "calculator_core.h"

#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

#ifdef CONFIG_ARCH_POSIX
static void posix_exit_main(int exit_code)
{
#if CONFIG_TEST
	if (exit_code == 0) {
		LOG_INF("PROJECT EXECUTION SUCCESSFUL");
	} else {
		LOG_INF("PROJECT EXECUTION FAILED");
	}
#endif
	posix_exit(exit_code);
}
#endif

int main(void)
{
	struct calculator calc;

	LOG_INF("Starting Scientific Calculator application");

	if (display_engine_init() != 0) {
		LOG_ERR("Failed to initialize display engine. Aborting.");
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 1;
#endif
	}

	if (keypad_init() != 0) {
		LOG_ERR("Failed to initialize keypad handler. Aborting.");
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 1;
#endif
	}

	calculator_init(&calc);

	while (1) {
		// 1. Get input
		key_code_t key = keypad_get_key();

		// 2. Update state and data (process key press)
		calculator_update_state(&calc, key);

		// 3. Render UI
		calculator_render_ui(&calc);

		// 4. Yield to other threads - reduced for better responsiveness
		k_msleep(10);
	}

#ifdef CONFIG_ARCH_POSIX
	posix_exit_main(0);
#endif
	return 0;
}
