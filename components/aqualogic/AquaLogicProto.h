#pragma once

/*
  Created by Andrei Rjeousski
*/

#ifdef USE_ARDUINO
#include "Arduino.h"
#else
#include <string>
#include <cstdint>
#include "esphome/core/helpers.h"
#endif
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome
{
  namespace aqualogic
  {

#define MAX_COMMAND_SIZE 10

    const uint16_t BUTTON_COMMAND_BUFFER = 64;

    const uint16_t FRAME_DLE = 0x10;
    const uint16_t FRAME_STX = 0x02;
    const uint16_t FRAME_ETX = 0x03;

    // Local wired panel (black face with service button)
    const uint16_t FRAME_TYPE_LOCAL_WIRED_KEY_EVENT = 0x0002;
    // Remote wired panel (white face)
    const uint16_t FRAME_TYPE_REMOTE_WIRED_KEY_EVENT = 0x0003;
    // Wireless remote
    const uint16_t FRAME_TYPE_WIRELESS_KEY_EVENT = 0x0083;
    const uint16_t FRAME_TYPE_WIRELESS2_KEY_EVENT = 0x008c;

    const uint16_t FRAME_TYPE_KEEP_ALIVE = 0x0101;
    const uint16_t FRAME_TYPE_LEDS = 0x0102;
    const uint16_t FRAME_TYPE_DISPLAY_UPDATE = 0x0103;
    const uint16_t FRAME_TYPE_LONG_DISPLAY_UPDATE = 0x040a;

    const uint16_t FRAME_TYPE_PUMP_SPEED_REQUEST = 0x0c01;
    const uint16_t FRAME_TYPE_PUMP_STATUS = 0x000c;

    const uint8_t FRAME_KEEP_ALIVE_FULL[4] = {0x01, 0x01, 0x00, 0x14};

    enum CONTROLLER_PCT_PARAM
    {
      POOL_CHLORINATOR = 0,
      SPA_CHLORINATOR,
      NUM_PCT_PARAM
    };

    enum CONTROLLER_TEMP_PARAM
    {
      AIR_TEMP = 0,
      POOL_TEMP,
      SPA_TEMP,
      NUM_TEMP_PARAM
    };

    enum CONTROLLER_FLAGS
    {
      // These correspond to the LEDs on the unit
      HEATER_1 = 0,
      VALVE_3,
      CHECK_SYSTEM,
      POOL,
      SPA,
      FILTER,
      LIGHTS,
      AUX_1,
      AUX_2,
      SERVICE,
      AUX_3,
      AUX_4,
      AUX_5,
      AUX_6,
      VALVE_4,
      SPILLOVER,
      SYSTEM_OFF,
      AUX_7,
      AUX_8,
      AUX_9,
      AUX_10,
      AUX_11,
      AUX_12,
      AUX_13,
      AUX_14,
      SUPER_CHLORINATE,
      IS_METRIC,
      HEATER_AUTO,
      CHECK_SYSTEM_MSG,
      FILTER_LOW_SPEED,
      HEATER_BLINKING,
      CHECK_SYSTEM_BLINKING,
      NUM_FLAGS
    };

    const char *const CONTROLLER_FLAG_NAMES[] = {
        "HEATER_1",
        "VALVE_3",
        "CHECK_SYSTEM",
        "POOL",
        "SPA",
        "FILTER",
        "LIGHTS",
        "AUX_1",
        "AUX_2",
        "SERVICE",
        "AUX_3",
        "AUX_4",
        "AUX_5",
        "AUX_6",
        "VALVE_4",
        "SPILLOVER",
        "SYSTEM_OFF",
        "AUX_7",
        "AUX_8",
        "AUX_9",
        "AUX_10",
        "AUX_11",
        "AUX_12",
        "AUX_13",
        "AUX_14",
        "SUPER_CHLORINATE",
        "IS_METRIC",
        "HEATER_AUTO",
        "CHECK_SYSTEM_MSG",
        "FILTER_LOW_SPEED",
        "HEATER_BLINKING",
        "CHECK_SYSTEM_BLINKING",
        "NUM_FLAGS"};

    enum CONTROLLER_KEYS : unsigned long
    {
      // TODO: Second word is the same on first down, 0000 every 100ms while holding
      KEY_NONE      = 0,
      KEY_RIGHT     = 0x0001,
      KEY_MENU      = 0x0002,
      KEY_LEFT      = 0x0004,
      KEY_UNLOCK    = 0x0005,
      KEY_SERVICE   = 0x0008,
      KEY_MINUS     = 0x0010,
      KEY_PLUS      = 0x0020,
      KEY_POOL_SPA  = 0x0040,
      KEY_FILTER    = 0x0080,
      KEY_LIGHTS    = 0x0100,
      KEY_AUX_1     = 0x0200,
      KEY_AUX_2     = 0x0400,
      KEY_AUX_3     = 0x0800,
      KEY_AUX_4     = 0x1000,
      KEY_AUX_5     = 0x2000,
      KEY_AUX_6     = 0x4000,
      KEY_AUX_7     = 0x8000,
      // These are only valid for WIRELESS_KEY_EVENTs
      KEY_VALVE_3   = 0x00010000,
      KEY_VALVE_4   = 0x00020000,
      KEY_HEATER_1  = 0x00040000,
      KEY_AUX_8     = 0x00080000,
      KEY_AUX_9     = 0x00100000,
      KEY_AUX_10    = 0x00200000,
      KEY_AUX_11    = 0x00400000,
      KEY_AUX_12    = 0x00800000,
      KEY_AUX_13    = 0x01000000,
      KEY_AUX_14    = 0x02000000
    };

    struct display_state_t
    {
      std::string line1;
      std::string line2;
      std::string raw_line1;
      std::string raw_line2;

      char line1_original[21] = {'\0'};
      char line2_original[21] = {'\0'};
      bool line1_blink_state[20];
      bool line2_blink_state[20];
    };

    struct pump_state_t
    {
      int speed;
      int power;
    };

    struct packet_stats_t
    {
      unsigned long num_packets = 0;
      unsigned long num_crc = 0;
      unsigned long num_timeouts = 0;
      unsigned long num_bytes_received = 0;
      unsigned long num_bytes_used = 0;
      unsigned long last_packet_received_ms = 0;
    };

    enum data_changed_flags_t
    {
      NOOP = 0,
      ERROR = 1 << 0,
      NO_CHANGED = 1 << 1,
      DATA_CHANGED = 1 << 2,
      DISPLAY_CHANGED = 1 << 3,
      KEY_PRESSED = 1 << 4,
    };

    inline data_changed_flags_t operator|(data_changed_flags_t a, data_changed_flags_t b)
    {
      return static_cast<data_changed_flags_t>(static_cast<int>(a) | static_cast<int>(b));
    }

    class AquaLogicProto;

    // Callbacks
    typedef void (*DataChangeCallback)(AquaLogicProto &data);
    typedef void (*DisplayChangeCallback)(AquaLogicProto &data);
    typedef void (*SwitchModeCallback)(bool isTx);

    class AquaLogicProto
    {
    public:
      AquaLogicProto();

      size_t ReadFrame(esphome::uart::UARTDevice &port, uint8_t buffer[], size_t maxLength, bool &complete);
      data_changed_flags_t ProcessFrame(uint8_t buffer[], size_t length);

      // Get States
      bool GetFlag(enum CONTROLLER_FLAGS flag);
      struct display_state_t GetDisplay();
      struct pump_state_t GetPumpStatus();
      int GetSaltLevel();
      float GetTemp(enum CONTROLLER_TEMP_PARAM temp);
      float GetPct(enum CONTROLLER_PCT_PARAM pct);

      // Sending
      bool CanSend();
      void SendCommand(const uint16_t type, const enum CONTROLLER_KEYS key, const uint8_t action = 1, const uint8_t wired_key_bytes = 4);

      // Key Resolution
      CONTROLLER_KEYS GetKeyByName(const char *);
      const char *GetKeyName(enum CONTROLLER_KEYS key);

      // Internal Stats
      struct packet_stats_t GetStats();

      // Callbacks
      void OnDataChangeCallback(DataChangeCallback cb);
      void OnDisplayChangeCallback(DisplayChangeCallback cb);
      std::string FormatDisplayLine(const std::string &raw_line, const bool blink_states[20]);

    private:
      enum state
      {
        WAIT_FOR_START = 0,
        WAIT_FOR_START_END = 1,
        WAIT_FOR_END = 2,
        WAIT_FOR_END_END = 3
      };

      // Internal Processing Variables
      // These variables are used only be Frame Reader
      state _currentState;
      size_t _bytesRead;
      unsigned long _frame_start_time;
      unsigned long _last_keep_alive_time;
      bool _is_last_keep_alive;

      uint8_t _commandBuffer[64];
      size_t _commandSize = 0;

      uint8_t _frame[64];
      size_t _frameSize = 0;
      bool _frameJustAdded = false;

      // Display State
      struct display_state_t _display;
      struct pump_state_t _pump;
      struct packet_stats_t _stats;

      DataChangeCallback _dataChangeCallback = nullptr;
      DisplayChangeCallback _displayChangeCallBack = nullptr;

      float _param_temp[NUM_TEMP_PARAM] = {0.0f};
      float _param_pct[NUM_PCT_PARAM] = {0.0f};
      bool _flags[CONTROLLER_FLAGS::NUM_FLAGS] = {false};
      int _param_salt_level = 0;

      const unsigned long _leds[26] = {
          1UL << 0,  // HEATER_1
          1UL << 1,  // VALVE_3
          1UL << 2,  // CHECK_SYSTEM
          1UL << 3,  // POOL
          1UL << 4,  // SPA
          1UL << 5,  // FILTER
          1UL << 6,  // LIGHTS
          1UL << 7,  // AUX_1
          1UL << 8,  // AUX_2
          1UL << 9,  // SERVICE
          1UL << 10, // AUX_3
          1UL << 11, // AUX_4
          1UL << 12, // AUX_5
          1UL << 13, // AUX_6
          1UL << 14, // VALVE_4
          1UL << 15, // SPILLOVER
          1UL << 16, // SYSTEM_OFF
          1UL << 17, // AUX_7
          1UL << 18, // AUX_8
          1UL << 19, // AUX_9
          1UL << 20, // AUX_10
          1UL << 21, // AUX_11
          1UL << 22, // AUX_12
          1UL << 23, // AUX_13
          1UL << 24, // AUX_14
          1UL << 25  // SUPER_CHLORINATE
      };

      bool ProcessTemp(const char *line, float &variable);
      bool ProcessChlorinator(const char *line, float &variable);
      bool ProcessGasHeater(const char *line);
      bool ProcessSaltLevel(const char *line);
      bool ProcessLeds(uint32_t states, uint32_t blink_states);
      bool SendFrame(esphome::uart::UARTDevice &port);      
      std::string convertToHex(uint8_t buffer[], size_t length);
      void GenerateFrame();
    };

  }
}
