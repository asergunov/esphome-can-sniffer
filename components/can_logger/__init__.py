import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.const import (
    CONF_ID,
    CONF_FILE,
    CONF_PATH,
    CONF_FORMAT,
    CONF_PLATFORM,
)
from esphome.components.sd_mmc_card import (
    CONF_SD_MMC_CARD_ID,
    SdCard,
)
from esphome.components.esp32_can.canbus import ESP32Can
from esphome.components.canbus import (
    CONF_CANBUS_ID,
    CanbusComponent,
)
from esphome.core import coroutine_with_priority

DEPENDENCIES = ["sd_mmc_card", "canbus"]

CONF_CAN = "can"
CONF_LOG_ID = "log_id"
CONF_USE_ESP32_CAN = "use_esp32_can"

FORMAT_CRTD = "crtd"

can_logger_ns = cg.esphome_ns.namespace("can_logger")
CanLogger = can_logger_ns.class_("CanLogger", cg.Component)
Format = can_logger_ns.enum("Format", is_class=True)


def _validate_file(config):
    return config


def _validate_path(value):
    return cv.string_strict(value)


CONFIG_SCHEMA = cv.ensure_list({
    cv.GenerateID(): cv.declare_id(CanLogger),
    cv.Required(CONF_CAN): cv.ensure_list({
        cv.GenerateID(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
        cv.Required(CONF_LOG_ID): cv.int_,
    }),
    cv.Required(CONF_FILE): cv.Schema({
        cv.GenerateID(CONF_SD_MMC_CARD_ID): cv.use_id(SdCard),
        cv.Required(CONF_PATH): _validate_path,
        cv.Optional(CONF_FORMAT, FORMAT_CRTD): cv.enum({
            FORMAT_CRTD: Format.CRTD,
        }),
    }, _validate_file)
})


def _final_validate(configs):
    if canbus_configs := fv.full_config.get().get("canbus"):
        for config in configs:
            for canbus_config in config[CONF_CAN]:
                canbus_id = canbus_config[CONF_CANBUS_ID]
                for canbus_conf in canbus_configs:
                    if canbus_conf[CONF_ID] != canbus_id:
                        continue
                    if canbus_conf[CONF_PLATFORM] == "esp32_can":
                        config[CONF_USE_ESP32_CAN] = True
    return configs


FINAL_VALIDATE_SCHEMA = _final_validate

@coroutine_with_priority(45.0)
async def to_code(configs):
    for config in configs:
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)
        sdcard = await cg.get_variable(config[CONF_FILE][CONF_SD_MMC_CARD_ID])
        cg.add(var.set_sd_card(sdcard))

        path = config[CONF_FILE][CONF_PATH]
        format_ = config[CONF_FILE][CONF_FORMAT]
        cg.add(var.set_file(path, format_))

        for canbus_config in config[CONF_CAN]:
            canbus = await cg.get_variable(canbus_config[CONF_CANBUS_ID])
            cg.add(var.add_canbus(canbus, canbus_config[CONF_LOG_ID]))

        if config.get(CONF_USE_ESP32_CAN):
            cg.add_define("USE_ESP32_CAN")
