# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses **PlatformIO** with three distinct build environments:

- `esp32-c3-devkitm-1`: Default environment with MAC-based auto role detection
- `master`: Forces master device role (`-DFORCE_MASTER_ROLE=1`)
- `slave`: Forces slave device role (`-DFORCE_SLAVE_ROLE=1`)

### Essential Build Commands

```bash
# Build specific environment
pio run -e master
pio run -e slave
pio run  # default environment

# Upload firmware
pio run -e master --target upload
pio run -e slave --target upload

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean

# Run tests
pio test --port COM3
```

### Convenient Build Scripts

- `build_both.bat`: Interactive menu for building master/slave/both/default
- `upload.bat`: Interactive firmware upload with monitoring
- `build.bat`: Simple single-environment build

## Architecture Overview

### Core System Design

The project implements a **dual-device wireless training system** using ESP32-C3 microcontrollers with ESP-NOW communication protocol.

**Key Architectural Patterns:**
- **State Machine**: System behavior controlled via `SystemState` enum (INIT → MENU → READY → TIMING → COMPLETE)
- **Hardware Abstraction**: Clean separation between hardware drivers and application logic
- **Component Managers**: Modular design with dedicated managers (ButtonManager, TimeManager, VibrationTraining)

### System States and Flow

```
STATE_INIT → STATE_MENU → STATE_READY → STATE_TIMING → STATE_COMPLETE
     ↑                        ↑             ↓              ↓
     └────────── STATE_ERROR ←┴─────────────┴──────────────┘
```

### Communication Architecture

**ESP-NOW Protocol Implementation:**
- Master-slave role determination via MAC address or compile flags
- Message structure defined in `message_t` with command types (CMD_START_TASK, CMD_TASK_COMPLETE, etc.)
- Peer-to-peer communication with <10ms latency

### Hardware Abstraction Layer

**Pin Configuration (config.h):**
- Vibration Sensor: GPIO2
- Button: GPIO5 (high-level trigger)
- WS2812B LEDs: GPIO1 (12 LEDs)
- Buzzer: GPIO4
- OLED I2C: SDA-GPIO8, SCL-GPIO9

## Testing Framework

Uses **Unity** testing framework with hardware-in-the-loop testing approach:

- `test/test_button.cpp`: Button hardware and debouncing
- `test/test_menu.cpp`: Menu system integration
- `test_communication.cpp`: ESP-NOW protocol validation
- `test_vibration_communication.cpp`: End-to-end training workflow

## Key Development Notes

### Role Configuration

Device roles can be set via:
1. **Compile-time flags**: Use specific build environments (master/slave)
2. **MAC address detection**: Default environment with predefined MAC addresses in `config.h`

### State Management

The main application loop centers around `updateSystem()` which handles state transitions. Key states:
- `STATE_MENU`: Menu navigation and selection
- `STATE_READY`: Waiting for training start
- `STATE_TIMING`: Active training/timing
- `STATE_COMPLETE`: Training finished, showing results

### Button Event Handling

Three button events managed by ButtonManager:
- **Single Click**: Menu navigation, start training, continue
- **Double Click**: Menu previous item (only in menu state)
- **Long Press**: Menu confirm, return to menu

### Hardware Dependencies

Critical libraries:
- `U8g2`: OLED display with Chinese font support
- `FastLED`: WS2812B LED control
- `ArduinoJson`: Communication message parsing
- `OneButton`: Advanced button state management

### Communication Protocol

Message structure includes command type, target/source IDs, timestamp, data payload, and checksum. Handle both sending (`onDataSent`) and receiving (`onDataReceived`) callbacks for robust communication.

### Debugging

Monitor serial output at 115200 baud. The system provides extensive debug logging for state transitions, button events, and communication status.