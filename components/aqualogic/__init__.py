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

ns = cg.global_ns
aqualogic_component_ns = cg.esphome_ns.namespace('aqualogic')
AquaLogicSendAction = aqualogic_component_ns.class_("AquaLogicSendAction", automation.Action)
AquaLogicToggleAction = aqualogic_component_ns.class_("AquaLogicToggleAction", automation.Action)
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
}

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(AqualogicComponent)
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
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
        },
    ),
    synchronous=True,
)
async def send_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_KEY], args, cg.uint32)
    cg.add(var.set_key(template_))
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
        },
    ),
    synchronous=True,
)
async def toggle_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_KEY], args, cg.uint32)
    cg.add(var.set_key(template_))
    return var
