SET(SOURCES screens/ui_ScreenMain.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_lexin_1_png.c
    images/ui_img_lexin_2_png.c)

add_library(ui ${SOURCES})
