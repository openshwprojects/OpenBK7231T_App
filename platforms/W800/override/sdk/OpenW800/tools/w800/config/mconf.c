/*
 * Copyright (C) 2002 Roman Zippel <zippel@linux-m68k.org>
 * Released under the terms of the GNU GPL v2.0.
 *
 * Introduced single menu mode (show all sub-menus in one large tree).
 * 2002-11-06 Petr Baudis <pasky@ucw.cz>
 *
 * i18n, 2005, Arnaldo Carvalho de Melo <acme@conectiva.com.br>
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <locale.h>

#include "lkc.h"
#include "lxdialog/dialog.h"

int g_lang_index = 0;

static const char *mconf_readme[2] = {N_(
"Overview\n"
"--------\n"
"This interface lets you select features and parameters for the build.\n"
"Features can either be built-in or ignored. Parameters\n"
"must be entered in as decimal or hexadecimal numbers or text.\n"
"\n"
"  [ ] can be built in or removed\n"
"  ( ) can enter parameters\n"
"\n"
"To change any of these features, highlight it with the cursor\n"
"keys and press <Y> to build it in, <N> to remove it.\n"
"You may also press the <Space Bar> to cycle\n"
"You can also press <Enter> to enter or edit parameters\n"
"\n"
"Some additional hints:\n"
"Menuconfig will display larger menus on screens which are\n"
"set to display more than the standard 25 row by 80 column geometry.\n"
"\n"),
N_(
"概述\n"
"--------\n"
"此界面使您可以配置需要的功能和参数。\n"
"功能可以选择是或否。\n"
"参数必须以十进制或十六进制数字或文本输入。\n"
"\n"
"  [ ] 可以选择是否\n"
"  ( ) 可以输入参数\n"
"\n"
"要更改所有这些功能，请用光标突出显示\n"
"并按 <Y> 选择，按<N> 取消。\n"
"您也可以按 <空格> 选择或者取消\n"
"您也可以按 <回车> 输入或编辑参数\n"
"\n"
"一些其他提示：\n"
"需要终端界面至少有25X80的大小才能正常显示。\n"
"\n")
},
*menu_instructions[2] = {N_(
	"Arrow keys navigate the menu.  "
    "<Enter> selects submenus --->.  "
    "Highlighted letters are hotkeys.  "
    "Pressing <Y> includes, <N> excludes.  "
    "Press <Esc><Esc> to exit, <?> for Help, </> for Search.  "),
 N_("使用方向键浏览菜单。"
    "<回车> 选择子菜单 --->。"
    "高亮显示的字母是热键。"
    "按 <Y> 选中，<N> 取消。"
    "按下 <Esc><Esc> 退出，<?> 帮助，</> 搜索。"),
},
*radiolist_instructions[2] = {N_(
	"Use the arrow keys to navigate this window or "
	"press the hotkey of the item you wish to select "
	"followed by the <SPACE BAR>. "
	"Press <?> for additional information about this option."),
 N_("使用方向键浏览此窗口并按 <空格> 或"
    "按您要选择的项目的热键。"
    "按 <?> 获取有关此选项的更多信息。"),
},
*inputbox_instructions_int[2] = {N_(
	"Please enter a decimal value. "
	"Fractions will not be accepted.  "
	"Use the <TAB> key to move from the input field to the buttons below it."),
 N_("请输入一个十进制值。"
    "分数将不被接受。"
    "使用 <TAB> 键从输入字段移至其下方的按钮。"),
},
*inputbox_instructions_hex[2] = {N_(
	"Please enter a hexadecimal value. "
	"Use the <TAB> key to move from the input field to the buttons below it."),
 N_("请输入一个十六进制值。"
    "使用 <TAB> 键从输入字段移至其下方的按钮。"),
},
*inputbox_instructions_string[2] = {N_(
	"Please enter a string value. "
	"Use the <TAB> key to move from the input field to the buttons below it."),
 N_("请输入一个字符串值。"
    "使用 <TAB> 键从输入字段移至其下方的按钮。"),
},
*setmod_text[2] = {N_(
	"This feature depends on another which has been configured as a module.\n"
	"As a result, this feature will be built as a module."),
 N_("此功能取决于已配置为模块的另一个功能。\n"
    "因此，此功能将作为模块构建。"),
},
*load_config_text[2] = {N_(
	"Enter the name of the configuration file you wish to load.  "
	"Accept the name shown to restore the configuration you "
	"last retrieved.  Leave blank to abort."),
 N_("输入要加载的配置文件的名称。\n"
    "如果不确定，应将其留空。"),
},
*load_config_help[2] = {N_(
	"\n"
	"For various reasons, one may wish to keep several different\n"
	"configurations available on a single machine.\n"
	"\n"
	"If you have saved a previous configuration in a file other than the\n"
	"default one, entering its name here will allow you to modify that\n"
	"configuration.\n"
	"\n"
	"If you are uncertain, then you have probably never used alternate\n"
	"configuration files. You should therefore leave this blank to abort.\n"),
 N_("\n"
    "您可以在计算机上保留几种不同的配置。\n"
    "\n"
    "如果您以前的配置已保存，则\n"
    "在此处输入配置文件名称\n"
    "载入后可对其进行修改。\n"
    "\n"
    "如果不确定，应将其留空。\n"),
},
*save_config_text[2] = {N_(
	"Enter a filename to which this configuration should be saved "
	"as an alternate.  Leave blank to abort."),
 N_("输入此配置的保存文件名。如果不保存，应留空。"),
},
*save_config_help[2] = {N_(
	"\n"
	"For various reasons, one may wish to keep different configurations\n"
	"available on a single machine.\n"
	"\n"
	"Entering a file name here will allow you to later retrieve, modify\n"
	"and use the current configuration as an alternate to whatever\n"
	"configuration options you have selected at that time.\n"
	"\n"
	"If you are uncertain what all this means then you should probably\n"
	"leave this blank.\n"),
 N_("\n"
    "您可以在计算机上保留几种不同的配置。\n"
    "\n"
    "在此输入文件名将当前配置保存为文件。\n"
    "\n"
    "如果不确定，应将其留空。\n"),
},
*search_help[2] = {N_(
	"\n"
	"Search for symbols and display their relations.\n"
	"Regular expressions are allowed.\n"
	"Example: search for \"^W800\"\n"
	"Result:\n"
	"-----------------------------------------------------------------\n"
	"Symbol: W800_DOWNLOAD_PORT [=COM18]\n"
	"Type  : string\n"
	"Prompt: download port\n"
	"  Location:\n"
	"(2) -> Download Configuration\n"
	"  Defined at wconfig:62\n"
	"\n"),
 N_("\n"
	"搜索符号并显示其关系。\n"
	"允许使用正则表达式。\n"
	"示例：搜索 \"^W800\"\n"
	"结果：\n"
	"-----------------------------------------------------------------\n"
	"Symbol: W800_DOWNLOAD_PORT [=COM18]\n"
	"Type  : string\n"
	"Prompt: download port\n"
	"  Location:\n"
	"(2) -> Download Configuration\n"
	"  Defined at wconfig:62\n"
    "\n")};

static int indent;
static struct menu *current_menu;
static int child_count;
static int single_menu_mode;
static int show_all_options;
static int save_and_exit;
static int silent;

static void conf(struct menu *menu, struct menu *active_menu);
static void conf_choice(struct menu *menu);
static void conf_string(struct menu *menu);
static void conf_load(void);
static void conf_save(void);
static int show_textbox_ext(const char *title, char *text, int r, int c,
			    int *keys, int *vscroll, int *hscroll,
			    update_text_fn update_text, void *data);
static void show_textbox(const char *title, const char *text, int r, int c);
static void show_helptext(const char *title, const char *text);
static void show_help(struct menu *menu);

static char filename[PATH_MAX+1];
static void set_config_filename(const char *config_filename)
{
	static char menu_backtitle[PATH_MAX+128];
	int size;

	size = snprintf(menu_backtitle, sizeof(menu_backtitle),
			"%s - %s", config_filename, rootmenu.prompt->text);
	if (size >= sizeof(menu_backtitle))
		menu_backtitle[sizeof(menu_backtitle)-1] = '\0';
	set_dialog_backtitle(menu_backtitle);

	size = snprintf(filename, sizeof(filename), "%s", config_filename);
	if (size >= sizeof(filename))
		filename[sizeof(filename)-1] = '\0';
}

struct subtitle_part {
	struct list_head entries;
	const char *text;
};
static LIST_HEAD(trail);

static struct subtitle_list *subtitles;
static void set_subtitle(void)
{
	struct subtitle_part *sp;
	struct subtitle_list *pos, *tmp;

	for (pos = subtitles; pos != NULL; pos = tmp) {
		tmp = pos->next;
		free(pos);
	}

	subtitles = NULL;
	list_for_each_entry(sp, &trail, entries) {
		if (sp->text) {
			if (pos) {
				pos->next = xcalloc(1, sizeof(*pos));
				pos = pos->next;
			} else {
				subtitles = pos = xcalloc(1, sizeof(*pos));
			}
			pos->text = sp->text;
		}
	}

	set_dialog_subtitles(subtitles);
}

static void reset_subtitle(void)
{
	struct subtitle_list *pos, *tmp;

	for (pos = subtitles; pos != NULL; pos = tmp) {
		tmp = pos->next;
		free(pos);
	}
	subtitles = NULL;
	set_dialog_subtitles(subtitles);
}

struct search_data {
	struct list_head *head;
	struct menu **targets;
	int *keys;
};

static void update_text(char *buf, size_t start, size_t end, void *_data)
{
	struct search_data *data = _data;
	struct jump_key *pos;
	int k = 0;

	list_for_each_entry(pos, data->head, entries) {
		if (pos->offset >= start && pos->offset < end) {
			char header[4];

			if (k < JUMP_NB) {
				int key = '0' + (pos->index % JUMP_NB) + 1;

				sprintf(header, "(%c)", key);
				data->keys[k] = key;
				data->targets[k] = pos->target;
				k++;
			} else {
				sprintf(header, "   ");
			}

			memcpy(buf + pos->offset, header, sizeof(header) - 1);
		}
	}
	data->keys[k] = 0;
}

static const char *search_prom_str[2] = 
{"Enter (sub)string or regexp to search for (with or without \"%s\")", 
"输入要搜索的(子)字符串或正则表达式(带或不带\"%s\")"};
static const char *search_cparm_str[2] = 
{"Search Configuration Parameter", 
"搜索配置参数"};
static const char *search_c_str[2] = 
{"Search Configuration", 
"搜索配置"};
static const char *search_str[2] = 
{"Search (%s)", 
"搜索 (%s)"};
static const char *search_res_str[2] = 
{"Search Results", 
"搜索结果"};

static void search_conf(void)
{
	struct symbol **sym_arr;
	struct gstr res;
	struct gstr title;
	char *dialog_input;
	int dres, vscroll = 0, hscroll = 0;
	bool again;
	struct gstr sttext;
	struct subtitle_part stpart;

	title = str_new();
	str_printf( &title, _(search_prom_str[g_lang_index]), CONFIG_);

again:
	dialog_clear();
	dres = dialog_inputbox(_(search_cparm_str[g_lang_index]),
			      str_get(&title),
			      10, 75, "");
	switch (dres) {
	case 0:
		break;
	case 1:
		show_helptext(_(search_c_str[g_lang_index]), search_help[g_lang_index]);
		goto again;
	default:
		str_free(&title);
		return;
	}

	/* strip the prefix if necessary */
	dialog_input = dialog_input_result;
	if (strncasecmp(dialog_input_result, CONFIG_, strlen(CONFIG_)) == 0)
		dialog_input += strlen(CONFIG_);

	sttext = str_new();
	str_printf(&sttext, search_str[g_lang_index], dialog_input_result);
	stpart.text = str_get(&sttext);
	list_add_tail(&stpart.entries, &trail);

	sym_arr = sym_re_search(dialog_input);
	do {
		LIST_HEAD(head);
		struct menu *targets[JUMP_NB];
		int keys[JUMP_NB + 1], i;
		struct search_data data = {
			.head = &head,
			.targets = targets,
			.keys = keys,
		};
		struct jump_key *pos, *tmp;

		res = get_relations_str(sym_arr, &head);
		set_subtitle();
		dres = show_textbox_ext(_(search_res_str[g_lang_index]), (char *)
					str_get(&res), 0, 0, keys, &vscroll,
					&hscroll, &update_text, (void *)
					&data);
		again = false;
		for (i = 0; i < JUMP_NB && keys[i]; i++)
			if (dres == keys[i]) {
				conf(targets[i]->parent, targets[i]);
				again = true;
			}
		str_free(&res);
		list_for_each_entry_safe(pos, tmp, &head, entries)
			free(pos);
	} while (again);
	free(sym_arr);
	str_free(&title);
	list_del(trail.prev);
	str_free(&sttext);
}

static void build_conf(struct menu *menu)
{
	struct symbol *sym;
	struct property *prop;
	struct menu *child;
	int type, tmp, doint = 2;
	tristate val;
	char ch;
	bool visible;

	/*
	 * note: menu_is_visible() has side effect that it will
	 * recalc the value of the symbol.
	 */
	visible = menu_is_visible(menu);
	if (show_all_options && !menu_has_prompt(menu))
		return;
	else if (!show_all_options && !visible)
		return;

	sym = menu->sym;
	prop = menu->prompt;
	if (!sym) {
		if (prop && menu != current_menu) {
			const char *prompt = menu_get_prompt(menu);
			switch (prop->type) {
			case P_MENU:
				child_count++;
				prompt = _(prompt);
				if (single_menu_mode) {
					item_make("%s%*c%s",
						  menu->data ? "-->" : "++>",
						  indent + 1, ' ', prompt);
				} else
					item_make("   %*c%s  %s",
						  indent + 1, ' ', prompt,
						  menu_is_empty(menu) ? "----" : "--->");
				item_set_tag('m');
				item_set_data(menu);
				if (single_menu_mode && menu->data)
					goto conf_childs;
				return;
			case P_COMMENT:
				if (prompt) {
					child_count++;
					item_make("   %*c*** %s ***", indent + 1, ' ', _(prompt));
					item_set_tag(':');
					item_set_data(menu);
				}
				break;
			default:
				if (prompt) {
					child_count++;
					item_make("---%*c%s", indent + 1, ' ', _(prompt));
					item_set_tag(':');
					item_set_data(menu);
				}
			}
		} else
			doint = 0;
		goto conf_childs;
	}

	type = sym_get_type(sym);
	if (sym_is_choice(sym)) {
		struct symbol *def_sym = sym_get_choice_value(sym);
		struct menu *def_menu = NULL;

		child_count++;
		for (child = menu->list; child; child = child->next) {
			if (menu_is_visible(child) && child->sym == def_sym)
				def_menu = child;
		}

		val = sym_get_tristate_value(sym);
		if (sym_is_changable(sym)) {
			switch (type) {
			case S_BOOLEAN:
				item_make("[%c]", val == no ? ' ' : '*');
				break;
			case S_TRISTATE:
				switch (val) {
				case yes: ch = '*'; break;
				case mod: ch = 'M'; break;
				default:  ch = ' '; break;
				}
				item_make("<%c>", ch);
				break;
			}
			item_set_tag('t');
			item_set_data(menu);
		} else {
			item_make("   ");
			item_set_tag(def_menu ? 't' : ':');
			item_set_data(menu);
		}

		item_add_str("%*c%s", indent + 1, ' ', _(menu_get_prompt(menu)));
		if (val == yes) {
			if (def_menu) {
				item_add_str(" (%s)", _(menu_get_prompt(def_menu)));
				item_add_str("  --->");
				if (def_menu->list) {
					indent += 2;
					build_conf(def_menu);
					indent -= 2;
				}
			}
			return;
		}
	} else {
		if (menu == current_menu) {
			item_make("---%*c%s", indent + 1, ' ', _(menu_get_prompt(menu)));
			item_set_tag(':');
			item_set_data(menu);
			goto conf_childs;
		}
		child_count++;
		val = sym_get_tristate_value(sym);
		if (sym_is_choice_value(sym) && val == yes) {
			item_make("   ");
			item_set_tag(':');
			item_set_data(menu);
		} else {
			switch (type) {
			case S_BOOLEAN:
				if (sym_is_changable(sym))
					item_make("[%c]", val == no ? ' ' : '*');
				else
					item_make("-%c-", val == no ? ' ' : '*');
				item_set_tag('t');
				item_set_data(menu);
				break;
			case S_TRISTATE:
				switch (val) {
				case yes: ch = '*'; break;
				case mod: ch = 'M'; break;
				default:  ch = ' '; break;
				}
				if (sym_is_changable(sym)) {
					if (sym->rev_dep.tri == mod)
						item_make("{%c}", ch);
					else
						item_make("<%c>", ch);
				} else
					item_make("-%c-", ch);
				item_set_tag('t');
				item_set_data(menu);
				break;
			default:
				tmp = 2 + strlen(sym_get_string_value(sym)); /* () = 2 */
				item_make("(%s)", sym_get_string_value(sym));
				tmp = indent - tmp + 4;
				if (tmp < 0)
					tmp = 0;
				item_add_str("%*c%s%s", tmp, ' ', _(menu_get_prompt(menu)),
					     (sym_has_value(sym) || !sym_is_changable(sym)) ?
					     "" : _(" (NEW)"));
				item_set_tag('s');
				item_set_data(menu);
				goto conf_childs;
			}
		}
		item_add_str("%*c%s%s", indent + 1, ' ', _(menu_get_prompt(menu)),
			  (sym_has_value(sym) || !sym_is_changable(sym)) ?
			  "" : _(" (NEW)"));
		if (menu->prompt->type == P_MENU) {
			item_add_str("  %s", menu_is_empty(menu) ? "----" : "--->");
			return;
		}
	}

conf_childs:
	indent += doint;
	for (child = menu->list; child; child = child->next)
		build_conf(child);
	indent -= doint;
}

static void conf(struct menu *menu, struct menu *active_menu)
{
	struct menu *submenu;
	const char *prompt = menu_get_prompt(menu);
	struct subtitle_part stpart;
	struct symbol *sym;
	int res;
	int s_scroll = 0;

	if (menu != &rootmenu)
		stpart.text = menu_get_prompt(menu);
	else
		stpart.text = NULL;
	list_add_tail(&stpart.entries, &trail);

	while (1) {
		item_reset();
		current_menu = menu;
		build_conf(menu);
		if (!child_count)
			break;
		set_subtitle();
		dialog_clear();
		res = dialog_menu(prompt ? _(prompt) : _("Main Menu"),
				  _(menu_instructions[g_lang_index]),
				  active_menu, &s_scroll);
		if (res == 1 || res == KEY_ESC || res == -ERRDISPLAYTOOSMALL)
			break;
		if (item_count() != 0) {
			if (!item_activate_selected())
				continue;
			if (!item_tag())
				continue;
		}
		submenu = item_data();
		active_menu = item_data();
		if (submenu)
			sym = submenu->sym;
		else
			sym = NULL;

		switch (res) {
		case 0:
			switch (item_tag()) {
			case 'm':
				if (single_menu_mode)
					submenu->data = (void *) (long) !submenu->data;
				else
					conf(submenu, NULL);
				break;
			case 't':
				if (sym_is_choice(sym) && sym_get_tristate_value(sym) == yes)
					conf_choice(submenu);
				else if (submenu->prompt->type == P_MENU)
					conf(submenu, NULL);
				break;
			case 's':
				conf_string(submenu);
				break;
			}
			break;
		case 2:
			if (sym)
				show_help(submenu);
			else {
				reset_subtitle();
				show_helptext(_("README"), _(mconf_readme[g_lang_index]));
			}
			break;
		case 3:
			reset_subtitle();
			conf_save();
			break;
		case 4:
			reset_subtitle();
			conf_load();
			break;
		case 5:
			if (item_is_tag('t')) {
				if (sym_set_tristate_value(sym, yes))
					break;
				if (sym_set_tristate_value(sym, mod))
					show_textbox(NULL, setmod_text[g_lang_index], 6, 74);
			}
			break;
		case 6:
			if (item_is_tag('t'))
				sym_set_tristate_value(sym, no);
			break;
		case 7:
			if (item_is_tag('t'))
				sym_set_tristate_value(sym, mod);
			break;
		case 8:
			if (item_is_tag('t'))
				sym_toggle_tristate_value(sym);
			else if (item_is_tag('m'))
				conf(submenu, NULL);
			break;
		case 9:
			search_conf();
			break;
		case 10:
			show_all_options = !show_all_options;
			break;
		}
	}

	list_del(trail.prev);
}

static int show_textbox_ext(const char *title, char *text, int r, int c, int
			    *keys, int *vscroll, int *hscroll, update_text_fn
			    update_text, void *data)
{
	dialog_clear();
	return dialog_textbox(title, text, r, c, keys, vscroll, hscroll,
			      update_text, data);
}

static void show_textbox(const char *title, const char *text, int r, int c)
{
	show_textbox_ext(title, (char *) text, r, c, (int []) {0}, NULL, NULL,
			 NULL, NULL);
}

static void show_helptext(const char *title, const char *text)
{
	show_textbox(title, text, 0, 0);
}

static void conf_message_callback(const char *fmt, va_list ap)
{
	char buf[PATH_MAX+1];

	vsnprintf(buf, sizeof(buf), fmt, ap);
	if (save_and_exit) {
		if (!silent)
			printf("%s", buf);
	} else {
		show_textbox(NULL, buf, 6, 60);
	}
}

static void show_help(struct menu *menu)
{
	struct gstr help = str_new();

	help.max_width = getmaxx(stdscr) - 10;
	menu_get_ext_help(menu, &help);

	show_helptext(_(menu_get_prompt(menu)), str_get(&help));
	str_free(&help);
}

static void conf_choice(struct menu *menu)
{
	const char *prompt = _(menu_get_prompt(menu));
	struct menu *child;
	struct symbol *active;

	active = sym_get_choice_value(menu->sym);
	while (1) {
		int res;
		int selected;
		item_reset();

		current_menu = menu;
		for (child = menu->list; child; child = child->next) {
			if (!menu_is_visible(child))
				continue;
			if (child->sym)
				item_make("%s", _(menu_get_prompt(child)));
			else {
				item_make("*** %s ***", _(menu_get_prompt(child)));
				item_set_tag(':');
			}
			item_set_data(child);
			if (child->sym == active)
				item_set_selected(1);
			if (child->sym == sym_get_choice_value(menu->sym))
				item_set_tag('X');
		}
		dialog_clear();
		res = dialog_checklist(prompt ? _(prompt) : _("Main Menu"),
					_(radiolist_instructions[g_lang_index]),
					MENUBOX_HEIGTH_MIN,
					MENUBOX_WIDTH_MIN,
					CHECKLIST_HEIGTH_MIN);
		selected = item_activate_selected();
		switch (res) {
		case 0:
			if (selected) {
				child = item_data();
				if (!child->sym)
					break;

				sym_set_tristate_value(child->sym, yes);
			}
			return;
		case 1:
			if (selected) {
				child = item_data();
				show_help(child);
				active = child->sym;
			} else
				show_help(menu);
			break;
		case KEY_ESC:
			return;
		case -ERRDISPLAYTOOSMALL:
			return;
		}
	}
}

static void conf_string(struct menu *menu)
{
	const char *prompt = menu_get_prompt(menu);

	while (1) {
		int res;
		const char *heading;

		switch (sym_get_type(menu->sym)) {
		case S_INT:
			heading = _(inputbox_instructions_int[g_lang_index]);
			break;
		case S_HEX:
			heading = _(inputbox_instructions_hex[g_lang_index]);
			break;
		case S_STRING:
			heading = _(inputbox_instructions_string[g_lang_index]);
			break;
		default:
			heading = _("Internal mconf error!");
		}
		dialog_clear();
		res = dialog_inputbox(prompt ? _(prompt) : _("Main Menu"),
				      heading, 10, 75,
				      sym_get_string_value(menu->sym));
		switch (res) {
		case 0:
			if (sym_set_string_value(menu->sym, dialog_input_result))
				return;
			show_textbox(NULL, _("You have made an invalid entry."), 5, 43);
			break;
		case 1:
			show_help(menu);
			break;
		case KEY_ESC:
			return;
		}
	}
}

static void conf_load(void)
{

	while (1) {
		int res;
		dialog_clear();
		res = dialog_inputbox(NULL, load_config_text[g_lang_index],
				      11, 55, filename);
		switch(res) {
		case 0:
			if (!dialog_input_result[0])
				return;
			if (!conf_read(dialog_input_result)) {
				set_config_filename(dialog_input_result);
				sym_set_change_count(1);
				return;
			}
			show_textbox(NULL, _("File does not exist!"), 5, 38);
			break;
		case 1:
			show_helptext(_("Load Alternate Configuration"), load_config_help[g_lang_index]);
			break;
		case KEY_ESC:
			return;
		}
	}
}

static void conf_save(void)
{
	while (1) {
		int res;
		dialog_clear();
		res = dialog_inputbox(NULL, save_config_text[g_lang_index],
				      11, 55, filename);
		switch(res) {
		case 0:
			if (!dialog_input_result[0])
				return;
			if (!conf_write(dialog_input_result)) {
				set_config_filename(dialog_input_result);
				return;
			}
			show_textbox(NULL, _("Can't create file!  Probably a nonexistent directory."), 5, 60);
			break;
		case 1:
			show_helptext(_("Save Alternate Configuration"), save_config_help[g_lang_index]);
			break;
		case KEY_ESC:
			return;
		}
	}
}

static const char *save_prom_str[2] = 
{"Do you wish to save your new configuration?\n"
 "(Press <ESC><ESC> to continue configuration.)", 
 "您要保存新配置吗？\n"
 "(按 <ESC> <ESC> 继续配置。)"};

static int handle_exit(void)
{
	int res;

	save_and_exit = 1;
	reset_subtitle();
	dialog_clear();
	if (conf_get_changed())
		res = dialog_yesno(NULL,
				   _(save_prom_str[g_lang_index]),
				   6, 60);
	else
		res = -1;

	end_dialog(saved_x, saved_y);

	switch (res) {
	case 0:
		if (conf_write(filename)) {
			fprintf(stderr, _("\n\n"
					  "Error while writing of the configuration.\n"
					  "Your configuration changes were NOT saved."
					  "\n\n"));
			return 1;
		}
		/* fall through */
	case -1:
		if (!silent)
			printf(_("\n\n"
				 "*** End of the configuration.\n"
				 "*** Execute 'make' to start the build or try 'make help'."
				 "\n\n"));
		res = 0;
		break;
	default:
		if (!silent)
			fprintf(stderr, _("\n\n"
					  "Your configuration changes were NOT saved."
					  "\n\n"));
		if (res != KEY_ESC)
			res = 0;
	}

	return res;
}

static void sig_handler(int signo)
{
	exit(handle_exit());
}

int main(int ac, char **av)
{
	char *mode;
	int res;

#if 0
    if (!strncmp(getenv("LANG"), "zh_CN", strlen("zh_CN")))
        g_lang_index = 1;
    else
        g_lang_index = 0;
#endif

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	signal(SIGINT, sig_handler);

	if (ac > 1 && strcmp(av[1], "-s") == 0) {
		silent = 1;
		/* Silence conf_read() until the real callback is set up */
		conf_set_message_callback(NULL);
		av++;
	}
	conf_parse(av[1]);
	conf_read(NULL);

	mode = getenv("MENUCONFIG_MODE");
	if (mode) {
		if (!strcasecmp(mode, "single_menu"))
			single_menu_mode = 1;
	}

	if (init_dialog(NULL)) {
		fprintf(stderr, N_("Your display is too small to run Menuconfig!\n"));
		fprintf(stderr, N_("It must be at least 19 lines by 80 columns.\n"));
		return 1;
	}

	set_config_filename(conf_get_configname());
	conf_set_message_callback(conf_message_callback);
	do {
		conf(&rootmenu, NULL);
		res = handle_exit();
	} while (res == KEY_ESC);

	return res;
}
