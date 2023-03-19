// channels 1 to 5 are used
setChannelType 1 toggle
setChannelType 2 toggle
setChannelType 3 toggle
setChannelType 4 toggle
setChannelType 5 toggle
// Btn_ScriptOnly is set on P26
addEventHandler OnClick 26 ToggleChannel 1
addEventHandler OnDblClick 26 ToggleChannel 2
addEventHandler On3Click 26 ToggleChannel 3
addEventHandler On4Click 26 ToggleChannel 4
addEventHandler On5Click 26 ToggleChannel 5