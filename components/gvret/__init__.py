import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.const import (
    CONF_ID,
)
from esphome.components.esp32_can.canbus import ESP32Can
from esphome.components.canbus import (
    CONF_CANBUS_ID,
    CanbusComponent,
)
from esphome.core import coroutine_with_priority

DEPENDENCIES = ["canbus"]

CONF_CAN = "can"

gvert_ns = cg.esphome_ns.namespace("gvert")
GVERTComponent = gvert_ns.class_("GVERTComponent", cg.Component)


CONFIG_SCHEMA = cv.ensure_list({
    cv.GenerateID(): cv.declare_id(GVERTComponent),
    cv.Required(CONF_CAN): cv.ensure_list({
        cv.GenerateID(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
    }),
})


@coroutine_with_priority(45.0)
async def to_code(configs):
    for config in configs:
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)
        
        for canbus_config in config[CONF_CAN]:
            canbus = await cg.get_variable(canbus_config[CONF_CANBUS_ID])
            cg.add(var.add_canbus(canbus))

