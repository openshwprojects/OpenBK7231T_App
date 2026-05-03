/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/esp_panel_utils_log.h"
#include "utils/esp_panel_utils_cxx.hpp"
#include "board/esp_panel_board.hpp"
#include "esp_panel_mp_types.h"
#include "esp_panel_mp_board.h"

namespace esp_panel::board {

/**
 * MicroPython Wrappers
 */
// Object
struct MP_Board {
    mp_obj_base_t base;
    std::shared_ptr<Board> board = nullptr;
};

static mp_obj_t board_del(mp_obj_t self_in)
{
    MP_Board *self = static_cast<MP_Board *>(MP_OBJ_TO_PTR(self_in));

    self->board = nullptr;

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1_CXX(board_del_func_obj, board_del);

static mp_obj_t board_init(mp_obj_t self_in)
{
    MP_Board *self = static_cast<MP_Board *>(MP_OBJ_TO_PTR(self_in));

    return mp_obj_new_bool(self->board->init());
}
static MP_DEFINE_CONST_FUN_OBJ_1_CXX(board_init_func_obj, board_init);

static mp_obj_t board_begin(mp_obj_t self_in)
{
    MP_Board *self = static_cast<MP_Board *>(MP_OBJ_TO_PTR(self_in));

    return mp_obj_new_bool(self->board->begin());
}
static MP_DEFINE_CONST_FUN_OBJ_1_CXX(board_begin_func_obj, board_begin);

static mp_obj_t board_deinit(mp_obj_t self_in)
{
    MP_Board *self = static_cast<MP_Board *>(MP_OBJ_TO_PTR(self_in));

    return mp_obj_new_bool(self->board->del());
}
static MP_DEFINE_CONST_FUN_OBJ_1_CXX(board_deinit_func_obj, board_deinit);

static mp_obj_t board_color_bar_test(mp_obj_t self_in)
{
    MP_Board *self = static_cast<MP_Board *>(MP_OBJ_TO_PTR(self_in));

    return mp_obj_new_bool(self->board->getLCD()->colorBarTest());
}
static MP_DEFINE_CONST_FUN_OBJ_1_CXX(board_color_bar_test_func_obj, board_color_bar_test);

// Local dict
static const mp_rom_map_elem_t locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR__del__), MP_ROM_PTR(&board_del_func_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&board_init_func_obj) },
    { MP_ROM_QSTR(MP_QSTR_begin), MP_ROM_PTR(&board_begin_func_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&board_deinit_func_obj) },
    { MP_ROM_QSTR(MP_QSTR_color_bar_test), MP_ROM_PTR(&board_color_bar_test_func_obj) },
};
static MP_DEFINE_CONST_DICT(board_locals_dict, locals_dict_table);

// Constructor
static mp_obj_t make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    MP_Board *self = mp_obj_malloc(MP_Board, &esp_panel_mp_board_type);
    self->board = utils::make_shared<Board>();

    return MP_OBJ_FROM_PTR(self);
}

// Print
static void print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_printf(print, "Board");
}

} // namespace esp_panel

// Type
MP_DEFINE_CONST_OBJ_TYPE(
    esp_panel_mp_board_type,
    MP_QSTR_Board,
    MP_TYPE_FLAG_NONE,
    make_new, (const void *)esp_panel::board::make_new,
    print, (const void *)esp_panel::board::print,
    locals_dict, &esp_panel::board::board_locals_dict
);
