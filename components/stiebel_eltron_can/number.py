import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_UPDATE_INTERVAL
from . import stiebel_eltron_can_ns, StiebelEltronCanComponent, ELSTER_TYPES, CAN_MEMBERS

DEPENDENCIES = ['stiebel_eltron_can']
StiebelEltronCanNumber = stiebel_eltron_can_ns.class_('StiebelEltronCanNumber', number.Number, cg.PollingComponent)

CONFIG_SCHEMA = number.number_schema(StiebelEltronCanNumber).extend({
    cv.GenerateID('stiebel_eltron_can_id'): cv.use_id(StiebelEltronCanComponent),
    cv.Required('elster_index'): cv.hex_int,
    cv.Optional('elster_type', default='DEFAULT'): cv.enum(ELSTER_TYPES, upper=True, space="_"),
    cv.Required('min_value'): cv.float_,
    cv.Required('max_value'): cv.float_,
    cv.Required('target'): cv.enum(CAN_MEMBERS, upper=True, space="_"),
    cv.Optional(CONF_UPDATE_INTERVAL, default="30s"): cv.update_interval,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = await number.new_number(
        config,
        min_value = config['min_value'],
        max_value = config['max_value'],
        step = 1.0,
    )
    await cg.register_component(var, config)

    parent = await cg.get_variable(config['stiebel_eltron_can_id'])
    cg.add(var.set_parent(parent))
    cg.add(var.set_elster_index(config['elster_index']))
    cg.add(var.set_elster_type(config['elster_type']))
    cg.add(var.set_target(config['target']))

    cg.add(parent.register_number(var))