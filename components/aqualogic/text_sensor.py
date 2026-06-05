import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from . import AqualogicComponent


DEPENDENCIES = ["aqualogic"]
CONF_AQUALOGIC = "aqualogic"
CONF_DISPLAY_LINE1 = "display1"
CONF_DISPLAY_LINE2 = "display2"
CONF_FLAGS_STATUS = "flags_status"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_AQUALOGIC): cv.use_id(AqualogicComponent),
        cv.Optional(CONF_DISPLAY_LINE1): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_DISPLAY_LINE2): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_FLAGS_STATUS): text_sensor.text_sensor_schema(),
    }
)

async def setup_sensor(config, key, setter):
    """setting up sensor"""
    if key not in config:
        return None
    var = await text_sensor.new_text_sensor(config[key])
    cg.add(setter(var))
    return var


# code generation entry point
async def to_code(config):
    """Code generation entry point"""
    server = await cg.get_variable(config[CONF_AQUALOGIC])

    # exposed sensors   
    await setup_sensor(config, CONF_DISPLAY_LINE1, server.set_text_display1)
    await setup_sensor(config, CONF_DISPLAY_LINE2, server.set_text_display2)
    await setup_sensor(config, CONF_FLAGS_STATUS, server.set_text_flags_status)

