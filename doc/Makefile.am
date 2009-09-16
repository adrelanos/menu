all-local: menu.html/index.html menu.txt.gz menu.info.gz

DOCS = announce-2 BUGS README.changes README.config-menus README.menu-methods README.new-sections README.package-menus README.pre1 TODO translate README.translations menu.sgml menu.direntry

man_MANS = install-menu.1 menufile.5 su-to-root.1 update-menus.1
EXTRA_DIST = $(man_MANS) $(DOCS) $(srcdir)/menu.html $(srcdir)/menu.txt.gz $(srcdir)/menu.info.gz update-menus.fr.1 su-to-root.fr.1 menufile.fr.5

docdir=$(datadir)/doc
pkgdocdir=$(docdir)/menu

menu.html/index.html: menu.sgml
	LANG=C debiandoc2html $<

menu-one-file.html: menu.sgml
	LANG=C debiandoc2html -1 -b menu-1-file $<
	mv menu-1-file.html/index.html menu-one-file.html
	rmdir menu-1-file.html

menu-one-file: menu-one-file.html

%.info: %.sgml %.direntry
	LANG=C debiandoc2info -s ./$*.direntry $<

%.txt: %.sgml
	LANG=C debiandoc2text -O $< > $@

%.gz:   %
	gzip --best --force --stdout $< > $@
clean-local:
	rm -f menu-one-file.html

maintainer-clean-local: 
	rm -rf menu.html *.info* *.txt*

install-default:
	$(INSTALL) -d \
	     $(DESTDIR)$(pkgdatadir)/default
	for i in $(top_srcdir)/default/*; do \
	   test -f $$i && $(INSTALL_DATA) $$i $(DESTDIR)$(pkgdatadir)/default;\
        done;

install-data-local:
	$(INSTALL) -d \
	     $(DESTDIR)$(pkgdocdir)/examples \
	     $(DESTDIR)$(pkgdocdir)/html \
	     $(DESTDIR)$(infodir)
             
	for i in $(top_srcdir)/examples/*; do \
	   test -f $$i && $(INSTALL_DATA) $$i $(DESTDIR)$(pkgdocdir)/examples;\
        done;

	$(INSTALL_DATA) $(srcdir)/menu.html/* $(DESTDIR)$(pkgdocdir)/html
	$(INSTALL_DATA) $(srcdir)/menu.txt.gz $(DESTDIR)$(pkgdocdir)
	$(INSTALL_DATA) $(srcdir)/menu.info.gz $(DESTDIR)$(infodir)
	$(INSTALL_DATA) $(srcdir)/menu.sgml $(DESTDIR)$(pkgdocdir)

	$(INSTALL_DATA) $(srcdir)/BUGS $(DESTDIR)$(pkgdocdir)
	$(INSTALL_DATA) $(srcdir)/README.translations $(DESTDIR)$(pkgdocdir)
	$(INSTALL_DATA) $(srcdir)/../README $(DESTDIR)$(pkgdocdir)
        
uninstall-local:
	rm -r $(DESTDIR)$(pkgdocdir)
	rm -r $(DESTDIR)$(pkgdatadir)/default
	rm    $(DESTDIR)$(infodir)/menu.info.gz
