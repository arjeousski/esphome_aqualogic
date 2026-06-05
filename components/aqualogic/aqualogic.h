#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif


#include "driver/uart.h"
#include "soc/uart_reg.h"
#include "esphome/core/log.h"
#include "AquaLogicProto.h"
#include <unordered_map>

// Allocate enough space for start/data/crc/end
//                            2     61   2   2
#define MAX_MESSAGE_SIZE 70
#define KEY_RETRY_DELAY 500  // ms between retries
#define MAX_KEY_RETRIES 3    // maximum number of retry attempts

namespace esphome
{
    namespace aqualogic
    {
        #ifdef USE_SENSOR
        using esphome::sensor::Sensor;
        #endif

        
        #ifdef USE_TEXT_SENSOR
        using esphome::text_sensor::TextSensor;
        #endif

        #ifdef USE_BINARY_SENSOR
        using esphome::binary_sensor::BinarySensor;
        #endif

        struct AQUA_Message
        {
            size_t length;
            unsigned char data[MAX_MESSAGE_SIZE];
        };

        class AquaLogicComponent : public uart::UARTDevice, public Component
        {
        public:
            void setup() override;
            void loop() override;
            void dump_config() override;
            void send_key(CONTROLLER_KEYS key);
            void send_key_with_retry(CONTROLLER_KEYS key);

            #ifdef USE_SENSOR
            void set_temp_pool(Sensor *sensor) { this->temp_pool_ = sensor; }
            void set_temp_air(Sensor *sensor) { this->temp_air_ = sensor; }
            void set_temp_spa(Sensor *sensor) { this->temp_spa_ = sensor; }
            void set_pump_speed(Sensor *sensor) { this->pump_speed_ = sensor; }
            void set_pump_power(Sensor *sensor) { this->pump_power_ = sensor; }
            void set_salt_level(Sensor *sensor) { this->salt_level_ = sensor; }
            void set_pool_chlorine_factor(Sensor *sensor) { this->pool_chlorine_factor_ = sensor; }
            void set_spa_chlorine_factor(Sensor *sensor) { this->spa_chlorine_factor_ = sensor; }



            #endif

            #ifdef USE_TEXT_SENSOR
            void set_text_display1(TextSensor *sensor) { this->text_display1_ = sensor; }
            void set_text_display2(TextSensor *sensor) { this->text_display2_ = sensor; }
            void set_text_flags_status(TextSensor *sensor) { this->text_flagsstatus_ = sensor; }
            #endif


            #ifdef USE_BINARY_SENSOR
            void set_binary_filter(BinarySensor *sensor) { this->binary_filter_ = sensor; }
            void set_binary_heater_auto(BinarySensor *sensor) { this->binary_header_auto_ = sensor; }
            void set_binary_heater_1(BinarySensor *sensor) { this->binary_header_1_ = sensor; }
            void set_binary_lights(BinarySensor *sensor) { this->binary_lights_ = sensor; }
            void set_binary_valve_3(BinarySensor *sensor) { this->binary_valve_3_ = sensor; }
            void set_binary_valve_4(BinarySensor *sensor) { this->binary_valve_4_ = sensor; }
            void set_binary_check_system(BinarySensor *sensor) { this->binary_check_system_ = sensor; }

            #endif

        protected:
            HighFrequencyLoopRequester high_freq_;

            // exposed sensors
            #ifdef USE_SENSOR
            Sensor *temp_pool_{nullptr};
            Sensor *temp_air_{nullptr};
            Sensor *temp_spa_{nullptr};
            Sensor *pump_speed_{nullptr};
            Sensor *pump_power_{nullptr};
            Sensor *salt_level_{nullptr};
            Sensor *pool_chlorine_factor_{nullptr};
            Sensor *spa_chlorine_factor_{nullptr};
            #endif

            #ifdef USE_TEXT_SENSOR
            TextSensor *text_display1_{nullptr};
            TextSensor *text_display2_{nullptr};
            TextSensor *text_flagsstatus_{nullptr};
            #endif

            #ifdef USE_BINARY_SENSOR
            BinarySensor *binary_filter_{nullptr};
            BinarySensor *binary_header_auto_{nullptr};
            BinarySensor *binary_header_1_{nullptr};
            BinarySensor *binary_lights_{nullptr};
            BinarySensor *binary_valve_3_{nullptr};
            BinarySensor *binary_valve_4_{nullptr};
            BinarySensor *binary_check_system_{nullptr};
            #endif
            // Local variables
            AquaLogicProto *aqua_{nullptr};
            
            unsigned char frameBuffer_[MAX_MESSAGE_SIZE];
            bool frameComplete_ = false;
            bool _uartConfigured = false;

            // Key retry state
            CONTROLLER_KEYS pending_key_ = KEY_NONE;
            unsigned long last_key_send_time_ = 0;
            int key_retry_count_ = 0;
            bool waiting_for_confirmation_ = false;
            bool initial_flag_state_ = false;

            // Key to flag mapping for confirmation
            static const std::unordered_map<CONTROLLER_KEYS, CONTROLLER_FLAGS> key_to_flag_map_;

            // Key retry methods
            bool is_key_confirmed_(CONTROLLER_KEYS key) const;
            bool is_flag_toggled_() const;
            void handle_key_retry_();
            void clear_pending_key_();
        };

        // Key to flag mapping definition moved to implementation file

    } // namespace aqualogic
} // namespace esphome