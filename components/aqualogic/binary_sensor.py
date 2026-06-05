import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID
from . import AqualogicComponent
from esphome.const import (
    DEVICE_CLASS_CONNECTIVITY,
    ENTITY_CATEGORY_DIAGNOSTIC,
)


DEPENDENCIES = ["aqualogic"]
CONF_AQUALOGIC = "aqualogic"

CONF_SENSOR_FILTER = "filter"
CONF_SENSOR_HEATER_AUTO = "heater_auto"
CONF_SENSOR_HEATER = "heater_1"
CONF_SENSOR_LIGHTS = "lights"
CONF_SENSOR_VALVE_3 = "valve_3"
CONF_SENSOR_VALVE_4 = "valve_4"
CONF_SENSOR_CHECK_SYSTEM = "check_system"


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_AQUALOGIC): cv.use_id(AqualogicComponent),
        cv.Optional(CONF_SENSOR_FILTER): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_SENSOR_HEATER_AUTO): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_SENSOR_HEATER): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_SENSOR_LIGHTS): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_SENSOR_VALVE_3): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_SENSOR_VALVE_4): binary_sensor.binary_sensor_schema(),
        cv.Optional(CONF_SENSOR_CHECK_SYSTEM): binary_sensor.binary_sensor_schema(),

    }
)

async def setup_sensor(config, key, setter):
    """setting up sensor"""
    if key not in config:
        return None
    var = await binary_sensor.new_binary_sensor(config[key])
    cg.add(setter(var))
    return var


# code generation entry point
async def to_code(config):
    """Code generation entry point"""
    server = await cg.get_variable(config[CONF_AQUALOGIC])

    # exposed sensors   
    await setup_sensor(config, CONF_SENSOR_FILTER, server.set_binary_filter)
    await setup_sensor(config, CONF_SENSOR_HEATER_AUTO, server.set_binary_heater_auto)
    await setup_sensor(config, CONF_SENSOR_HEATER, server.set_binary_heater_1)   
    await setup_sensor(config, CONF_SENSOR_LIGHTS, server.set_binary_lights)
    await setup_sensor(config, CONF_SENSOR_VALVE_3, server.set_binary_valve_3)
    await setup_sensor(config, CONF_SENSOR_VALVE_4, server.set_binary_valve_4)
    await setup_sensor(config, CONF_SENSOR_CHECK_SYSTEM, server.set_binary_check_system)


