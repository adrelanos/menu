INCLUDES = -I$(top_srcdir)/update-menus

sbin_PROGRAMS = install-menu

install_menu_LDADD = ../update-menus/stringtoolbox.o ../update-menus/parsestream.o
install_menu_SOURCES = install-menu.h install-menu.cc menu-tree.h menu-tree.cc hints.h hints.cc functions.cc functions.h

EXTRA_DIST = hintstest.cc

install-exec-local:
	ln -s ../sbin/install-menu $(DESTDIR)$(bindir)/install-menu

