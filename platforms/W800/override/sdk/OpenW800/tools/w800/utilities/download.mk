download:
	@echo ""
	@echo "follow the prompts until the firmware download is complete:"
	@echo ""
	@../../../../../wm_tool -c $(DL_PORT) -rs at -ds 2M -dl ../../../../../../../bin/w800/w800.fls