#!/usr/bin/env python
from __future__ import print_function
from collections import OrderedDict
import getopt
import sys
import yaml

class quoted(str): pass

def quoted_presenter(dumper, data):
    return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='"')
yaml.add_representer(quoted, quoted_presenter)

class literal(str): pass

def literal_presenter(dumper, data):
    return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
yaml.add_representer(literal, literal_presenter)

def ordered_dict_presenter(dumper, data):
    return dumper.represent_dict(data.items())
yaml.add_representer(OrderedDict, ordered_dict_presenter)

def dict_constructor(loader, node):
    return OrderedDict(loader.construct_pairs(node))
_mapping_tag = yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG
yaml.add_constructor(_mapping_tag, dict_constructor)

class IntField(object):
    def __init__(self, name, position, length, signed):
        self.name = name
        self.position = position
        self.length = length
        self.signed = signed

        if not self.length in [1, 2, 4]:
            raise Exception("bad length " + str(self.length))

        self.user_type = ('u' if not signed else '') + 'int' + str(length * 8) + '_t'

    def getter_sig(self):
        return "{0}* {1}".format(self.user_type, self.name)

    def unpack(self):
        if self.length == 1:
            return "*{0} = data[{1}];".format(self.name, self.position)
        elif self.length == 2:
            return "*{0} = SW_TO_SHORT(data + {1});".format(self.name, self.position)
        elif self.length == 4:
            return "*{0} = DW_TO_INT(data + {1});".format(self.name, self.position)

    def setter_sig(self):
        return "{0} {1}".format(self.user_type, self.name)

    def pack(self):
        if self.length == 1:
            return "data[{0}] = {1};".format(self.position, self.name)
        elif self.length == 2:
            return "SHORT_TO_SW({0}, data + {1});".format(self.name, self.position)
        elif self.length == 4:
            return "INT_TO_DW({0}, data + {1});".format(self.name, self.position)

    def spec(self):
        rep = [('position', self.position), ('length', self.length)]
        if self.signed:
            rep.append(('signed', True))
        return rep

    @staticmethod
    def load(spec):
        return IntField(spec['name'], spec['position'], spec['length'], spec['signed'] if signed in spec else False)

def load_field(name, spec):
    if spec['type'] == 'int':
        return IntField(name, spec['position'], spec['length'], spec.get('signed', False))
    else:
        raise Exception("unknown field type '{0}'".format(spec['type']))

GETTER_TEMPLATE = """/** @ingroup ctrl
 * {gen_doc}
 * @param devh UVC device handle
 * {args_doc}
 * @param req_code UVC_GET_* request to execute
 */
uvc_error_t uvc_get_{control_name}(uvc_device_handle_t *devh, {args_signature}, enum uvc_req_code req_code) {{
  uint8_t data[{control_length}];
  uvc_error_t ret;

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_GET, req_code,
    {control_code} << 8,
    {unit_fn} << 8 | devh->info->ctrl_if.bInterfaceNumber,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data)) {{
    {unpack}
    return UVC_SUCCESS;
  }} else {{
    return ret;
  }}
}}
"""

SETTER_TEMPLATE = """/** @ingroup ctrl
 * {gen_doc}
 * @param devh UVC device handle
 * {args_doc}
 */
uvc_error_t uvc_set_{control_name}(uvc_device_handle_t *devh, {args_signature}) {{
  uint8_t data[{control_length}];
  uvc_error_t ret;

  {pack}

  ret = libusb_control_transfer(
    devh->usb_devh,
    REQ_TYPE_SET, UVC_SET_CUR,
    {control_code} << 8,
    {unit_fn} << 8 | devh->info->ctrl_if.bInterfaceNumber,
    data,
    sizeof(data),
    0);

  if (ret == sizeof(data))
    return UVC_SUCCESS;
  else
    return ret;
}}
"""

def gen_decl(unit_name, unit, control_name, control):
    fields = [(load_field(field_name, field_details), field_details['doc']) for field_name, field_details in control['fields'].items()] if 'fields' in control else []

    get_args_signature = ', '.join([field.getter_sig() for (field, desc) in fields])
    set_args_signature = ', '.join([field.setter_sig() for (field, desc) in fields])

    return "uvc_error_t uvc_get_{function_name}(uvc_device_handle_t *devh, {args_signature}, enum uvc_req_code req_code);\n".format(**{
        "function_name": control_name,
        "args_signature": get_args_signature
    }) + "uvc_error_t uvc_set_{function_name}(uvc_device_handle_t *devh, {args_signature});\n".format(**{
        "function_name": control_name,
        "args_signature": set_args_signature
    })

def gen_ctrl(unit_name, unit, control_name, control):
    fields = [(load_field(field_name, field_details), field_details['doc']) for field_name, field_details in control['fields'].items()] if 'fields' in control else []

    get_args_signature = ', '.join([field.getter_sig() for (field, desc) in fields])
    set_args_signature = ', '.join([field.setter_sig() for (field, desc) in fields])
    unpack = "\n    ".join([field.unpack() for (field, desc) in fields])
    pack = "\n  ".join([field.pack() for (field, desc) in fields])

    get_gen_doc_raw = None
    set_gen_doc_raw = None

    if 'doc' in control:
        doc = control['doc']

        if isinstance(doc, str):
            get_gen_doc_raw = "\n * ".join(doc.splitlines())
            set_gen_doc_raw = get_gen_doc_raw
        else:
            if 'get' in doc:
                get_gen_doc_raw = "\n * ".join(doc['get'].splitlines())
            if 'set' in doc:
                set_gen_doc_raw = "\n * ".join(doc['set'].splitlines())

    if get_gen_doc_raw is not None:
        get_gen_doc = get_gen_doc_raw.format(gets_sets='Reads')
    else:
        get_gen_doc = '@brief Reads the ' + control['control'] + ' control.'

    if set_gen_doc_raw is not None:
        set_gen_doc = set_gen_doc_raw.format(gets_sets='Sets')
    else:
        set_gen_doc = '@brief Sets the ' + control['control'] + ' control.'

    get_args_doc = "\n * ".join(["@param[out] {0} {1}".format(field.name, desc) for (field, desc) in fields])
    set_args_doc = "\n * ".join(["@param {0} {1}".format(field.name, desc) for (field, desc) in fields])

    control_code = 'UVC_' + unit['control_prefix'] + '_' + control['control'] + '_CONTROL'

    unit_fn = "uvc_get_camera_terminal(devh)->bTerminalID" if (unit_name == "camera_terminal") else ("uvc_get_" + unit_name + "s(devh)->bUnitID")

    return GETTER_TEMPLATE.format(
        unit=unit,
        unit_fn=unit_fn,
        control_name=control_name,
        control_code=control_code,
        control_length=control['length'],
        args_signature=get_args_signature,
        args_doc=get_args_doc,
        gen_doc=get_gen_doc,
        unpack=unpack) + "\n\n" + SETTER_TEMPLATE.format(
            unit=unit,
            unit_fn=unit_fn,
            control_name=control_name,
            control_code=control_code,
            control_length=control['length'],
            args_signature=set_args_signature,
            args_doc=set_args_doc,
            gen_doc=set_gen_doc,
            pack=pack
        )

def export_unit(unit):
    def fmt_doc(doc):
        def wrap_doc_entry(entry):
            if "\n" in entry:
                return literal(entry)
            else:
                return entry

        if isinstance(doc, str):
            return wrap_doc_entry(doc)
        else:
            return OrderedDict([(mode, wrap_doc_entry(text)) for mode, text in doc.items()])

    def fmt_ctrl(control_name, control_details):
        contents = OrderedDict()
        contents['control'] = control_details['control']
        contents['length'] = control_details['length']
        contents['fields'] = control_details['fields']

        if 'doc' in control_details:
            contents['doc'] = fmt_doc(control_details['doc'])

        return (control_name, contents)

    unit_out = OrderedDict()
    unit_out['type'] = unit['type']
    if 'guid' in unit:
        unit_out['guid'] = unit['guid']
    if 'description' in unit:
        unit_out['description'] = unit['description']
    if 'control_prefix' in unit:
        unit_out['control_prefix'] = unit['control_prefix']
    unit_out['controls'] = OrderedDict([fmt_ctrl(ctrl_name, ctrl_details) for ctrl_name, ctrl_details in unit['controls'].items()])
    return unit_out

if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hi:", ["help", "input="])
    except getopt.GetoptError as err:
        print(str(err))
        usage()
        sys.exit(-1)

    inputs = []

    for opt, val in opts:
        if opt in ('-h', '--help'):
            usage()
            sys.exit(0)
        elif opt in ('-i', '--input'):
            inputs.append(val)

    mode = None
    for arg in args:
        if arg in ('def', 'decl', 'yaml'):
            if mode is None:
                mode = arg
            else:
                print("Can't specify more than one mode")
                sys.exit(-1)
        else:
            print("Invalid mode '{0}'".format(arg))
            sys.exit(-1)

    def iterunits():
        for input_file in inputs:
            with open(input_file, "r") as fp:
                units = yaml.load(fp)['units']
                for unit_name, unit_details in units.iteritems():
                    yield unit_name, unit_details

    if mode == 'def':
        print("""/* This is an AUTO-GENERATED file! Update it with the output of `ctrl-gen.py def`. */
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

static const int REQ_TYPE_SET = 0x21;
static const int REQ_TYPE_GET = 0xa1;
""")
        fun = gen_ctrl
    elif mode == 'decl':
        fun = gen_decl
    elif mode == 'yaml':
        exported_units = OrderedDict()
        for unit_name, unit_details in iterunits():
            exported_units[unit_name] = export_unit(unit_details)

        yaml.dump({'units': exported_units}, sys.stdout, default_flow_style=False)
        sys.exit(0)

    for unit_name, unit_details in iterunits():
        for control_name, control_details in unit_details['controls'].iteritems():
            code = fun(unit_name, unit_details, control_name, control_details)
            print(code)
