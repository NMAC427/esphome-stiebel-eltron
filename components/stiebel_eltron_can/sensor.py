import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_UPDATE_INTERVAL, CONF_ACCURACY_DECIMALS
from . import stiebel_eltron_can_ns, StiebelEltronCanComponent, ELSTER_TYPES, CAN_MEMBERS

DEPENDENCIES = ['stiebel_eltron_can']
StiebelEltronCanSensor = stiebel_eltron_can_ns.class_('StiebelEltronCanSensor', sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = sensor.sensor_schema(StiebelEltronCanSensor).extend({
    cv.GenerateID('stiebel_eltron_can_id'): cv.use_id(StiebelEltronCanComponent),
    cv.Required('elster_index'): cv.hex_int,
    cv.Optional('elster_type', default='DEFAULT'): cv.enum(ELSTER_TYPES, upper=True, space="_"),
    cv.Optional('target'): cv.enum(CAN_MEMBERS, upper=True, space="_"),
    cv.Optional(CONF_UPDATE_INTERVAL, default="30s"): cv.update_interval,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config['stiebel_eltron_can_id'])
    cg.add(var.set_parent(parent))
    cg.add(var.set_elster_index(config['elster_index']))
    cg.add(var.set_elster_type(config['elster_type']))
    cg.add(var.set_target(config['target']))

    if CONF_ACCURACY_DECIMALS not in config:
        cg.add(var.auto_accuracy_decimals())

    cg.add(parent.register_sensor(var))