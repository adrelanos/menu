SUBDIRS = update-menus install-menu doc scripts po po-sections po-su-to-root
EXTRA_DIST = autogen.sh Doxyfile
dist-hook:
	mkdir $(distdir)/examples $(distdir)/default
	for i in $(srcdir)/examples/*; do \
	   test -f $$i && cp $$i $(distdir)/examples; done
	for i in $(srcdir)/default/*; do \
	   test -f $$i && cp $$i $(distdir)/default; done
