import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import canbus

DEPENDENCIES = ['canbus']
CODEOWNERS = ['@ncam427']

stiebel_eltron_can_ns = cg.esphome_ns.namespace('stiebel_eltron_can')

Type = stiebel_eltron_can_ns.enum('ElsterType')
CanMember = stiebel_eltron_can_ns.namespace('can_members')

ELSTER_TYPES = {
    'DEFAULT': Type.et_default,
    'DEC': Type.et_dec_val,
    'CENT': Type.et_cent_val,
    'MIL': Type.et_mil_val,
    'DOUBLE': Type.et_double_val,
    'TRIPLE': Type.et_triple_val,
    'INV_DOUBLE': Type.et_inv_double_val,
    'INV_TRIPLE': Type.et_inv_triple_val,
}

CAN_MEMBERS = {
    'KESSEL': CanMember.Kessel,
    'WPM': CanMember.WPM,
    'MANAGER': CanMember.Manager,
    'HK1': CanMember.HK1,
    'MFG': CanMember.MFG,
    'WWTEMP': CanMember.WWTEMP,
}

StiebelEltronCanComponent = stiebel_eltron_can_ns.class_('StiebelEltronCanComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(StiebelEltronCanComponent),
    cv.Required('canbus_id'): cv.use_id(canbus.CanbusComponent),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    canbus_var = await cg.get_variable(config['canbus_id'])
    cg.add(var.set_canbus(canbus_var))