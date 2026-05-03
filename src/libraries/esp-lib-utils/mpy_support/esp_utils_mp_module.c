/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "py/runtime.h"
#include "memory/esp_utils_mem.h"

static mp_obj_t mem_info(void)
{
    return mp_obj_new_bool(esp_utils_mem_print_info());
}
MP_DEFINE_CONST_FUN_OBJ_0(mem_info_func_obj, mem_info);

// Define all attributes of the module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
static const mp_rom_map_elem_t module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_esp_utils) },
    { MP_ROM_QSTR(MP_QSTR_mem_info), MP_ROM_PTR(&mem_info_func_obj) },
};
static MP_DEFINE_CONST_DICT(module_globals, module_globals_table);

// Define module object.
const mp_obj_module_t esp_utils_mp_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *) &module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_esp_utils, esp_utils_mp_module);
