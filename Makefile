# Makefile for Calculator Project
# This manages building both the Zephyr application and the keypad simulator

# Variables
KEYPAD_SCRIPT = keypad_web.py
ZEPHYR_APP_DIR = build
ZEPHYR_APP_EXE = $(ZEPHYR_APP_DIR)/zephyr/zephyr.exe
VENV_DIR = .venv

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

$(ZEPHYR_APP_EXE):
	@echo "Building Zephyr application..."
	west build -b native_sim
	@echo "Zephyr application built successfully!"

# Run both applications
.PHONY: run
run: zephyr
	@echo "Starting calculator system..."
	@echo "1. Starting Zephyr application in a new terminal..."
	gnome-terminal --working-directory=$(shell pwd) -- ./$(ZEPHYR_APP_EXE)
	@sleep 2
	@echo "2. Starting Python keypad simulator..."
	@echo "   Please ensure you have activated the virtual environment: source $(VENV_DIR)/bin/activate"
	python $(KEYPAD_SCRIPT)

# Run only the Zephyr application
.PHONY: run-zephyr
run-zephyr: zephyr
	@echo "Starting Zephyr application..."
	./$(ZEPHYR_APP_EXE)

# Run only the keypad simulator
.PHONY: run-keypad
run-keypad:
	@echo "Starting Python keypad simulator..."
	@echo "Please activate the virtual environment first if you haven't:"
	@echo "source $(VENV_DIR)/bin/activate"
	python $(KEYPAD_SCRIPT)

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
	@echo "  run           - Run both the Zephyr app and the Python keypad simulator"
	@echo "  run-zephyr    - Build and run only the Zephyr application"
	@echo "  run-keypad    - Run only the Python keypad simulator"
	@echo "  clean         - Clean all build artifacts and FIFO"
	@echo "  clean-zephyr  - Clean only the Zephyr build directory"
	@echo "  help          - Show this help message"
