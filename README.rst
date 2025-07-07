.. zephyr:code-sample:: display
   :name: CASIO fx-991ES PLUS Calculator Simulator
   :relevant-api: display_interface

   A complete CASIO fx-991ES PLUS scientific calculator implementation with web interface.

Overview
********

This project implements a complete CASIO fx-991ES PLUS scientific calculator using Zephyr RTOS
for the calculation engine and a modern web interface for user interaction. The system consists
of two main components:

1. **Zephyr Calculator Application**: Handles mathematical computations and display rendering
2. **Web Interface**: Provides an authentic CASIO fx-991ES PLUS user interface

The calculator supports advanced mathematical functions including trigonometry, logarithms,
matrix operations, vector calculations, and complex number arithmetic, just like the real
CASIO fx-991ES PLUS calculator.

Project Structure
*****************

- ``src/``: Zephyr calculator application source code
- ``web/``: Web interface with Flask backend and responsive frontend  
- ``misc/``: Legacy implementations and development utilities
- ``boards/``: Board-specific configurations
- ``build/``: Zephyr build output

Communication
*************

The web interface communicates with the Zephyr application through a FIFO (named pipe),
enabling real-time key press forwarding and display updates.

Building and Running
********************

**Prerequisites**:

.. code-block:: bash

   # Install Python dependencies
   python -m venv .venv
   source .venv/bin/activate
   pip install flask

**Build and Run**:

**Build and Run**:

.. code-block:: bash

   # Build and run both components
   make run
   
   # Or run components separately:
   make run-zephyr    # Zephyr calculator only
   make run-keypad    # Web interface only

**Web Interface**:

Open http://localhost:5000 in your browser to access the CASIO fx-991ES PLUS simulator.

Features
********

- Complete CASIO fx-991ES PLUS button layout
- SHIFT/ALPHA mode support with visual indicators  
- Scientific functions (sin, cos, tan, log, ln, sqrt, etc.)
- Matrix and vector operations
- Complex number calculations
- Keyboard shortcuts support
- Real-time communication with Zephyr backend

**Supported Boards**:

This calculator works on any Zephyr-supported board. For development and testing,
the ``native_sim`` board is recommended:

**Supported Boards**:

This calculator works on any Zephyr-supported board. For development and testing,
the ``native_sim`` board is recommended:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display  
   :board: native_sim
   :goals: build
   :compact:

For testing purpose without the need of any hardware, the :ref:`native_sim <native_sim>`
board is also supported and can be built as follows;

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: native_sim
   :goals: build
   :compact:

List of Arduino-based display shields
*************************************

- :ref:`adafruit_2_8_tft_touch_v2`
- :ref:`ssd1306_128_shield`
- :ref:`st7789v_generic`
- :ref:`waveshare_epaper`
