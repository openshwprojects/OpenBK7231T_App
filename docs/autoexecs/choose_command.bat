

alias col1 led_basecolor_rgb 0xFF0000
alias col2 led_basecolor_rgb 0x00FF00
alias col3 led_basecolor_rgb 0x0000FF
alias col4 led_basecolor_rgb 0xFFFF00
alias col5 led_basecolor_rgb 0xFF00FF
alias col6 led_basecolor_rgb 0x00FFFF

// use choice to choose effect by index stored in $CH10
alias do_chosen_effect Choice $CH10 col1 col2 col3 col4 col5 col6
// when click on Btn_ScriptOnly on P24 happens, add 1 to $CH10 (wrap to 0-5) and do effect
addEventHandler OnClick 24 backlog addChannel 10 1 0 5 1; do_chosen_effect