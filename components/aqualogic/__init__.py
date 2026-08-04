import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
import logging
from esphome import automation


_LOGGER = logging.getLogger(__name__)


MULTI_CONF = True

CODEOWNERS = ["@arjeousski"]
DEPENDENCIES = ['uart']

CONF_KEY = "key"
CONF_TYPE = "type"
FRAME_TYPES = {
    "LOCAL_WIRED": 0x0002,
    "REMOTE_WIRED": 0x0003,
    "WIRELESS": 0x0083,
    "WIRELESS2": 0x008c,
}

ns = cg.global_ns
aqualogic_component_ns = cg.esphome_ns.namespace('aqualogic')
AquaLogicSendAction = aqualogic_component_ns.class_("AquaLogicSendAction", automation.Action)
AquaLogicToggleAction = aqualogic_component_ns.class_("AquaLogicToggleAction", automation.Action)
AquaLogicPressAction = aqualogic_component_ns.class_("AquaLogicPressAction", automation.Action)
AquaLogicReleaseAction = aqualogic_component_ns.class_("AquaLogicReleaseAction", automation.Action)
AqualogicComponent = aqualogic_component_ns.class_('AquaLogicComponent', cg.Component, uart.UARTDevice)

AquaLogicControllerKeys = aqualogic_component_ns.enum("CONTROLLER_KEYS")
AquaLogicKeys = {
    "RIGHT": AquaLogicControllerKeys.KEY_RIGHT,
    "MENU": AquaLogicControllerKeys.KEY_MENU,
    "LEFT": AquaLogicControllerKeys.KEY_LEFT,
    "SERVICE": AquaLogicControllerKeys.KEY_SERVICE,
    "MINUS": AquaLogicControllerKeys.KEY_MINUS,
    "PLUS" : AquaLogicControllerKeys.KEY_PLUS,
    "POOL_SPA": AquaLogicControllerKeys.KEY_POOL_SPA,
    "FILTER" : AquaLogicControllerKeys.KEY_FILTER,
    "LIGHTS" : AquaLogicControllerKeys.KEY_LIGHTS,
    "AUX_1" : AquaLogicControllerKeys.KEY_AUX_1,
    "AUX_2" : AquaLogicControllerKeys.KEY_AUX_2,
    "AUX_3" : AquaLogicControllerKeys.KEY_AUX_3,
    "AUX_4" : AquaLogicControllerKeys.KEY_AUX_4,
    "AUX_5" : AquaLogicControllerKeys.KEY_AUX_5,
    "AUX_6" : AquaLogicControllerKeys.KEY_AUX_6,
    "AUX_7" : AquaLogicControllerKeys.KEY_AUX_7,    
    "VALVE_3" : AquaLogicControllerKeys.KEY_VALVE_3,
    "VALVE_4" : AquaLogicControllerKeys.KEY_VALVE_4,
    "HEATER_1" : AquaLogicControllerKeys.KEY_HEATER_1,
    "AUX_8" : AquaLogicControllerKeys.KEY_AUX_8,    
    "AUX_9" : AquaLogicControllerKeys.KEY_AUX_9,    
    "AUX_10" : AquaLogicControllerKeys.KEY_AUX_10,    
    "AUX_11" : AquaLogicControllerKeys.KEY_AUX_11,    
    "AUX_12" : AquaLogicControllerKeys.KEY_AUX_12,    
    "AUX_13" : AquaLogicControllerKeys.KEY_AUX_13,    
    "AUX_14" : AquaLogicControllerKeys.KEY_AUX_14,    
    "UNLOCK" : AquaLogicControllerKeys.KEY_UNLOCK,
}

CONF_WIRED_KEY_BYTES = "wired_key_bytes"

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(AqualogicComponent),
        cv.Optional(CONF_WIRED_KEY_BYTES, default=4): cv.int_range(min=2, max=4),
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_wired_key_bytes(config[CONF_WIRED_KEY_BYTES]))
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)

@automation.register_action(
    "aqualogic.send",
    AquaLogicSendAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(AqualogicComponent),
            cv.Required(CONF_KEY): cv.templatable(cv.enum(
                AquaLogicKeys, upper=True
            )),
            cv.Optional(CONF_TYPE, default="WIRELESS2"): cv.templatable(cv.enum(
                FRAME_TYPES, upper=True
            )),
        },
    ),
    synchronous=True,
)
async def send_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_key = await cg.templatable(config[CONF_KEY], args, cg.uint32)
    cg.add(var.set_key(template_key))
    template_type = await cg.templatable(config[CONF_TYPE], args, cg.uint16)
    cg.add(var.set_type(template_type))
    return var


@automation.register_action(
    "aqualogic.toggle",
    AquaLogicToggleAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(AqualogicComponent),
            cv.Required(CONF_KEY): cv.templatable(cv.enum(
                AquaLogicKeys, upper=True
            )),
            cv.Optional(CONF_TYPE, default="WIRELESS2"): cv.templatable(cv.enum(
                FRAME_TYPES, upper=True
            )),
        },
    ),
    synchronous=True,
)
async def toggle_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_key = await cg.templatable(config[CONF_KEY], args, cg.uint32)
    cg.add(var.set_key(template_key))
    template_type = await cg.templatable(config[CONF_TYPE], args, cg.uint16)
    cg.add(var.set_type(template_type))
    return var


@automation.register_action(
    "aqualogic.press",
    AquaLogicPressAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(AqualogicComponent),
            cv.Required(CONF_KEY): cv.templatable(cv.enum(
                AquaLogicKeys, upper=True
            )),
            cv.Optional(CONF_TYPE, default="WIRELESS2"): cv.templatable(cv.enum(
                FRAME_TYPES, upper=True
            )),
        },
    ),
    synchronous=True,
)
async def press_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_key = await cg.templatable(config[CONF_KEY], args, cg.uint32)
    cg.add(var.set_key(template_key))
    template_type = await cg.templatable(config[CONF_TYPE], args, cg.uint16)
    cg.add(var.set_type(template_type))
    return var


@automation.register_action(
    "aqualogic.release",
    AquaLogicReleaseAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(AqualogicComponent),
        },
    ),
    synchronous=True,
)
async def release_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var
