#include "aqualogic.h"

namespace esphome {
namespace aqualogic {

static const char *TAG = "aqualogic.component";

// Key to flag mapping definition
const std::unordered_map<CONTROLLER_KEYS, CONTROLLER_FLAGS> AquaLogicComponent::key_to_flag_map_ = {
    {KEY_FILTER, FILTER},
    {KEY_LIGHTS, LIGHTS},
    {KEY_VALVE_3, VALVE_3},
    {KEY_VALVE_4, VALVE_4},
    {KEY_HEATER_1, HEATER_AUTO},
    {KEY_AUX_1, AUX_1},
    {KEY_AUX_2, AUX_2},
    {KEY_AUX_3, AUX_3},
    {KEY_AUX_4, AUX_4},
    {KEY_AUX_5, AUX_5},
    {KEY_AUX_6, AUX_6},
    {KEY_AUX_7, AUX_7},
    {KEY_AUX_8, AUX_8},
    {KEY_AUX_9, AUX_9},
    {KEY_AUX_10, AUX_10},
    {KEY_AUX_11, AUX_11},
    {KEY_AUX_12, AUX_12},
    {KEY_AUX_13, AUX_13},
    {KEY_AUX_14, AUX_14}
};

#ifdef USE_BINARY_SENSOR
static const CONTROLLER_FLAGS AUX_FLAGS[14] = {
    AUX_1, AUX_2, AUX_3, AUX_4, AUX_5, AUX_6, AUX_7,
    AUX_8, AUX_9, AUX_10, AUX_11, AUX_12, AUX_13, AUX_14
};
#endif

// Forward declarations
void dataChanged(AquaLogicProto &obj);

static std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static std::string format_display_line(const std::string &raw_line, const bool blink_states[20]) {
    std::string formatted = "";
    bool in_blink = false;
    for (size_t i = 0; i < raw_line.length() && i < 20; i++) {
        bool char_blinks = blink_states[i];
        if (char_blinks && !in_blink) {
            formatted += '[';
            in_blink = true;
        } else if (!char_blinks && in_blink) {
            formatted += ']';
            in_blink = false;
        }
        formatted += raw_line[i];
    }
    if (in_blink) {
        formatted += ']';
    }
    return trim(formatted);
}

// Send key with confirmation retry functionality
void AquaLogicComponent::send_key_with_retry(CONTROLLER_KEYS key, uint16_t type) {
    // Check if key is in map
    auto it = key_to_flag_map_.find(key);
    if (it == key_to_flag_map_.end()) {
        ESP_LOGW(TAG, "Key %s not in key_to_flag_map_, not using retry logic", aqua_->GetKeyName(key));
        // Fall back to direct key send without retry
        send_key(key, type);
        return;
    }

    if (waiting_for_confirmation_) {
        ESP_LOGW(TAG, "Already waiting for key confirmation, ignoring new key");
        return;
    }

    initial_flag_state_ = aqua_->GetFlag(it->second);
    ESP_LOGD(TAG, "Initial flag state for %s: %s", 
            aqua_->GetKeyName(key), initial_flag_state_ ? "true" : "false");

    pending_key_ = key;
    pending_key_type_ = type;
    waiting_for_confirmation_ = true;
    key_retry_count_ = 0;
    last_key_send_time_ = 0; // Force immediate send
}

bool AquaLogicComponent::is_flag_toggled_() const {
    if (pending_key_ == KEY_NONE) {
        return false;
    }
    auto it = key_to_flag_map_.find(pending_key_);
    if (it != key_to_flag_map_.end()) {
        bool current_state = aqua_->GetFlag(it->second);
        return current_state != initial_flag_state_;
    }
    return false;
}

bool AquaLogicComponent::is_key_confirmed_(CONTROLLER_KEYS key) const {
    if (key == KEY_NONE) {
        return false;
    }
    auto it = key_to_flag_map_.find(key);
    if (it != key_to_flag_map_.end()) {
        bool flag_state = aqua_->GetFlag(it->second);
        return flag_state != initial_flag_state_;
    }
    return true;
}

void AquaLogicComponent::handle_key_retry_() {
    if (!waiting_for_confirmation_ || pending_key_ == KEY_NONE) {
        return;
    }

    unsigned long now = millis();
    
    if (now - last_key_send_time_ >= KEY_RETRY_DELAY) {
        if (key_retry_count_ >= MAX_KEY_RETRIES) {
            ESP_LOGW(TAG, "Max retries (%d) reached for key %s", 
                    MAX_KEY_RETRIES, aqua_->GetKeyName(pending_key_));
            clear_pending_key_();
            return;
        }

        if (is_flag_toggled_()) {
            ESP_LOGD(TAG, "Key %s confirmed after %d retries (flag toggled)", 
                    aqua_->GetKeyName(pending_key_), key_retry_count_);
            clear_pending_key_();
            return;
        } else if (key_retry_count_ > 0) {
            auto it = key_to_flag_map_.find(pending_key_);
            if (it == key_to_flag_map_.end()) {
                ESP_LOGD(TAG, "Key %s sent (no flag mapping)", aqua_->GetKeyName(pending_key_));
                clear_pending_key_();
                return;
            }
        }

        ESP_LOGD(TAG, "Retry %d/%d for key %s", 
                key_retry_count_ + 1, MAX_KEY_RETRIES, aqua_->GetKeyName(pending_key_));
        
        last_key_send_time_ = now;

        if (aqua_->CanSend()) {
            // Action 3 = Click (Wired), Action 1 = Press (Wireless)
            uint8_t action = (pending_key_type_ == 0x0002 || pending_key_type_ == 0x0003) ? 3 : 1;
            aqua_->SendCommand(pending_key_type_, pending_key_, action, wired_key_bytes_);
            key_retry_count_++;
        } else {
            ESP_LOGW(TAG, "Cannot retry key, buffer full");
        }
    }
}

void AquaLogicComponent::clear_pending_key_() {
    waiting_for_confirmation_ = false;
    pending_key_ = KEY_NONE;
    pending_key_type_ = FRAME_TYPE_WIRELESS2_KEY_EVENT;
    key_retry_count_ = 0;
    last_key_send_time_ = 0;
    initial_flag_state_ = false;
}

void AquaLogicComponent::send_key(CONTROLLER_KEYS key, uint16_t type) {
    if (waiting_for_confirmation_) {
        ESP_LOGW(TAG, "Cannot send key %s, already waiting for key %s confirmation", 
                aqua_->GetKeyName(key), aqua_->GetKeyName(pending_key_));
        return;
    }
    
    ESP_LOGD(TAG, "Sending Key=%u Name=%s Type=%04x", (unsigned long)key, aqua_->GetKeyName(key), type);
    if (aqua_->CanSend()) {
        // Action 3 = Click for wired frames, Action 1 for wireless
        uint8_t action = (type == 0x0002 || type == 0x0003) ? 3 : 1;
        aqua_->SendCommand(type, key, action, wired_key_bytes_);
    } else {
        ESP_LOGW(TAG, "Cannot send key, buffer full");
    }
}

void AquaLogicComponent::press_key(CONTROLLER_KEYS key, uint16_t type) {
    ESP_LOGD(TAG, "Holding Key=%u Name=%s Type=%04x", (unsigned long)key, aqua_->GetKeyName(key), type);
    held_key_ = key;
    held_key_type_ = type;
}

void AquaLogicComponent::release_key() {
    if (held_key_ != KEY_NONE) {
        ESP_LOGD(TAG, "Releasing Key=%u Name=%s", (unsigned long)held_key_, aqua_->GetKeyName(held_key_));
        // Action 2 = End Hold / Release
        aqua_->SendCommand(held_key_type_, held_key_, 2, wired_key_bytes_);
    }
    held_key_ = KEY_NONE;
}

void AquaLogicComponent::setup() {    
    aqua_ = new AquaLogicProto();
}

void AquaLogicComponent::loop() {
    // 1. Handle state confirmation retries
    handle_key_retry_();

    // 2. Transmit Action 1 continuous press while holding
    if (held_key_ != KEY_NONE) {
        if (aqua_->CanSend()) {
            aqua_->SendCommand(held_key_type_, held_key_, 1, wired_key_bytes_); // 1 = Press / Hold
        }
    }

    // 3. Process incoming serial data
    while (this->available()) {
        size_t bytesRead = aqua_->ReadFrame(*this, frameBuffer_, MAX_MESSAGE_SIZE, frameComplete_);

        if (frameComplete_) {
            struct AQUA_Message newMessage;
            newMessage.length = bytesRead;
            memcpy(newMessage.data, frameBuffer_, bytesRead);
            data_changed_flags_t result = aqua_->ProcessFrame(newMessage.data, newMessage.length);

            if (result && !(result & ERROR)) {
                if (waiting_for_confirmation_ && is_key_confirmed_(pending_key_)) {
                    ESP_LOGD(TAG, "Key %s confirmed after %d retries", 
                            aqua_->GetKeyName(pending_key_), key_retry_count_);
                    clear_pending_key_();
                }

                if (result & DATA_CHANGED) {
                    #ifdef USE_SENSOR

                    if (this->temp_air_)
                        this->temp_air_->publish_state(aqua_->GetTemp(AIR_TEMP));
                    
                    if (this->temp_pool_)
                        this->temp_pool_->publish_state(aqua_->GetTemp(POOL_TEMP));                      

                    if (this->temp_spa_)
                        this->temp_spa_->publish_state(aqua_->GetTemp(SPA_TEMP));                      

                    if (this->pump_speed_)
                        this->pump_speed_->publish_state(aqua_->GetPumpStatus().speed);                      

                    if (this->pump_power_)
                        this->pump_power_->publish_state(aqua_->GetPumpStatus().power);                                                             
 
                    if (this->salt_level_)
                        this->salt_level_->publish_state(aqua_->GetSaltLevel());

                    if (this->pool_chlorine_factor_)
                        this->pool_chlorine_factor_->publish_state(aqua_->GetPct(POOL_CHLORINATOR));

                    if (this->spa_chlorine_factor_)
                        this->spa_chlorine_factor_->publish_state(aqua_->GetPct(SPA_CHLORINATOR));
                    #endif

                    #ifdef USE_TEXT_SENSOR

                    if (this->text_flagsstatus_) {
                        std::string value = "";
                        for (size_t i = 0; i < NUM_FLAGS; i++) {
                            if (aqua_->GetFlag(static_cast<CONTROLLER_FLAGS>(i))) {
                                if (value.length() > 0) {
                                    value.append(",");
                                }
                                value.append(CONTROLLER_FLAG_NAMES[i]);
                            }                            
                        }
                        this->text_flagsstatus_->publish_state(value);   
                    }

                    #endif

                    #ifdef USE_BINARY_SENSOR
                    if (this->binary_filter_)
                        this->binary_filter_->publish_state(aqua_->GetFlag(FILTER));
                    
                    if (this->binary_header_auto_)
                        this->binary_header_auto_->publish_state(aqua_->GetFlag(HEATER_AUTO));

                    if (this->binary_header_1_)
                        this->binary_header_1_->publish_state(aqua_->GetFlag(HEATER_1));

                    if (this->binary_lights_)
                        this->binary_lights_->publish_state(aqua_->GetFlag(LIGHTS));

                    if (this->binary_valve_3_)
                        this->binary_valve_3_->publish_state(aqua_->GetFlag(VALVE_3));

                    if (this->binary_valve_4_)
                        this->binary_valve_4_->publish_state(aqua_->GetFlag(VALVE_4));

                    if (this->binary_check_system_)
                        this->binary_check_system_->publish_state(aqua_->GetFlag(CHECK_SYSTEM));
                    if (this->binary_filter_low_speed_)
                        this->binary_filter_low_speed_->publish_state(aqua_->GetFlag(FILTER_LOW_SPEED));
                    if (this->binary_heater_blinking_)
                        this->binary_heater_blinking_->publish_state(aqua_->GetFlag(HEATER_BLINKING));
                    if (this->binary_check_system_blinking_)
                        this->binary_check_system_blinking_->publish_state(aqua_->GetFlag(CHECK_SYSTEM_BLINKING));
                    if (this->binary_pool_)
                        this->binary_pool_->publish_state(aqua_->GetFlag(POOL));
                    if (this->binary_spa_)
                        this->binary_spa_->publish_state(aqua_->GetFlag(SPA));
                    if (this->binary_service_)
                        this->binary_service_->publish_state(aqua_->GetFlag(SERVICE));
                    if (this->binary_spillover_)
                        this->binary_spillover_->publish_state(aqua_->GetFlag(SPILLOVER));
                    if (this->binary_system_off_)
                        this->binary_system_off_->publish_state(aqua_->GetFlag(SYSTEM_OFF));
                    if (this->binary_super_chlorinate_)
                        this->binary_super_chlorinate_->publish_state(aqua_->GetFlag(SUPER_CHLORINATE));
                    if (this->binary_is_metric_)
                        this->binary_is_metric_->publish_state(aqua_->GetFlag(IS_METRIC));
                    for (size_t i = 0; i < 14; i++) {
                        if (this->binary_aux_[i]) {
                            this->binary_aux_[i]->publish_state(aqua_->GetFlag(AUX_FLAGS[i]));
                        }
                    }
                    #endif
                }

                if (result & DISPLAY_CHANGED) {
                    #ifdef USE_TEXT_SENSOR
                    struct display_state_t display = aqua_->GetDisplay();

                    if (this->text_display1_) {
                        std::string formatted = format_display_line(display.raw_line1.c_str(), display.line1_blink_state);
                        this->text_display1_->publish_state(formatted);   
                    }                        

                    if (this->text_display2_) {
                        std::string formatted = format_display_line(display.raw_line2.c_str(), display.line2_blink_state);
                        this->text_display2_->publish_state(formatted);   
                    }
                    #endif
                }
            }            
        }
    }
}

void AquaLogicComponent::dump_config(){
    ESP_LOGCONFIG(TAG, "Empty UART component");
    #ifdef USE_SENSOR
        LOG_SENSOR("  ", "Temp Pool:", this->temp_pool_);
        LOG_SENSOR("  ", "Temp Air:", this->temp_air_);
        LOG_SENSOR("  ", "Temp Spa:", this->temp_spa_);
    #endif
}

}  // namespace aqualogic
}  // namespace esphome