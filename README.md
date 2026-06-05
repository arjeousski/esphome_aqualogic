# ESPHome AquaLogic / ProLogic Pool Controller Component

This custom ESPHome component interfaces with **Hayward/Goldline AquaLogic and ProLogic** pool automation systems. It communicates with the pool controller's main board over an RS-485 bus using UART, allowing you to monitor sensors, mirror the LCD display, emulate panel keys, and automate pool functions (like heating schedules) directly from ESPHome and Home Assistant.

> [!IMPORTANT]
> This project is not affiliated with or endorsed by Hayward Industries Inc. in any way.

## Features

- **Sensors**: Monitor pool temperature, spa temperature, air temperature, pump speed, pump power, salt level, and chlorinator percentages.
- **Binary Sensors**: Track real-time status of the filter pump, lights, heater (actively firing), heater auto mode (enabled), waterfall (valve 3), and system warning alarms (Check System).
- **Text Sensors**: Mirror display line 1, display line 2, and active controller flags.
- **Controls (Buttons)**: Emulate physical key presses for panel navigation (`Menu`, `Left`, `Right`, `Plus`, `Minus`) and toggle pool functions (`Filter`, `Lights`, `Heater (auto)`, `Waterfall`).
- **Heater Scheduler**: Configurable allowed run-windows with automatic timezone synchronization.

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

To use this component in your ESPHome configuration, you can reference it either directly from your Git repository (ideal for sharing and production) or as a local component (great for development).

### 1. Remote installation (via Git)
Add the following to your ESPHome configuration file, replacing the URL with your GitLab repository:

```yaml
external_components:
  - source:
      type: git
      url: https://gitlab.com/your-username/aqualogic-esphome.git
      # Optional: specify a branch, tag, or commit ref
      # ref: main
```

### 2. Local installation
Copy the `aqualogic` folder into the `external_components` directory of your ESPHome project, then reference it locally:

```yaml
external_components:
  - source: external_components
```

---

## Configuration Example

Below is a typical configuration snippet for [poolcontroller.yaml](poolcontroller.yaml):

```yaml
# Configure UART interface (AquaLogic runs at 19200 baud, 8N2)
uart:
  tx_pin: 27
  rx_pin: 26
  baud_rate: 19200
  stop_bits: 2

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
    heater_auto:
      id: heater_auto
      name: "Heater Auto Status"
    heater_1:
      name: "Heater Status"
    valve_3:
      name: "Waterfall Status"
    check_system:
      name: "Check System Warning"
      device_class: problem

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

## How It Works

### Key Emulation: Send vs Toggle Actions

The component exposes two distinct actions for transmitting panel commands over RS-485:

#### 1. `aqualogic.send` (Fire-and-Forget)
- **Use Case**: Navigation keys (`LEFT`, `RIGHT`, `PLUS`, `MINUS`, `MENU`).
- **How it works**: Transmits the key event frame over the RS-485 line exactly once. Because navigation keys do not correspond to a single binary state flag on the panel (e.g., pressing `MENU` doesn't switch on a physical status LED), the component does not listen for confirmation or attempt auto-retries.

#### 2. `aqualogic.toggle` (Confirmed with Auto-Retries)
- **Use Case**: Equipment toggles (`FILTER`, `LIGHTS`, `HEATER_1`, `VALVE_3`).
- **How it works**:
  1. Records the **initial state** of the corresponding feature flag.
  2. Transmits the key event frame.
  3. Listens to the incoming status broadcasts from the AquaLogic board.
  4. If the corresponding flag does not change state within **500ms**, it automatically re-sends the key frame, retrying up to a maximum of **3 times**.
  5. While waiting for a confirmation, it rejects new toggle requests to prevent queue collision and command flooding.

### Display Parsing
Some parameters (like temperatures, salt level, and chlorinator status) are updated by parsing the raw ASCII text broadcast to the character display LCD. The ESP32 parses these string patterns dynamically as the controller cycles through display screens.

---

## Credits & Acknowledgements

The protocol decoding logic, key map offsets, and communication flow are based on the foundational work done by Sean Wilson in the Python [swilson/aqualogic](https://github.com/swilson/aqualogic) library.

---

## License

This project is licensed under the MIT License.
