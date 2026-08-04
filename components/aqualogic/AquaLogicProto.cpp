#include "AquaLogicProto.h"
namespace esphome
{
    namespace aqualogic
    {

        static const char *TAG = "aqualogicproto";

        static std::string trim(const std::string &str) {
            size_t first = str.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) return "";
            size_t last = str.find_last_not_of(" \t\r\n");
            return str.substr(first, (last - first + 1));
        }

        AquaLogicProto::AquaLogicProto()
        {
        }

        size_t AquaLogicProto::ReadFrame(esphome::uart::UARTDevice &port, uint8_t buffer[], size_t maxLength, bool &complete)
        {

            //ESP_LOGD(TAG, "ReadFrame: BytesReadSoFar=%d", _bytesRead);
            // Starting to read the frame if previous was complete
            if (complete)
            {
                complete = false;
                _bytesRead = _frame_start_time = 0;
                _currentState = WAIT_FOR_START;
            }

            bool done = false;
            while (!done && port.available())
            {
                unsigned long diff;
                // Timeout if we are waiting for too long
                if (_frame_start_time > 0 && (diff = millis() - _frame_start_time) > 500)
                {
                    _stats.num_bytes_used -= _bytesRead;
                    ESP_LOGW(TAG, "Timeout, too long from packet start: %lu", (unsigned long)diff);
                    _stats.num_timeouts++;
                    complete = false;
                    _bytesRead = _frame_start_time = 0;
                    _currentState = WAIT_FOR_START;
                    break;
                }

                uint8_t ch = port.read();

                // ESP_LOGD(TAG,"%02x ", ch);
                _stats.num_bytes_received++;
                switch (_currentState)
                {
                case WAIT_FOR_START:
                    if (ch == FRAME_DLE)
                    {
                        _frame_start_time = millis();
                        _currentState = WAIT_FOR_START_END;
                    }
                    break;
                case WAIT_FOR_START_END:
                    if (ch == FRAME_STX)
                    {
                        _currentState = WAIT_FOR_END;
                    }
                    else
                    {
                        // Keep looking for packet start
                        _currentState = WAIT_FOR_START;
                    }
                    break;
                case WAIT_FOR_END:
                    if (ch == FRAME_DLE)
                    {
                        // Save in case its not really the end
                        _currentState = WAIT_FOR_END_END;
                    }
                    else
                    {
                        _stats.num_bytes_used++;
                        buffer[_bytesRead++] = ch;
                    }
                    break;
                case WAIT_FOR_END_END:
                    if (ch == FRAME_ETX)
                    {
                        done = true;

                        _stats.last_packet_received_ms = millis();
                    }
                    else
                    {
                        // if ch == 0, we add FRAME_DLE character back to stream
                        if (!ch)
                        {
                            _stats.num_bytes_used++;
                            buffer[_bytesRead++] = FRAME_DLE;
                            _currentState = WAIT_FOR_END;
                        }
                        else
                        {
                            _currentState = WAIT_FOR_START;
                            _stats.num_bytes_used -= _bytesRead;
                            _bytesRead = 0;
                        }
                    }
                    break;
                default:
                    break;
                }
            }


            
            complete = done;

            if (!complete) {           
                return _bytesRead;
            }

            // special handling for Keep Alive packet
            if (_bytesRead == 4 && memcmp(buffer, FRAME_KEEP_ALIVE_FULL, _bytesRead) == 0)
            {
                if (!port.available()) {
                    SendFrame(port);
                }

                
                _last_keep_alive_time = _stats.last_packet_received_ms;
                _is_last_keep_alive = true;

                // skip Keep Alive packet
                complete = false;
                _bytesRead = 0;
                _currentState = WAIT_FOR_START;
            }
            else
            {
                _is_last_keep_alive = false;

                // Compute CRC
                uint16_t computed_crc = FRAME_DLE + FRAME_STX;
                for (size_t i = 0; i < _bytesRead - 2; i++)
                {
                    computed_crc += buffer[i];
                }

                uint16_t frame_crc = buffer[_bytesRead - 1] | buffer[_bytesRead - 2] << 8;

                if (computed_crc == frame_crc)
                {
                    //ESP_LOGD(TAG, "Packet: %s", convertToHex(buffer, _bytesRead).c_str());

                    // Remove checksum
                    _bytesRead -= 2;
                    // log_d("Time from last KeepAlive %d Frame: %02x %02x", _frame_start_time - _last_keep_alive_time, buffer[0], buffer[1]);
                }
                else
                {
                    _stats.num_crc++;                    
                    ESP_LOGW(TAG, "Bad Packet CRC: Expected=%x Got=%x Packet=%s", frame_crc, computed_crc, convertToHex(buffer, _bytesRead).c_str());
                    complete = false;
                    _bytesRead = 0;
                }
            }

            _frame_start_time = 0;
            _stats.num_bytes_used += 4;
            _stats.num_packets++;

            ESP_LOGV(TAG, "Stats: %d %d", _stats.num_bytes_received, _stats.num_bytes_used);

            return _bytesRead;
        }

        bool AquaLogicProto::SendFrame(esphome::uart::UARTDevice &port)
        {
            if (_frameSize == 0)
                return false;

            if (!_frameJustAdded)
            {
                port.write_array(_frame, _frameSize);
                port.flush();

                ESP_LOGD(TAG, "SentFrame: Frame=%s", convertToHex(_frame, _frameSize).c_str());

                _commandSize = 0;
                _frameSize = 0;
            }
            else
            {
                _frameJustAdded = false;
            }
            return true;
        }

        std::string AquaLogicProto::convertToHex(uint8_t buffer[], size_t length)
        {
            char strBuf[length * 3 + 1] = {'\0'};
            for (size_t i = 0; i < length; i++)
            {
                sprintf(strBuf + strlen(strBuf), "%02x ", buffer[i]);
            }
            return std::string(strBuf);
        }

        data_changed_flags_t AquaLogicProto::ProcessFrame(uint8_t buffer[], size_t length)
        {
            uint16_t frameType = buffer[1] | buffer[0] << 8;
            bool dataChanged = false;
            bool displayChanged = false;

            /*if (frameType == FRAME_TYPE_DISPLAY_UPDATE || frameType == FRAME_TYPE_LONG_DISPLAY_UPDATE || (frameType == FRAME_TYPE_LEDS)) {
                for (size_t i = 0; i < length; i++)
                {
                    Serial.print((uint8_t)buffer[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
            }*/

            if (frameType == FRAME_TYPE_KEEP_ALIVE)
            {
                // Serial.print(".");
            }
            else if (frameType == FRAME_TYPE_DISPLAY_UPDATE)
            {
                size_t displayWidth = (length - 3) / 2; // crc + display byte
                                                        //        frame_display_update_t *frame = (frame_display_update_t *)(&buffer[0]);

                // break up strings into own variables
                char line1[displayWidth + 1];
                char line2[displayWidth + 1];
                memcpy(line1, buffer + sizeof(FRAME_TYPE_DISPLAY_UPDATE), displayWidth);
                memcpy(line2, buffer + sizeof(FRAME_TYPE_DISPLAY_UPDATE) + displayWidth, displayWidth);

                // Add terminators
                line1[displayWidth] = 0;
                line2[displayWidth] = 0;

                if (strcmp(line1, _display.line1_original) == 0 && strcmp(line2, _display.line2_original) == 0)
                    return ERROR;
                // Strings changed

                strcpy(_display.line1_original, line1);
                strcpy(_display.line2_original, line2);

                // Assign blink flags, strip blinks from characters
                for (size_t i = 0; i < displayWidth; i++)
                {
                    _display.line1_blink_state[i] = line1[i] & 0b10000000;
                    _display.line2_blink_state[i] = line2[i] & 0b10000000;
                    line1[i] = line1[i] & 0b01111111;
                    line2[i] = line2[i] & 0b01111111;
                }

                // Copy raw strings
                _display.raw_line1 = line1;
                _display.raw_line2 = line2;

                std::string x = line1;
                x = trim(x);
                if (_display.line1 != x)
                {
                    displayChanged = true;
                    _display.line1 = x;
                }

                x = line2;
                x = trim(x);
                if (_display.line2 != x)
                {
                    displayChanged = true;
                    _display.line2 = x;
                }

                if (displayChanged)
                {
                    ESP_LOGD(TAG, "Display Update Line1(%s) Line2(%s)", _display.line1.c_str(), _display.line2.c_str());
                }

                // Air Temp
                if (_display.line1.rfind("Air Temp", 0) == 0)
                {
                    dataChanged |= ProcessTemp(line1, _param_temp[AIR_TEMP]);
                }
                else if (_display.line1.rfind("Pool Temp", 0) == 0)
                {
                    dataChanged |= ProcessTemp(line1, _param_temp[POOL_TEMP]);
                }
                else if (_display.line1.rfind("Spa Temp", 0) == 0)
                {
                    dataChanged |= ProcessTemp(line1, _param_temp[SPA_TEMP]);
                }
                else if (_display.line1 == "Pool Chlorinator")
                {
                    dataChanged |= ProcessChlorinator(line2, _param_pct[POOL_CHLORINATOR]);
                }
                else if (_display.line1 == "Spa Chlorinator")
                {
                    dataChanged |= ProcessChlorinator(line2, _param_pct[SPA_CHLORINATOR]);
                }
                else if (_display.line1 == "Salt Level")
                {
                    dataChanged |= ProcessSaltLevel(line2);
                }
                else if (_display.line1 == "Gas Heater")
                {
                    dataChanged |= ProcessGasHeater(line2);
                }
            }
            else if (frameType == FRAME_TYPE_LEDS)
            {
                if (length != 10)
                {
                    ESP_LOGW(TAG, "LED - unexpected length!");
                    return ERROR;
                }
                uint32_t states = (uint32_t)buffer[5] << 24 | (uint32_t)buffer[4] << 16 | (uint32_t)buffer[3] << 8 | (uint32_t)buffer[2];
                uint32_t blink_states = (uint32_t)buffer[9] << 24 | (uint32_t)buffer[8] << 16 | (uint32_t)buffer[7] << 8 | (uint32_t)buffer[6];

                bool changed = ProcessLeds(states, blink_states);
                if (changed)
                {
                    ESP_LOGD(TAG, "States: %lx Blink: %lx", (unsigned long)states, (unsigned long)blink_states);
                }

                dataChanged |= changed;
            }
            else if (frameType == FRAME_TYPE_PUMP_SPEED_REQUEST)
            {
                // Expected message to be 2 bytes
                int pumpSpeed = (uint16_t)buffer[2] << 8 | (uint16_t)buffer[3];
                if (pumpSpeed != _pump.speed)
                {
                    //    _param_pump_speed = pumpSpeed;
                    //    dataChanged = true;
                }
            }
            else if (frameType == FRAME_TYPE_PUMP_STATUS)
            {
                // Expected message to be 7 bytes
                int pumpSpeed = (int)buffer[3] << 8 | (int)buffer[4];
                int power = ((((buffer[5] & 0xf0) >> 4) * 1000) +
                             (((buffer[5] & 0x0f)) * 100) +
                             (((buffer[6] & 0xf0) >> 4) * 10) +
                             (((buffer[6] & 0x0f))));

                if (pumpSpeed != _pump.speed || power != _pump.power)
                {
                    _pump.speed = pumpSpeed;
                    _pump.power = power;
                    dataChanged = true;
                    ESP_LOGD(TAG, "Pump Speed: Speed=%d Power=%d", pumpSpeed, power);
                }
            }
            else if (frameType == FRAME_TYPE_LONG_DISPLAY_UPDATE)
            {
                // Do nothing, this is combination of LED + Display
            }
            else if (frameType == FRAME_TYPE_LOCAL_WIRED_KEY_EVENT || frameType == FRAME_TYPE_REMOTE_WIRED_KEY_EVENT)
            {
                uint32_t key1 = 0;
                uint32_t key2 = 0;
                bool has_copy = false;

                if (length == 4)
                {
                    key1 = (uint32_t)buffer[2] | (uint32_t)buffer[3] << 8;
                    has_copy = false;
                }
                else if (length == 10)
                {
                    key1 = (uint32_t)buffer[2] | (uint32_t)buffer[3] << 8 | (uint32_t)buffer[4] << 16 | (uint32_t)buffer[5] << 24;
                    key2 = (uint32_t)buffer[6] | (uint32_t)buffer[7] << 8 | (uint32_t)buffer[8] << 16 | (uint32_t)buffer[9] << 24;
                    has_copy = true;
                }
                else
                {
                    ESP_LOGW(TAG, "Wired Key Event Length Mismatch: Expected 4 or 10, Got=%d", length);
                    return ERROR;
                }

                if (has_copy)
                {
                    ESP_LOGD(TAG, "Wired Key Event (10-byte): Key1=%lx Key2=%lx Packet: %s", (unsigned long)key1, (unsigned long)key2, convertToHex(buffer, length).c_str());
                    if (key1 != key2)
                    {
                        ESP_LOGW(TAG, "Wired Key Event Mismatch: Key1=%lx Key2=%lx", (unsigned long)key1, (unsigned long)key2);
                        return ERROR;
                    }
                }
                else
                {
                    ESP_LOGD(TAG, "Wired Key Event (4-byte): Key=%lx Packet: %s", (unsigned long)key1, convertToHex(buffer, length).c_str());
                }

                if (key1 == 0)
                {
                    ESP_LOGD(TAG, "Wired Key Released: Key=%lx Packet: %s", (unsigned long)key1, convertToHex(buffer, length).c_str());
                    return NOOP;
                }

                enum CONTROLLER_KEYS key = static_cast<enum CONTROLLER_KEYS>(key1);
                ESP_LOGD(TAG, "Got Wired Key: KeyId=%lx Key=%s", (unsigned long)key, GetKeyName(key));
            }
            else if (frameType == FRAME_TYPE_WIRELESS_KEY_EVENT || frameType == FRAME_TYPE_WIRELESS2_KEY_EVENT)
            {
                // Wireless Key 2(frame type) + 01 + 4x2 (keys) + 00
                if (length != 12)
                {
                    ESP_LOGW(TAG, "Key Event Length Missmatch: Expected=12 Got=%d", length);
                    return ERROR;
                }

                // 4 byte key
                uint32_t key1 = (uint32_t)buffer[3] | (uint32_t)buffer[4] << 8 | (uint32_t)buffer[5] << 16 | (uint32_t)buffer[6] << 24;
                uint32_t key2 = (uint32_t)buffer[7] | (uint32_t)buffer[8] << 8 | (uint32_t)buffer[9] << 16 | (uint32_t)buffer[10] << 24;

                ESP_LOGD(TAG, "Key Event: Key1=%lx Key2=%lx Packet: %s", (unsigned long)key1, (unsigned long)key2, convertToHex(buffer, length).c_str());


                if (key1 != key2)
                {
                    ESP_LOGW(TAG, "Key event Missmatch: Key1=%lx Key2=%lx", (unsigned long)key1, (unsigned long)key2);
                    return ERROR;
                }

                if (key1 == 0)
                {
                    ESP_LOGD(TAG, "Key Released: Key=%lx Packet: %s", (unsigned long)key1, convertToHex(buffer, length).c_str());
                    return NOOP;
                }

                enum CONTROLLER_KEYS key = static_cast<enum CONTROLLER_KEYS>(key1);
                ESP_LOGD(TAG, "Got Key: KeyId=%lx Key=%s", (unsigned long)key, GetKeyName(key));
            }
            else
            {
                ESP_LOGD(TAG, "Unknown Frame: %s", convertToHex(buffer, length).c_str());
            }

            data_changed_flags_t result = NOOP;

            // Call DataChanged callback
            if (dataChanged)
            {
                result = result | DATA_CHANGED;

                if (_dataChangeCallback != nullptr)
                    _dataChangeCallback(*this);
            }

            // Call DataChanged callback
            if (displayChanged)
            {
                result = result | DISPLAY_CHANGED;
                if (_displayChangeCallBack != nullptr)
                    _displayChangeCallBack(*this);
            }

            return result;
        }

        bool AquaLogicProto::ProcessTemp(const char *line, float &variable)
        {
            bool changed = false;
            char tmp[strlen(line) + 1];
            strcpy(tmp, line);
            // Digits are 3rd token
            char *token = strtok(tmp, " ");
            token = strtok(NULL, " ");
            token = strtok(NULL, " ");

            if (token[strlen(token) - 1] == 'F' && _flags[IS_METRIC])
            {
                _flags[IS_METRIC] = false;
                changed = true;
            }
            else if (token[strlen(token) - 1] == 'C' && !_flags[IS_METRIC])
            {
                _flags[IS_METRIC] = true;
                changed = true;
            }

            // Get rid of degree
            token[strlen(token) - 2] = 0;

            float newValue = atof(token);

            if (newValue != variable)
            {
                variable = newValue;
                changed = true;
            }

            return changed;
        }

        bool AquaLogicProto::ProcessChlorinator(const char *line, float &variable)
        {
            // X Chlorinator <value>%
            bool changed = false;
            char tmp[strlen(line) + 1];
            strcpy(tmp, line);
            char *token = strtok(tmp, " ");

            // Get rid of %
            token[strlen(token) - 1] = 0;

            float newValue = atof(token);

            if (newValue != variable)
            {
                variable = newValue;
                changed = true;
            }

            return changed;
        }

        bool AquaLogicProto::ProcessSaltLevel(const char *line)
        {
            // Salt Level <value> [g/L|PPM|

            bool changed = false;
            char tmp[strlen(line) + 1];
            strcpy(tmp, line);
            char *token = strtok(tmp, " ");

            // Get rid of %
            int newValue = atoi(token);

            if (newValue != _param_salt_level)
            {
                _param_salt_level = newValue;
                changed = true;
            }

            return changed;
        }

        bool AquaLogicProto::ProcessGasHeater(const char *line)
        {
            // Gas Heater / Auto / Manual
            bool changed = false;

            std::string val(line);
            val = trim(val);
            if (val.find("Auto") != std::string::npos && !_flags[HEATER_AUTO])
            {
                _flags[HEATER_AUTO] = true;
                changed = true;
            }
            else if (val == "Manual Off" && _flags[HEATER_AUTO])
            {
                _flags[HEATER_AUTO] = false;
                changed = true;
            }
            return changed;
        }

        bool AquaLogicProto::ProcessLeds(uint32_t states, uint32_t blink_states)
        {
            bool changed = false;
            for (size_t i = 0; i < (sizeof(_leds) / sizeof(_leds[0])); i++)
            {
                bool is_solid = (states & _leds[i]) != 0;
                if (is_solid && !_flags[i])
                {
                    _flags[i] = true;
                    changed = true;
                }
                else if (!is_solid && _flags[i])
                {
                    _flags[i] = false;
                    changed = true;
                }
            }

            // FILTER_LOW_SPEED (index 29) -> Filter LED is blinking (index 5)
            bool filter_low = (blink_states & _leds[5]) != 0;
            if (filter_low != _flags[FILTER_LOW_SPEED])
            {
                _flags[FILTER_LOW_SPEED] = filter_low;
                changed = true;
            }

            // HEATER_BLINKING (index 30) -> Heater LED is blinking (index 0)
            bool heater_blink = (blink_states & _leds[0]) != 0;
            if (heater_blink != _flags[HEATER_BLINKING])
            {
                _flags[HEATER_BLINKING] = heater_blink;
                changed = true;
            }

            // CHECK_SYSTEM_BLINKING (index 31) -> Check System LED is blinking (index 2)
            bool check_sys_blink = (blink_states & _leds[2]) != 0;
            if (check_sys_blink != _flags[CHECK_SYSTEM_BLINKING])
            {
                _flags[CHECK_SYSTEM_BLINKING] = check_sys_blink;
                changed = true;
            }

            return changed;
        }

        void AquaLogicProto::OnDataChangeCallback(DataChangeCallback cb)
        {
            _dataChangeCallback = cb;
        }

        void AquaLogicProto::OnDisplayChangeCallback(DisplayChangeCallback cb)
        {
            _displayChangeCallBack = cb;
        }

        bool AquaLogicProto::GetFlag(CONTROLLER_FLAGS flag)
        {
            return _flags[flag];
        }

        struct display_state_t AquaLogicProto::GetDisplay()
        {
            return _display;
        }

        float AquaLogicProto::GetTemp(enum CONTROLLER_TEMP_PARAM temp)
        {
            return _param_temp[temp];
        }

        float AquaLogicProto::GetPct(enum CONTROLLER_PCT_PARAM pct)
        {
            return _param_pct[pct];
        }

        struct pump_state_t AquaLogicProto::GetPumpStatus()
        {
            return _pump;
        }

        int AquaLogicProto::GetSaltLevel()
        {
            return _param_salt_level;
        }

        struct packet_stats_t AquaLogicProto::GetStats()
        {
            return _stats;
        }

        const char *AquaLogicProto::GetKeyName(enum CONTROLLER_KEYS key)
        {
            switch (key)
            {
            case KEY_RIGHT:
                return "RIGHT";
            case KEY_MENU:
                return "MENU";
            case KEY_LEFT:
                return "LEFT";
            case KEY_UNLOCK:
                return "UNLOCK";
            case KEY_SERVICE:
                return "SERVICE";
            case KEY_MINUS:
                return "MINUS";
            case KEY_PLUS:
                return "PLUS";
            case KEY_POOL_SPA:
                return "POOL_SPA";
            case KEY_FILTER:
                return "FILTER";
            case KEY_LIGHTS:
                return "LIGHTS";
            case KEY_AUX_1:
                return "AUX_1";
            case KEY_AUX_2:
                return "AUX_2";
            case KEY_AUX_3:
                return "AUX_3";
            case KEY_AUX_4:
                return "AUX_4";
            case KEY_AUX_5:
                return "AUX_5";
            case KEY_AUX_6:
                return "AUX_6";
            case KEY_AUX_7:
                return "AUX_7";
            case KEY_VALVE_3:
                return "VALVE_3";
            case KEY_VALVE_4:
                return "VALVE_4";
            case KEY_HEATER_1:
                return "HEATER_1";
            case KEY_AUX_8:
                return "AUX_8";
            case KEY_AUX_9:
                return "AUX_9";
            case KEY_AUX_10:
                return "AUX_10";
            case KEY_AUX_11:
                return "AUX_11";
            case KEY_AUX_12:
                return "AUX_12";
            case KEY_AUX_13:
                return "AUX_13";
            case KEY_AUX_14:
                return "AUX_14";
            default:
                return "UNKNOWN";
            }
        }

        char *strupper(char *str)
        {
            unsigned char *p = (unsigned char *)str;

            while (*p)
            {
                *p = toupper((unsigned char)*p);
                p++;
            }

            return str;
        }

        CONTROLLER_KEYS AquaLogicProto::GetKeyByName(const char *value)
        {

            // Covnert to upper case
            char copy[strlen(value) + 1];
            strcpy(copy, value);
            char *upper = strupper(copy);

            ESP_LOGD(TAG, "UpperCase: %s", upper);
            /*const char OFFSET = 'a' - 'A';
            for (size_t i = 0; i < strlen(upper); i++)
            {
                char c = value[i];
                if (c >= 'a' && c <= 'z')
                {
                    upper[i] = c - OFFSET;
                }
                else
                {
                    upper[i] = c;
                }
            }
            */
            for (size_t i = 0; i < NUM_FLAGS; i++)
            {
                enum CONTROLLER_KEYS key = static_cast<enum CONTROLLER_KEYS>(i);
                if (strcmp(GetKeyName(key), upper) == 0)
                    return key;
            }

            return static_cast<enum CONTROLLER_KEYS>(0);
        }

        void AquaLogicProto::SendCommand(const uint16_t type, const enum CONTROLLER_KEYS key, const uint8_t action, const uint8_t wired_key_bytes)
        {
            if (key == KEY_NONE && (type == FRAME_TYPE_WIRELESS_KEY_EVENT || type == FRAME_TYPE_WIRELESS2_KEY_EVENT))
            {
                ESP_LOGD(TAG, "SendCommand: Skipping KEY_NONE for wireless");
                return;
            }

            // Command type
            _commandBuffer[0] = type >> 8;
            _commandBuffer[1] = type;

            if (type == FRAME_TYPE_LOCAL_WIRED_KEY_EVENT || type == FRAME_TYPE_REMOTE_WIRED_KEY_EVENT)
            {
                if (wired_key_bytes == 2)
                {
                    // Wired frame (2-byte keys): type (2) + press_keys (2) + release_keys (2) = 6 bytes total
                    _commandSize = 6;
                    if (action == 1) // Press
                    {
                        _commandBuffer[2] = key;
                        _commandBuffer[3] = key >> 8;
                        _commandBuffer[4] = 0;
                        _commandBuffer[5] = 0;
                    }
                    else if (action == 2) // Release
                    {
                        _commandBuffer[2] = 0;
                        _commandBuffer[3] = 0;
                        _commandBuffer[4] = key;
                        _commandBuffer[5] = key >> 8;
                    }
                    else // action == 3 (Click: Press + Release combined)
                    {
                        _commandBuffer[2] = key;
                        _commandBuffer[3] = key >> 8;
                        _commandBuffer[4] = key;
                        _commandBuffer[5] = key >> 8;
                    }
                }
                else
                {
                    // Wired frame (4-byte keys): type (2) + press_keys (4) + release_keys (4) = 10 bytes total
                    _commandSize = 10;
                    if (action == 1) // Press
                    {
                        _commandBuffer[2] = key;
                        _commandBuffer[3] = key >> 8;
                        _commandBuffer[4] = key >> 16;
                        _commandBuffer[5] = key >> 24;
                        _commandBuffer[6] = 0;
                        _commandBuffer[7] = 0;
                        _commandBuffer[8] = 0;
                        _commandBuffer[9] = 0;
                    }
                    else if (action == 2) // Release
                    {
                        _commandBuffer[2] = 0;
                        _commandBuffer[3] = 0;
                        _commandBuffer[4] = 0;
                        _commandBuffer[5] = 0;
                        _commandBuffer[6] = key;
                        _commandBuffer[7] = key >> 8;
                        _commandBuffer[8] = key >> 16;
                        _commandBuffer[9] = key >> 24;
                    }
                    else // action == 3 (Click: Press + Release combined)
                    {
                        _commandBuffer[2] = key;
                        _commandBuffer[3] = key >> 8;
                        _commandBuffer[4] = key >> 16;
                        _commandBuffer[5] = key >> 24;
                        _commandBuffer[6] = key;
                        _commandBuffer[7] = key >> 8;
                        _commandBuffer[8] = key >> 16;
                        _commandBuffer[9] = key >> 24;
                    }
                }
            }
            else
            {
                // Wireless frame: type (2) + action (1) + key (4) + key copy (4) + end (1) = 12 bytes total
                _commandSize = 12;
                _commandBuffer[2] = action; // 1 = Press, 2 = Release
                _commandBuffer[3] = key;
                _commandBuffer[4] = key >> 8;
                _commandBuffer[5] = key >> 16;
                _commandBuffer[6] = key >> 24;
                _commandBuffer[7] = key;
                _commandBuffer[8] = key >> 8;
                _commandBuffer[9] = key >> 16;
                _commandBuffer[10] = key >> 24;
                _commandBuffer[11] = 0;
            }

            ESP_LOGD(TAG, "SendCommand: Type=%04x Key=%lx Action=%d Size=%d Payload=%s", 
                    type, (unsigned long)key, action, _commandSize, convertToHex(_commandBuffer, _commandSize).c_str());

            GenerateFrame();
        }

        bool AquaLogicProto::CanSend()
        {
            return _commandSize == 0;
        }

        void AquaLogicProto::GenerateFrame()
        {
            // The CRC is computed over the raw bytes (Start + Payload)
            // But the frame sent over UART must be DLE stuffed (DLE -> DLE 0x00)
            
            // CRC includes DLE + STX + Payload
            uint16_t crc = FRAME_DLE + FRAME_STX;
            for (size_t i = 0; i < _commandSize; i++)
            {
                crc += _commandBuffer[i];
            }

            _frame[0] = FRAME_DLE;
            _frame[1] = FRAME_STX;
            size_t n = 2;

            // Stuff payload
            for (size_t i = 0; i < _commandSize; i++)
            {
                _frame[n++] = _commandBuffer[i];
                if (_commandBuffer[i] == FRAME_DLE)
                {
                    _frame[n++] = 0x00;
                }
            }

            // Stuff CRC (Big Endian)
            uint8_t crc_h = (uint8_t)(crc >> 8);
            uint8_t crc_l = (uint8_t)(crc & 0xFF);

            _frame[n++] = crc_h;
            if (crc_h == FRAME_DLE) _frame[n++] = 0x00;

            _frame[n++] = crc_l;
            if (crc_l == FRAME_DLE) _frame[n++] = 0x00;

            // End markers
            _frame[n++] = FRAME_DLE;
            _frame[n++] = FRAME_ETX;

            _frameSize = n;

            // It is possible that we are generating frame while data is waiting on UART, so we are going to skip one KeepAlive
            _frameJustAdded = true;
        }

    }
}