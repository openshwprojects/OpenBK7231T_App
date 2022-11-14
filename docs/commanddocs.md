## How to automatically document commands

Commands are reigstered like this:

`    CMD_RegisterCommand("obkDeviceList", "", Cmd_obkDeviceList, "qqq", NULL);`

by running `npm run getcommands` the source code is parsed and modified to add comment lines above such lines, if they do not already exist:

```
	//cmddetail:{"name":"obkDeviceList","args":"",
	//cmddetail:"descr":"qqq",
	//cmddetail:"fn":"Cmd_obkDeviceList","file":"driver/drv_ssdp.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("obkDeviceList", "", Cmd_obkDeviceList, "qqq", NULL);
```

Such comment lines are parsed during the proces to create entries in `docs/commands.md`:

| Command        | Arguments          | Description  |
|:------------- |:------------- | -----:|
| obkDeviceList |  | qqq |

and `docs/commands-extended.md`

| Command        | Arguments          | Description  | Loc |
|:------------- |:-------------:|:----- | ------:|
| obkDeviceList |  | qqq | driver/drv_ssdp.c<br/>Cmd_obkDeviceList |


The JSON strings which result in the entries may be multi-line by using `\n` in-line.

When adding a new command, add the 

`    CMD_RegisterCommand("obkDeviceList", "", Cmd_obkDeviceList, "qqq", NULL);`

line, save your file, run npm run getcommands, and then augment the json with details.



