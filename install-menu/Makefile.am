INCLUDES = -I$(top_srcdir)/update-menus

sbin_PROGRAMS = install-menu

install_menu_LDADD = ../update-menus/adstring.o
install_menu_SOURCES = install-menu.h install-menu.cc menu-tree.h menu-tree.cc hints.h hints.cc 

EXTRA_DIST = hintstest.cc

install-menu.cc: ../update-menus/adstring.h ../update-menus/common.h ../update-menus/compose.hpp

install-exec-local:
	ln -s ../sbin/install-menu $(DESTDIR)$(bindir)/install-menu

