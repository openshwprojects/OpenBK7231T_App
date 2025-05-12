# meant to be included, depends on BERRY_SRCPATH and BERRY_MODULEPATH

INCLUDES += -I$(BERRY_SRCPATH)

BERRY_SRC_C += $(BERRY_SRCPATH)/be_api.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_baselib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_bytecode.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_byteslib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_class.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_code.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_debug.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_debuglib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_exec.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_filelib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_func.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_gc.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_gclib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_globallib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_introspectlib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_jsonlib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_lexer.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_libs.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_list.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_listlib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_map.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_maplib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_mathlib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_mem.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_module.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_object.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_oslib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_parser.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_rangelib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_repl.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_solidifylib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_strictlib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_string.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_strlib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_syslib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_timelib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_undefinedlib.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_var.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_vector.c
BERRY_SRC_C += $(BERRY_SRCPATH)/be_vm.c


ifeq ($(TARGET_PLATFORM),bk7231n)
else ifeq ($(TARGET_PLATFORM),bk7231t)
else
BERRY_SRC_C += $(BERRY_MODULEPATH)/../be_bindings.c
BERRY_SRC_C += $(BERRY_MODULEPATH)/../be_modtab.c
BERRY_SRC_C += $(BERRY_MODULEPATH)/../be_port.c
BERRY_SRC_C += $(BERRY_MODULEPATH)/../be_run.c

BERRY_SRC_C += $(BERRY_MODULEPATH)/be_i2c.c
endif
