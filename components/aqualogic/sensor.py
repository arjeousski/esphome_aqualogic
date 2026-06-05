import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_MOISTURE,
    DEVICE_CLASS_POWER_FACTOR,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_SPEED,
    DEVICE_CLASS_POWER,
    STATE_CLASS_MEASUREMENT,
    UNIT_PARTS_PER_MILLION,
    UNIT_PERCENT,
    UNIT_WATT,

)
from . import AqualogicComponent

UNIT_FAHRENHEIT = "°F"

DEPENDENCIES = ["aqualogic"]

CONF_AQUALOGIC = "aqualogic"
CONF_TEMP_AIR = "air_temp"
CONF_TEMP_POOL = "pool_temp"
CONF_TEMP_SPA = "spa_temp"
CONF_PUMP_SPEED = "pump_speed"
CONF_PUMP_POWER = "pump_power"
CONF_SALT_LEVEL = "salt_level"
CONF_POOL_CHLORINE_FACTOR = "pool_chlorine_factor"
CONF_SPA_CHLORINE_FACTOR = "spa_chlorine_factor"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_AQUALOGIC): cv.use_id(AqualogicComponent),
        cv.Optional(CONF_TEMP_AIR): sensor.sensor_schema(
            unit_of_measurement=UNIT_FAHRENHEIT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMP_POOL): sensor.sensor_schema(
            unit_of_measurement=UNIT_FAHRENHEIT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMP_SPA): sensor.sensor_schema(
            unit_of_measurement=UNIT_FAHRENHEIT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_PUMP_SPEED): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_PUMP_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_SALT_LEVEL): sensor.sensor_schema(
            unit_of_measurement=UNIT_PARTS_PER_MILLION,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_MOISTURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_POOL_CHLORINE_FACTOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER_FACTOR,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_SPA_CHLORINE_FACTOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER_FACTOR,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)
async def setup_sensor(config, key, setter):
    """setting up sensor"""
    if key not in config:
        return None
    var = await sensor.new_sensor(config[key])
    cg.add(setter(var))
    return var


async def setup_input(config, key, setter):
    """setting up input"""
    if key not in config:
        return None
    var = await cg.get_variable(config[key])
    cg.add(setter(var))
    return var


# code generation entry point
async def to_code(config):
    """Code generation entry point"""
    server = await cg.get_variable(config[CONF_AQUALOGIC])

    # exposed sensors   
    await setup_sensor(config, CONF_TEMP_AIR, server.set_temp_air)
    await setup_sensor(config, CONF_TEMP_POOL, server.set_temp_pool)
    await setup_sensor(config, CONF_TEMP_SPA, server.set_temp_spa)
    await setup_sensor(config, CONF_PUMP_SPEED, server.set_pump_speed)
    await setup_sensor(config, CONF_PUMP_POWER, server.set_pump_power)
    await setup_sensor(config, CONF_SALT_LEVEL, server.set_salt_level)
    await setup_sensor(config, CONF_POOL_CHLORINE_FACTOR, server.set_pool_chlorine_factor)
    await setup_sensor(config, CONF_SPA_CHLORINE_FACTOR, server.set_spa_chlorine_factor)