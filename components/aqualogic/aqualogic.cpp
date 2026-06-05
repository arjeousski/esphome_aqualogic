#include "aqualogic.h"
#include <ArduinoJson.h>

namespace esphome {
namespace aqualogic {

static const char *TAG = "aqualogic.component";

// Key to flag mapping definition
const std::unordered_map<CONTROLLER_KEYS, CONTROLLER_FLAGS> AquaLogicComponent::key_to_flag_map_ = {
    {KEY_FILTER, FILTER},
    {KEY_LIGHTS, LIGHTS},
    {KEY_VALVE_3, VALVE_3},
    {KEY_VALVE_4, VALVE_4},
    {KEY_HEATER_1, HEATER_AUTO}
};

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

// New key retry functionality
void AquaLogicComponent::send_key_with_retry(CONTROLLER_KEYS key) {
    // First check if this key is in our map
    auto it = key_to_flag_map_.find(key);
    if (it == key_to_flag_map_.end()) {
        ESP_LOGW(TAG, "Key %s not in key_to_flag_map_, not using retry logic", aqua_->GetKeyName(key));
        // Fall back to direct key send without retry
        send_key(key);
        return;
    }

    if (waiting_for_confirmation_) {
        ESP_LOGW(TAG, "Already waiting for key confirmation, ignoring new key");
        return;
    }

    // If we get here, the key is in our map and we can use the retry logic
    initial_flag_state_ = aqua_->GetFlag(it->second);
    ESP_LOGD(TAG, "Initial flag state for %s: %s", 
            aqua_->GetKeyName(key), initial_flag_state_ ? "true" : "false");

    pending_key_ = key;
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
        // For keys with flag mapping, check if the flag has toggled from initial state
        return flag_state != initial_flag_state_;
    }
    // If key doesn't have a corresponding flag, consider it confirmed immediately
    return true;
}

void AquaLogicComponent::handle_key_retry_() {
    if (!waiting_for_confirmation_ || pending_key_ == KEY_NONE) {
        return;
    }

    unsigned long now = millis();
    
    // Check if it's time to send/retry
    if (now - last_key_send_time_ >= KEY_RETRY_DELAY) {
        if (key_retry_count_ >= MAX_KEY_RETRIES) {
            ESP_LOGW(TAG, "Max retries (%d) reached for key %s", 
                    MAX_KEY_RETRIES, aqua_->GetKeyName(pending_key_));
            clear_pending_key_();
            return;
        }

        // Check if the flag has toggled or if this is an unmapped key that was sent once
        if (is_flag_toggled_()) {
            ESP_LOGD(TAG, "Key %s confirmed after %d retries (flag toggled)", 
                    aqua_->GetKeyName(pending_key_), key_retry_count_);
            clear_pending_key_();
            return;
        } else if (key_retry_count_ > 0) {
            // For keys without flag mapping, consider confirmed after first send
            auto it = key_to_flag_map_.find(pending_key_);
            if (it == key_to_flag_map_.end()) {
                ESP_LOGD(TAG, "Key %s sent (no flag mapping)", 
                        aqua_->GetKeyName(pending_key_));
                clear_pending_key_();
                return;
            }
        }

        // Send the key again
        ESP_LOGD(TAG, "Retry %d/%d for key %s", 
                key_retry_count_ + 1, MAX_KEY_RETRIES, 
                aqua_->GetKeyName(pending_key_));
        
        // Even if we cannot send, increment the timer so that we don't send too often
        last_key_send_time_ = now;

        if (aqua_->CanSend()) {
            aqua_->SendCommand(FRAME_TYPE_WIRELESS2_KEY_EVENT, pending_key_);
            key_retry_count_++;
        } else {
            ESP_LOGW(TAG, "Cannot retry key, buffer full");
        }
    }
}

void AquaLogicComponent::clear_pending_key_() {
    waiting_for_confirmation_ = false;
    pending_key_ = KEY_NONE;
    key_retry_count_ = 0;
    last_key_send_time_ = 0;
    initial_flag_state_ = false;
}

void AquaLogicComponent::send_key(CONTROLLER_KEYS key) {
    if (waiting_for_confirmation_) {
        ESP_LOGW(TAG, "Cannot send key %s, already waiting for key %s confirmation", 
                aqua_->GetKeyName(key), aqua_->GetKeyName(pending_key_));
        return;
    }
    
    ESP_LOGD(TAG, "Sending Key=%u Name=%s", key, aqua_->GetKeyName(key));
    if (aqua_->CanSend()) {
        aqua_->SendCommand(FRAME_TYPE_WIRELESS2_KEY_EVENT, key);
    } else {
        ESP_LOGW(TAG, "Cannot send key, buffer full");
    }
}

void AquaLogicComponent::setup() {    
    // Tweak the hardware FIFO thresholds for immediate frame retrieval
    this->set_rx_full_threshold(8);
    this->set_rx_timeout(1);

    aqua_ = new AquaLogicProto();
}

void AquaLogicComponent::loop() {
    // Handle pending key retries
    handle_key_retry_();

    while(this->available()) {
        size_t bytesRead = aqua_->ReadFrame(*this, frameBuffer_, MAX_MESSAGE_SIZE, frameComplete_);

        if (frameComplete_)
        {
            struct AQUA_Message newMessage;
            newMessage.length = bytesRead;
            memcpy(newMessage.data, frameBuffer_, bytesRead);
            data_changed_flags_t result = aqua_->ProcessFrame(newMessage.data, newMessage.length);

            if (result && !(result & ERROR)) 
            {
                // If we're waiting for confirmation, check if the key was confirmed
                if (waiting_for_confirmation_ && is_key_confirmed_(pending_key_)) {
                    ESP_LOGD(TAG, "Key %s confirmed after %d retries", 
                            aqua_->GetKeyName(pending_key_), key_retry_count_);
                    clear_pending_key_();
                }
                if (result & DATA_CHANGED) {
                    #ifdef USE_SENSOR

                    if (this->temp_air_)
                        this->temp_air_->publish_state(aqua_->GetTemp(AIR_TEMP));
                    
                    if (this->temp_air_)
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
                    // Status Json
                    /*if (this->text_status_) {
                        DynamicJsonDocument status_doc(1024);

                        struct packet_stats_t stats = this->aqua_->GetStats();

                        status_doc["num_packets"] = stats.num_packets;
                        status_doc["num_crc"] = stats.num_crc;
                        status_doc["num_timeouts"] = stats.num_timeouts;
                        status_doc["num_bytes_received"] = stats.num_bytes_received;
                        status_doc["num_bytes_used"] = stats.num_bytes_used;
                        status_doc["last_packet_received_ms"] = millis() - stats.last_packet_received_ms;

                        String s;
                        serializeJson(status_doc, s);
                        ESP_LOGD(TAG, "Stats: %s",s.c_str());
                    }*/

                    if (this->text_flagsstatus_) {
                        std::string value = "";
                        for (size_t i = 0; i < NUM_FLAGS; i++)
                        {
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
        LOG_SENSOR("  ", "Temp Air:", this->temp_spa_);
    #endif
}


}  // namespace aqualogic
}  // namespace esphome