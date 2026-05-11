#include "../new_common.h"
#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../hal/hal_pins.h"
#include "drv_display_shared.h"

#if defined(PLATFORM_ESPIDF) && defined(ENABLE_DISPLAY)

#include <lvgl.h>
#include "display/lvgl_v8_port.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"

static lv_obj_t *screen_main;
static lv_obj_t *msg_list;
static lv_obj_t *kb;
static lv_obj_t *mem_label;
static lv_obj_t *chart;
static lv_chart_series_t * ser;
static lv_obj_t * arc_thermo;
static lv_obj_t * label_thermo;
static int tick_count = 0;

static void add_chat_bubble(const char * txt, bool is_user) {
    lv_obj_t * wrapper = lv_obj_create(msg_list);
    lv_obj_set_width(wrapper, lv_pct(100));
    lv_obj_set_height(wrapper, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_clear_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * msg_box = lv_obj_create(wrapper);
    lv_obj_set_size(msg_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(msg_box, 15, 0);
    lv_obj_set_style_border_width(msg_box, 0, 0);
    lv_obj_set_style_pad_all(msg_box, 15, 0);
    
    lv_obj_t * label = lv_label_create(msg_box);
    lv_label_set_text(label, txt);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, 300); // fixed max width for wrap
    
    if(is_user) {
        lv_obj_set_style_bg_color(msg_box, lv_color_hex(0x2196F3), 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(msg_box, LV_ALIGN_RIGHT_MID, -10, 0);
    } else {
        lv_obj_set_style_bg_color(msg_box, lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0x333333), 0);
        lv_obj_align(msg_box, LV_ALIGN_LEFT_MID, 10, 0);
    }
    lv_obj_scroll_to_view(wrapper, LV_ANIM_ON);
}

static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(kb);
    } else if(code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    } else if(code == LV_EVENT_READY) {
        const char * txt = lv_textarea_get_text(ta);
        if(strlen(txt) > 0) {
            add_chat_bubble(txt, true);
            // Simulate bot reply
            add_chat_bubble("I am a demo bot! That's interesting.", false);
        }
        lv_textarea_set_text(ta, "");
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(ta, LV_STATE_FOCUSED);
    }
}

static void thermo_anim_cb(void * var, int32_t v) {
    lv_arc_set_value((lv_obj_t*)var, v);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d\xc2\xb0""C", (int)v); // UTF-8 degree symbol
    lv_label_set_text(label_thermo, buf);
}

static void gui_build_main_screen() {
    screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0xF5F7FA), LV_PART_MAIN);

    // Left sidebar tab view for landscape Pro look
    lv_obj_t * tabview = lv_tabview_create(screen_main, LV_DIR_LEFT, 100);
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0xF5F7FA), LV_PART_MAIN);
    
    lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x2196F3), (lv_style_selector_t)((int)LV_PART_ITEMS | (int)LV_STATE_CHECKED));
    lv_obj_set_style_border_width(tab_btns, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(tab_btns, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(tab_btns, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(tab_btns, 15, LV_PART_MAIN);

    lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "\n" LV_SYMBOL_HOME "\nHome");
    lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "\n" LV_SYMBOL_SETTINGS "\nCtrl");
    lv_obj_t * tab3 = lv_tabview_add_tab(tabview, "\n" LV_SYMBOL_BELL "\nChat");

    // --- TAB 1: Home Dashboard ---
    lv_obj_set_layout(tab1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab1, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(tab1, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(tab1, 20, 0);

    // Card 1: Thermostat
    lv_obj_t * card1 = lv_obj_create(tab1);
    lv_obj_set_size(card1, 280, 280);
    lv_obj_set_style_radius(card1, 20, 0);
    lv_obj_set_style_border_width(card1, 0, 0);
    lv_obj_set_style_shadow_width(card1, 20, 0);
    lv_obj_set_style_shadow_color(card1, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card1, 10, 0);

    lv_obj_t * thermo_title = lv_label_create(card1);
    lv_label_set_text(thermo_title, "Living Room");
    lv_obj_align(thermo_title, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(thermo_title, lv_color_hex(0x888888), 0);

    arc_thermo = lv_arc_create(card1);
    lv_obj_set_size(arc_thermo, 180, 180);
    lv_obj_align(arc_thermo, LV_ALIGN_CENTER, 0, 15);
    lv_arc_set_range(arc_thermo, 10, 35);
    lv_arc_set_value(arc_thermo, 10);
    lv_obj_set_style_arc_color(arc_thermo, lv_color_hex(0xE0E0E0), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_thermo, lv_color_hex(0xFF9800), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_thermo, 15, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_thermo, 15, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_thermo, NULL, LV_PART_KNOB); 

    label_thermo = lv_label_create(card1);
    lv_label_set_text(label_thermo, "10\xc2\xb0""C");
    lv_obj_align(label_thermo, LV_ALIGN_CENTER, 0, 15);
    // Note: ensure font supports degree symbol, otherwise it may not show. We use default font.

    // Card 2: Power Chart
    lv_obj_t * card2 = lv_obj_create(tab1);
    lv_obj_set_size(card2, 340, 280);
    lv_obj_set_style_radius(card2, 20, 0);
    lv_obj_set_style_border_width(card2, 0, 0);
    lv_obj_set_style_shadow_width(card2, 20, 0);
    lv_obj_set_style_shadow_color(card2, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card2, 10, 0);

    lv_obj_t * chart_title = lv_label_create(card2);
    lv_label_set_text(chart_title, "Power Usage (W)");
    lv_obj_align(chart_title, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(chart_title, lv_color_hex(0x888888), 0);

    chart = lv_chart_create(card2);
    lv_obj_set_size(chart, 300, 180);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 15);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x2196F3), LV_PART_ITEMS);
    
    ser = lv_chart_add_series(chart, lv_color_hex(0x2196F3), LV_CHART_AXIS_PRIMARY_Y);
    for(int i = 0; i < 10; i++) {
        lv_chart_set_next_value(chart, ser, lv_rand(100, 500));
    }

    // --- TAB 2: Controls ---
    lv_obj_set_layout(tab2, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab2, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(tab2, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for(int i=0; i<6; i++) {
        lv_obj_t * c_card = lv_obj_create(tab2);
        lv_obj_set_size(c_card, 220, 120);
        lv_obj_set_style_radius(c_card, 15, 0);
        lv_obj_set_style_border_width(c_card, 0, 0);
        lv_obj_set_style_shadow_width(c_card, 10, 0);
        lv_obj_set_style_shadow_color(c_card, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(c_card, 10, 0);

        lv_obj_t * sw = lv_switch_create(c_card);
        lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_set_style_bg_color(sw, lv_color_hex(0x4CAF50), (lv_style_selector_t)((int)LV_PART_INDICATOR | (int)LV_STATE_CHECKED));

        lv_obj_t * l = lv_label_create(c_card);
        char lbuf[32];
        snprintf(lbuf, sizeof(lbuf), "Light %d", i+1);
        lv_label_set_text(l, lbuf);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 10, 0);
    }

    // --- TAB 3: Fake Chat ---
    lv_obj_t * chat_container = lv_obj_create(tab3);
    lv_obj_set_size(chat_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_border_width(chat_container, 0, 0);
    lv_obj_set_style_bg_opa(chat_container, 0, 0);
    lv_obj_set_style_pad_all(chat_container, 0, 0);
    lv_obj_set_layout(chat_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(chat_container, LV_FLEX_FLOW_COLUMN);

    msg_list = lv_obj_create(chat_container);
    lv_obj_set_width(msg_list, LV_PCT(100));
    lv_obj_set_flex_grow(msg_list, 1);
    lv_obj_set_style_border_width(msg_list, 0, 0);
    lv_obj_set_style_bg_color(msg_list, lv_color_hex(0xF5F7FA), 0);
    lv_obj_set_layout(msg_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(msg_list, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * ta = lv_textarea_create(chat_container);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_placeholder_text(ta, "Type a message...");
    lv_obj_add_event_cb(ta, ta_event_cb, LV_EVENT_ALL, NULL);

    // Global keyboard
    kb = lv_keyboard_create(screen_main);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    // Setup initial chat
    add_chat_bubble("Hello! I'm your smart home assistant.", false);

    // System info in corner
    mem_label = lv_label_create(screen_main);
    lv_obj_align(mem_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_text_color(mem_label, lv_color_hex(0xAAAAAA), 0);

    // Startup Animation for thermostat
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc_thermo);
    lv_anim_set_exec_cb(&a, thermo_anim_cb);
    lv_anim_set_time(&a, 1500);
    lv_anim_set_values(&a, 10, 24);
    lv_anim_start(&a);
}

extern "C" void DisplayDemo_Init() {
    if (!Display_PreInit()) {
        return;
    }
    
    lvgl_port_lock(-1);
    gui_build_main_screen();
    lv_scr_load(screen_main);
    lvgl_port_unlock();

    Display_PostInit();
}

extern "C" void DisplayDemo_OnEverySecond() {
    if(!g_display_ready) return;
    tick_count++;

    char mbuf[64];
    size_t free_int = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    snprintf(mbuf, sizeof(mbuf), "Free Mem: %u B", (unsigned)free_int);
    
    lvgl_port_lock(-1);
    if(mem_label) lv_label_set_text(mem_label, mbuf);
    if(chart && ser) lv_chart_set_next_value(chart, ser, lv_rand(100, 500));
    lvgl_port_unlock();
}

extern "C" void DisplayDemo_Shutdown() {
    Display_Shutdown();
}

#else

extern "C" void DisplayDemo_Init() {}
extern "C" void DisplayDemo_OnEverySecond() {}
extern "C" void DisplayDemo_Shutdown() {}

#endif
