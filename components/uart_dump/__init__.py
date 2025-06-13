import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
)
from esphome.components.uart import (
    CONF_UART_ID,
    UART_DEVICE_SCHEMA,
)
from esphome.core import coroutine_with_priority

DEPENDENCIES = ["uart"]

uart_dump_ns = cg.esphome_ns.namespace("uart_dump")
UARTDump = uart_dump_ns.class_("UARTDump", cg.Component)

CONFIG_SCHEMA = cv.ensure_list(UART_DEVICE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(UARTDump),
}))


async def to_code(configs):
    for config in configs:
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)
        uart = await cg.get_variable(config[CONF_UART_ID])
        cg.add(var.set_uart(uart))
