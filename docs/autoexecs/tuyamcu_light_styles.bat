// See: https://www.elektroda.com/rtvforum/topic4014389.html

startDriver httpButtons
setButtonEnabled 0 1
setButtonLabel 0 "Music mode"
setButtonCommand 0 "tuyaMcu_sendState 21 4 3"

setButtonEnabled 1 1
setButtonLabel 1 "Light mode"
setButtonCommand 1 "tuyaMcu_sendState 21 4 1" 
 
setButtonEnabled 2 1
setButtonLabel 2 "Curtain"
setButtonCommand 2 "startScript autoexec.bat do_cur"
 
setButtonEnabled 3 1
setButtonLabel 3 "Collision"
setButtonCommand 3 "startScript autoexec.bat do_col"
 
setButtonEnabled 4 1
setButtonLabel 4 "Rainbow"
setButtonCommand 4 "startScript autoexec.bat do_rai"
 
setButtonEnabled 5 1
setButtonLabel 5 "Pile"
setButtonCommand 5 "startScript autoexec.bat do_pil"
 
setButtonEnabled 6 1
setButtonLabel 6 "Firework"
setButtonCommand 6 "startScript autoexec.bat do_fir"
 
setButtonEnabled 7 1
setButtonLabel 7 "Chase"
setButtonCommand 7 "startScript autoexec.bat do_chase"
 
// startDriver MCP9808 [ClkPin] [DatPin] [OptionalTargetChannel]
startDriver MCP9808 7 8  1
MCP9808_Adr 0x30
MCP9808_Cycle 1


// use choice to choose effect by index stored in $CH10
alias do_chosen_effect Choice $CH10 cmd_cur cmd_col cmd_rai cmd_pil cmd_fir cmd_chase
// when click on Btn_ScriptOnly on P24 happens, add 1 to $CH10 (wrap to 0-5) and do effect
addEventHandler OnClick 24 backlog addChannel 10 1 0 4 1; do_chosen_effect
 
// stop execution
return
 
 


do_chase:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 020e0d00001403e803e800000000
return
 
 
do_cur:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 000e0d00002e03e802cc00000000
return
 
do_col:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 07464602000003e803e800000000464602007803e803e80000000046460200f003e803e800000000464602003d03e803e80000000046460200ae03e803e800000000464602011303e803e800000000
return
 
do_rai:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 06464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000
return
 
do_pil:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 010e0d000084000003e800000000
return
 
do_fir:
tuyaMcu_sendState 21 4 2
delay_s 0.1
tuyaMcu_sendState 25 3 05464601000003e803e800000000464601007803e803e80000000046460100f003e803e800000000464601003d03e803e80000000046460100ae03e803e800000000464601011303e803e800000000
return