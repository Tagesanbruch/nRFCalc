# Makefile for Calculator Project
# This manages building both the Zephyr application and the keypad simulator

# Variables
KEYPAD_SCRIPT = web/app.py
ZEPHYR_APP_DIR = build
ZEPHYR_APP_EXE = $(ZEPHYR_APP_DIR)/display/zephyr/zephyr.exe
VENV_DIR = .venv
WEB_DIR = web

# Find all source files for dependency checking
SRC_FILES := $(shell find src -name "*.c" -o -name "*.h")
CONFIG_FILES := prj.conf CMakeLists.txt $(shell find boards -name "*.conf" -o -name "*.overlay" 2>/dev/null)

# Default target
.PHONY: all
all: zephyr

# The keypad is a script, so it doesn't need a build step.
# We keep the target for consistency.
.PHONY: keypad_simulator
keypad_simulator:
	@echo "Python keypad simulator is a script. No build needed."

# Build the Zephyr application
.PHONY: zephyr
zephyr: $(ZEPHYR_APP_EXE)

$(ZEPHYR_APP_EXE): $(SRC_FILES) $(CONFIG_FILES)
	@echo "Building Zephyr application..."
	west build -b native_sim
	@echo "Zephyr application built successfully!"

# Force rebuild (useful when Make doesn't detect changes properly)
.PHONY: rebuild
rebuild:
	@echo "Force rebuilding Zephyr application..."
	west build -b native_sim
	@echo "Zephyr application rebuilt successfully!"

# Run both applications
.PHONY: run
run: zephyr
	@echo "Starting calculator system..."
	@echo "1. Starting Zephyr application in a new terminal..."
	gnome-terminal --working-directory=$(shell pwd) --title="Zephyr Calculator" -- ./$(ZEPHYR_APP_EXE) || \
	xterm -T "Zephyr Calculator" -e ./$(ZEPHYR_APP_EXE) || \
	konsole --workdir $(shell pwd) --title "Zephyr Calculator" -e ./$(ZEPHYR_APP_EXE) || \
	(echo "No suitable terminal emulator found. Starting Zephyr in background..." && ./$(ZEPHYR_APP_EXE) &)
	@sleep 3
	@echo "2. Starting CASIO fx-991ES PLUS Web Simulator..."
	@echo "   Web interface will open at: http://localhost:5000"
	@echo "   Press Ctrl+C to stop the web simulator"
	@if [ -f "$(VENV_DIR)/bin/activate" ]; then \
		echo "   Activating virtual environment..."; \
		cd $(WEB_DIR) && . ../$(VENV_DIR)/bin/activate && python app.py; \
	else \
		echo "   Virtual environment not found. Using system Python..."; \
		cd $(WEB_DIR) && python app.py; \
	fi

# Run both applications in background
.PHONY: run-bg
run-bg: zephyr
	@echo "Starting calculator system in background..."
	@echo "1. Starting Zephyr application in background..."
	./$(ZEPHYR_APP_EXE) &
	@echo "   Zephyr PID: $$!"
	@echo "2. Waiting for Zephyr to initialize..."
	@sleep 3
	@echo "3. Starting CASIO fx-991ES PLUS Web Simulator..."
	@echo "   Web interface will open at: http://localhost:5000"
	@echo "   Use 'make stop' to stop both applications"
	@if [ -f "$(VENV_DIR)/bin/activate" ]; then \
		echo "   Activating virtual environment..."; \
		cd $(WEB_DIR) && . ../$(VENV_DIR)/bin/activate && python app.py; \
	else \
		echo "   Virtual environment not found. Using system Python..."; \
		cd $(WEB_DIR) && python app.py; \
	fi

# Stop all running applications
.PHONY: stop
stop:
	@echo "Stopping calculator applications..."
	@echo "Killing Zephyr processes..."
	@pkill -f "zephyr.exe" || echo "No Zephyr processes found"
	@echo "Killing Python web server..."
	@pkill -f "app.py" || echo "No Python web server found"
	@echo "Cleaning up FIFO..."
	@rm -f /tmp/calculator_keypad_fifo
	@echo "All applications stopped."
.PHONY: run-zephyr
run-zephyr: zephyr
	@echo "Starting Zephyr application..."
	./$(ZEPHYR_APP_EXE)

# Run only the keypad simulator
.PHONY: run-keypad
run-keypad:
	@echo "Starting CASIO fx-991ES PLUS Web Simulator..."
	@if [ -f "$(VENV_DIR)/bin/activate" ]; then \
		echo "Activating virtual environment..."; \
		cd $(WEB_DIR) && . ../$(VENV_DIR)/bin/activate && python app.py; \
	else \
		echo "Virtual environment not found. Using system Python..."; \
		cd $(WEB_DIR) && python app.py; \
	fi

# Clean build artifacts
.PHONY: clean
clean: clean-zephyr
	@echo "Cleaning up FIFO..."
	rm -f /tmp/calculator_keypad_fifo
	@echo "Clean complete!"

# Clean only keypad simulator (no action needed for scripts)
.PHONY: clean-keypad
clean-keypad:
	@echo "No cleanup needed for Python script."

# Clean only Zephyr build
.PHONY: clean-zephyr
clean-zephyr:
	@echo "Cleaning Zephyr build..."
	rm -rf $(ZEPHYR_APP_DIR)

# Help
help:
	@echo "Calculator Project Makefile"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build the Zephyr application (keypad is a script)"
	@echo "  zephyr        - Build only the Zephyr application"
	@echo "  run           - Run both apps (Zephyr in new terminal, web in foreground)"
	@echo "  run-bg        - Run both apps in background mode"
	@echo "  run-zephyr    - Build and run only the Zephyr application"
	@echo "  run-keypad    - Run only the CASIO fx-991ES PLUS Web Simulator"
	@echo "  stop          - Stop all running calculator applications"
	@echo "  clean         - Clean all build artifacts and FIFO"
	@echo "  clean-zephyr  - Clean only the Zephyr build directory"
	@echo "  help          - Show this help message"
