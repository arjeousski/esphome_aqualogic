# ESPHome AquaLogic / ProLogic Pool Controller Component

This custom ESPHome component interfaces with **Hayward/Goldline AquaLogic and ProLogic** pool automation systems. It communicates with the pool controller's main board over an RS-485 bus using UART, allowing you to monitor sensors, mirror the LCD display, emulate panel keys, and automate pool functions (like heating schedules) directly from ESPHome and Home Assistant.

> [!IMPORTANT]
> This project is not affiliated with or endorsed by Hayward Industries Inc. in any way.

## Features

- **Sensors**: Monitor pool temperature, spa temperature, air temperature, pump speed, pump power, salt level, and chlorinator percentages.
- **Binary Sensors**: Track real-time status of the filter pump (solid/High speed), filter pump low speed (blinking/Low speed), lights, heater (actively heating), heater waiting/idle mode (blinking), waterfall (valve 3), system warning alarms (Check System - solid), and flashing warnings (Check System - blinking).
- **Text Sensors**: Mirror display line 1, display line 2, and active controller flags. Characters that are set to blink on the physical LCD display are automatically enclosed in square brackets (e.g. `[Check System]`) in the text sensors.
- **Controls (Buttons)**: Emulate physical key presses for panel navigation (`Menu`, `Left`, `Right`, `Plus`, `Minus`) and toggle pool functions (`Filter`, `Lights`, `Heater (auto)`, `Waterfall`).
- **Heater Scheduler**: Configurable allowed run-windows with automatic timezone synchronization.
- **Dual-Framework Compatibility**: Fully compatible with both `arduino` and `esp-idf` compilation frameworks under ESPHome.

---

## Hardware Requirements

To connect your ESP32/ESP8266 to the AquaLogic controller, you need:

1. **ESP32 Development Board** (e.g., NodeMCU-32S). An ESP32 is recommended due to hardware UART capabilities and loop performance.
2. **RS-485 to TTL Module**: Must support **3.3V TTL logic levels** (e.g., SP3485, MAX3485, or a 3.3V-compatible module). Standard 5V-only modules (like MAX485) should not be connected directly to the ESP32 GPIOs without a logic level shifter, as the ESP32 runs on 3.3V logic.
3. **RS-485 Cable Connection**: Converted logic lines connected to the AquaLogic main board keypad terminal.

### Wiring Diagram

| AquaLogic Board (Keypad Terminal) | RS-485 Module | ESP32 Pin |
| :--- | :--- | :--- |
| **GND** (Pin 1) | GND | GND |
| **RX/TX -** (Pin 2) | B (B-) | - |
| **RX/TX +** (Pin 3) | A (A+) | - |
| **+10V** (Pin 4) | - (DO NOT CONNECT TO ESP32) | - |
| - | RXD | RX Pin (e.g. GPIO26) |
| - | TXD | TX Pin (e.g. GPIO27) |
| - | VCC | 3.3V or 5V (depending on module) |

> [!CAUTION]
> Pin 4 of the keypad terminal on the AquaLogic board outputs ~10VDC. **Do not connect this pin directly to your ESP32 or RS-485 module logic pins**, or you will damage the board. Power the ESP32 separately (e.g., using a step-down buck converter connected to the 10V line, or via USB).

---

## Installation

To use this component in your ESPHome configuration, you can reference it either directly from the GitHub repository (ideal for production) or as a local component (great for development).

### 1. Remote installation (via Git)
Add the following to your ESPHome configuration file:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/arjeousski/esphome_aqualogic.git
      # Optional: specify a branch, tag, or commit ref
      # ref: main
    components: [ aqualogic ]
```

### 2. Local installation
Copy the `components/aqualogic` folder into the `external_components` directory of your ESPHome project, then reference it locally:

```yaml
external_components:
  - source: external_components
```

---

## Configuration Example

Below is a typical configuration snippet for [poolcontroller.yaml](poolcontroller.yaml):

> [!TIP]
> **UART Optimization Settings (`rx_full_threshold` & `rx_timeout`)**:
> On ESP32 controllers, configuring `rx_full_threshold: 8` and `rx_timeout: 1` under the `uart` component triggers hardware receive interrupts for incoming data immediately. This ensures the ESP32 processes keep-alive frames instantly, which is critical for sending key event commands back to the pool controller within the narrow response window.

```yaml
# Configure the esp32 framework (supports both arduino and esp-idf)
esp32:
  board: nodemcu-32s
  framework:
    type: arduino  # Or switch to: esp-idf

# Configure UART interface (AquaLogic runs at 19200 baud, 8N2)
uart:
  tx_pin: 27
  rx_pin: 26
  baud_rate: 19200
  stop_bits: 2
  # ESP32 FIFO optimizations for immediate frame retrieval
  rx_full_threshold: 8
  rx_timeout: 1

# Initialize AquaLogic base component
aqualogic:

# Expose sensors
sensor:
  - platform: aqualogic
    air_temp:
      name: "Air Temperature"
    pool_temp:    
      name: "Pool Temperature"
    pump_speed:
      name: "Pump Speed"
    pump_power:
      name: "Pump Power"
    salt_level:
      name: "Salt Level"
    pool_chlorine_factor:
      name: "Pool Chlorine Factor"
    spa_chlorine_factor:
      name: "Spa Chlorine Factor"

# Expose LCD display lines and flags
text_sensor:
  - platform: aqualogic
    display1:
      name: "Display Line 1"
    display2:
      name: "Display Line 2"
    flags_status:
      name: "Flags"

# Expose status binary sensors
binary_sensor:
  - platform: aqualogic
    lights:
      name: "Lights Status"
    filter:
      name: "Filter Status"
    filter_low_speed:
      name: "Filter Low Speed Status"
    heater_auto:
      id: heater_auto
      name: "Heater Auto Status"
    heater_1:
      name: "Heater Status"
    heater_blinking:
      name: "Heater Waiting Status"
    valve_3:
      name: "Waterfall Status"
    check_system:
      name: "Check System Warning"
      device_class: problem
    check_system_blinking:
      name: "Check System Warning Flashing"
      device_class: problem
    pool:
      name: "Pool Mode Active"
    spa:
      name: "Spa Mode Active"
    aux_1:
      name: "Aux 1 Cleaner Status"

# Emulate buttons/controls
button:
  - platform: template
    name: "Heater 1 (auto)"
    id: heater_auto_button
    on_press:
      - aqualogic.toggle:
          key: HEATER_1

  - platform: template
    name: "Lights"
    on_press:
      - aqualogic.toggle:
          key: LIGHTS
          type: LOCAL_WIRED # Optional: LOCAL_WIRED, REMOTE_WIRED, WIRELESS, WIRELESS2 (default)

  - platform: template
    name: "Filter"
    icon: "mdi:water-pump"
    on_press:
      - aqualogic.toggle:
          key: FILTER

  - platform: template
    name: "Waterfall"
    icon: "mdi:waterfall"
    on_press:
      - aqualogic.toggle:
          key: VALVE_3

  # Keyboard Emulation for menu navigation
  - platform: template
    name: "Key: Left"
    icon: "mdi:arrow-left"
    on_press:
      - aqualogic.send:
          key: LEFT

  - platform: template
    name: "Key: Right"
    icon: "mdi:arrow-right"
    on_press:
      - aqualogic.send:
          key: RIGHT

  - platform: template
    name: "Key: Plus"
    icon: "mdi:plus"
    on_press:
      - aqualogic.send:
          key: PLUS

  - platform: template
    name: "Key: Minus"
    icon: "mdi:minus"
    on_press:
      - aqualogic.send:
          key: MINUS

  - platform: template
    name: "Key: Menu"
    icon: "mdi:menu"
    on_press:
      - aqualogic.send:
          key: MENU
```

---

## Reference Tables: Keys, States, and Mappings

To configure key emulation and status monitoring, use the following tables as a reference for available keys, state flags, and confirmation rules.

### 1. Available Keys
These keys can be passed as the `key` argument to either `aqualogic.send` or `aqualogic.toggle` actions:

| Key String | Internal Constant | Supported Frame Type(s) | Action Support | Description |
| :--- | :--- | :--- | :--- | :--- |
| `MENU` | `KEY_MENU` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send Only | Menu button navigation |
| `LEFT` | `KEY_LEFT` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send Only | Left arrow navigation |
| `RIGHT` | `KEY_RIGHT` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send Only | Right arrow navigation |
| `PLUS` | `KEY_PLUS` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send Only | Plus (`+`) button navigation |
| `MINUS` | `KEY_MINUS` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send Only | Minus (`-`) button navigation |
| `FILTER` | `KEY_FILTER` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send & Toggle | Filter pump button |
| `LIGHTS` | `KEY_LIGHTS` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send & Toggle | Lights button |
| `POOL_SPA` | `KEY_POOL_SPA` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send Only | Pool / Spa mode button |
| `VALVE_3` | `KEY_VALVE_3` | `WIRELESS`, `WIRELESS2` (Default) | Send & Toggle | Valve 3 (Waterfall) button |
| `VALVE_4` | `KEY_VALVE_4` | `WIRELESS`, `WIRELESS2` (Default) | Send & Toggle | Valve 4 button |
| `HEATER_1` | `KEY_HEATER_1` | `WIRELESS`, `WIRELESS2` (Default) | Send & Toggle | Heater auto button |
| `SERVICE` | `KEY_SERVICE` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send Only | Service button (on panel) |
| `AUX_1` - `AUX_7` | `KEY_AUX_1` - `KEY_AUX_7` | `LOCAL_WIRED`, `REMOTE_WIRED`, `WIRELESS`, `WIRELESS2` (Default) | Send & Toggle | Aux 1 to Aux 7 buttons |
| `AUX_8` - `AUX_14` | `KEY_AUX_8` - `KEY_AUX_14` | `WIRELESS`, `WIRELESS2` (Default) | Send & Toggle | Aux 8 to Aux 14 buttons |

### 2. Available States (Flags)
These boolean flags are broadcasted by the controller. Any of these flags can be:
- Exposed as a dedicated, individual **Binary Sensor** entity in your ESPHome configuration.
- Inspected as part of the aggregated, comma-separated list in the `flags_status` text sensor (e.g. `POOL,FILTER,LIGHTS`).
- Queried programmatically inside custom lambdas/scripts using the C++ helper: `GetFlag(FLAG_CONSTANT)`.

| Flag Constant | YAML Config Key | Exposure Type | Description |
| :--- | :--- | :--- | :--- |
| `FILTER` | `filter` | Binary Sensor | Filter pump status - High Speed (active/idle) |
| `FILTER_LOW_SPEED` | `filter_low_speed` | Binary Sensor | Filter pump status - Low Speed (active/idle) |
| `LIGHTS` | `lights` | Binary Sensor | Lights status (active/idle) |
| `HEATER_AUTO` | `heater_auto` | Binary Sensor | Heater control auto mode state (active/idle) |
| `HEATER_1` | `heater_1` | Binary Sensor | Heater status (active/idle) |
| `HEATER_BLINKING` | `heater_blinking` | Binary Sensor | Heater waiting/idle blinking status |
| `VALVE_3` | `valve_3` | Binary Sensor | Valve 3 status (open/closed) |
| `VALVE_4` | `valve_4` | Binary Sensor | Valve 4 status (open/closed) |
| `CHECK_SYSTEM` | `check_system` | Binary Sensor | System check status warning LED (active/clear) |
| `CHECK_SYSTEM_BLINKING` | `check_system_blinking` | Binary Sensor | System check flashing warning LED (active/clear) |
| `POOL` | `pool` | Binary Sensor | Pool mode state active |
| `SPA` | `spa` | Binary Sensor | Spa mode state active |
| `SERVICE` | `service` | Binary Sensor | Service mode state active |
| `SPILLOVER` | `spillover` | Binary Sensor | Spillover mode state active |
| `SYSTEM_OFF` | `system_off` | Binary Sensor | System power off state |
| `SUPER_CHLORINATE` | `super_chlorinate` | Binary Sensor | Super chlorination mode state |
| `IS_METRIC` | `is_metric` | Binary Sensor | Metric temperature unit state |
| `AUX_1` - `AUX_14` | `aux_1` - `aux_14` | Binary Sensor | Aux channel 1 to 14 active flags |

### 3. Toggle Mappings for Confirmation (`aqualogic.toggle`)
For keys that support state confirmation, the retry mechanism monitors the corresponding status flag. If a key is sent using `aqualogic.toggle` but is not in this table, it will fall back to a standard fire-and-forget command (equivalent to `aqualogic.send`).

| Key Command | Mapped Status Flag | Confirmation Behavior |
| :--- | :--- | :--- |
| `FILTER` | `FILTER` | Verifies filter pump status changed |
| `LIGHTS` | `LIGHTS` | Verifies lights status changed |
| `VALVE_3` | `VALVE_3` | Verifies Valve 3 / Waterfall status changed |
| `VALVE_4` | `VALVE_4` | Verifies Valve 4 status changed |
| `HEATER_1` | `HEATER_AUTO` | Verifies Heater Auto state changed |
| `AUX_1` - `AUX_14` | `AUX_1` - `AUX_14` | Verifies corresponding Aux status changed |

---

## How It Works

### Key Emulation: Send vs Toggle Actions

The component exposes two distinct actions for transmitting panel commands over RS-485:

#### 1. `aqualogic.send` (Fire-and-Forget)
- **Use Case**: Navigation keys (`LEFT`, `RIGHT`, `PLUS`, `MINUS`, `MENU`).
- **How it works**: Transmits the key event frame over the RS-485 line exactly once. Because navigation keys do not correspond to a single binary state flag on the panel (e.g., pressing `MENU` doesn't switch on a physical status LED), the component does not listen for confirmation or attempt auto-retries.

#### 2. `aqualogic.toggle` (Confirmed with Auto-Retries)
- **Use Case**: Equipment toggles (`FILTER`, `LIGHTS`, `HEATER_1`, `VALVE_3`, `VALVE_4`).
- **How it works**:
  1. Records the **initial state** of the corresponding feature flag.
  2. Transmits the key event frame.
  3. Listens to the incoming status broadcasts from the AquaLogic board.
  4. If the corresponding flag does not change state within **500ms**, it automatically re-sends the key frame, retrying up to a maximum of **3 times**.
  5. While waiting for a confirmation, it rejects new toggle requests to prevent queue collision and command flooding.

### Action Parameters

Both `aqualogic.send` and `aqualogic.toggle` accept the following parameters:

* **`key`** (Required): The key string to send (see the [Available Keys](#1-available-keys) reference table).
* **`type`** (Optional): The frame format used to transmit the key event. Supports the following options:
  - `LOCAL_WIRED`: Local wired control panel frame type (`0x0002`)
  - `REMOTE_WIRED`: Remote wired control panel frame type (`0x0003`)
  - `WIRELESS`: Standard wireless remote frame type (`0x0083`)
  - `WIRELESS2` (Default): Second/modern wireless remote frame type (`0x008c`)

#### Custom Frame Type Example

If you want to emulate a local physical wired panel rather than the default wireless remote:

```yaml
on_press:
  - aqualogic.send:
      key: MENU
      type: LOCAL_WIRED
```

### Display Parsing
Some parameters (like temperatures, salt level, and chlorinator status) are updated by parsing the raw ASCII text broadcast to the character display LCD. The ESP32 parses these string patterns dynamically as the controller cycles through display screens.

---

## Credits & Acknowledgements

The protocol decoding logic, key map offsets, and communication flow are based on the foundational work done by Sean Wilson in the Python [swilson/aqualogic](https://github.com/swilson/aqualogic) library.

---

## License

This project is licensed under the MIT License.
