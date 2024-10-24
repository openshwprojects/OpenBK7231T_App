MENUBUILD   := ../../../bin/build/config

MENUCFLAGS  := -I/usr/include/ncurses -DCURSES_LOC="<ncurses.h>" -DLOCALE -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer

ifeq ($(shell uname -s),Linux)
MENULDFLAGS := -lncurses
else
MENULDFLAGS := -lncurses -lintl
endif

HOSTCC      := gcc

CONFBASE    := $(MENUBUILD)/lxdialog/menubox.o   \
               $(MENUBUILD)/lxdialog/yesno.o     \
               $(MENUBUILD)/lxdialog/textbox.o   \
               $(MENUBUILD)/lxdialog/inputbox.o  \
               $(MENUBUILD)/lxdialog/util.o      \
               $(MENUBUILD)/lxdialog/checklist.o \
               $(MENUBUILD)/zconf.tab.o

all: mconf

$(MENUBUILD)/%.o: %.c
	@mkdir -p $(MENUBUILD)
	@mkdir -p $(MENUBUILD)/lxdialog
	@echo "HOSTCC   $<"; $(HOSTCC) -c $(MENUCFLAGS) $< -o $@

mconf: $(CONFBASE) \
        $(MENUBUILD)/mconf.o
	@echo "HOSTLD   $@"; $(HOSTCC) -o $(MENUBUILD)/$@ $^ $(MENULDFLAGS)

clean :
	@find $(MENUBUILD) -name "*.o" -exec rm "{}" \;
#	@rm -rf $(MENUBUILD)