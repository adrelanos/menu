INCLUDES = -I$(top_srcdir)/update-menus

bin_PROGRAMS = install-menu

install_menu_LDADD = ../update-menus/stringtoolbox.o ../update-menus/parsestream.o
install_menu_SOURCES = install-menu.h install-menu.cc menu-tree.h menu-tree.cc hints.h hints.cc functions.cc functions.h

install-exec-local:
	test -d "$(DESTDIR)$(sbindir)" || mkdir "$(DESTDIR)$(sbindir)"
	ln -f -s ../bin/install-menu $(DESTDIR)$(sbindir)/install-menu

