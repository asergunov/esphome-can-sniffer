import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
)
from esphome.components.canbus import (
    CONF_CANBUS_ID,
    CanbusComponent,
)
from esphome.core import coroutine_with_priority
from esphome.components import time as time_

DEPENDENCIES = ["canbus"]

CONF_CAN = "can"

gvret_ns = cg.esphome_ns.namespace("gvret")
GVERTComponent = gvret_ns.class_("GVERTComponent", cg.Component)


CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(GVERTComponent),
    cv.Optional(CONF_TIME_ID): cv.use_id(time_.RealTimeClock),
    cv.Required(CONF_CAN): cv.ensure_list({
        cv.GenerateID(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
    }),
})


@coroutine_with_priority(45.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if rtc := config.get(CONF_TIME_ID):
        rtcComponent = await cg.get_variable(rtc)
        cg.add(var.set_real_time_clock(rtcComponent))

    for canbus_config in config[CONF_CAN]:
        canbus = await cg.get_variable(canbus_config[CONF_CANBUS_ID])
        cg.add(var.add_canbus(canbus))
