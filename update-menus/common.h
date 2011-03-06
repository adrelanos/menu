//   -*- mode: c++; -*-
//
#ifndef COMMON_H
#define COMMON_H

#include <libintl.h>

const int MAX_LINE = 10240;

#define _(string) gettext(string)
#define LOCALEDIR "/usr/share/locale"

#define COMMAND_VAR "command"
#define ICON_VAR    "icon"
#define NEEDS_VAR   "needs"
#define SECTION_VAR "section"
#define BASESECTION_VAR "basesection"
#define SORT_VAR    "sort"
#define TITLE_VAR   "title"
#define HOTKEY_VAR  "hotkey"
#define HINTS_VAR   "hints"
#define PACKAGE_VAR "package"
#define PRIVATE_ENTRYCOUNT_VAR "__PRIVATE__ENTRYCOUNT"
#define PRIVATE_ENTRYINDEX_VAR "__PRIVATE__ENTRYINDEX"
#define PRIVATE_LEVEL_VAR "__PRIVATE__LEVEL"

//conditionals in the menuentry files (like "package(vi)")
#define COND_PACKAGE "package"

#define CONFIGMENUS     "/etc/menu"
#define PACKAGEMENUSLIB "/usr/lib/menu"
#define PACKAGEMENUS    "/usr/share/menu"
#define MENUMENUS       "/usr/share/menu/default"
#define USERMENUS       ".menu"

#define MENUMETHODS     "/etc/menu-methods"
#define USERMETHODS     ".menu-methods"

#define UPMEN_LOCKFILE  "/var/run/update-menus.pid"
#define DPKG_LOCKFILE   "/var/lib/dpkg/lock"
#define TRANSLATE_FILE  "/etc/menu-methods/translate_menus"
#define USERTRANSLATE   ".menu-methods/translate_menus"
#define CONFIG_FILE     "/etc/menu-methods/menu.config"
#define USERCONFIG      ".menu-methods/menu.config"


#define TRANSLATE_TRANS    "translate"
#define SUBSTITUTE_TRANS   "substitute"
#define SUBTRANSLATE_TRANS "subtranslate"
#define ENDTRANSLATE_TRANS "endtranslate"

#endif /* COMMON_H */
