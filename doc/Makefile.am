all-local: menu.html/ menu.txt.gz menu.info.gz

DOCS = announce-2 BUGS cmap.xpm menu.sgml README.changes README.config-menus README.menu-methods README.new-sections README.package-menus README.pre1 TODO translate

man_MANS = install-menu.1 menufile.5 su-to-root.1 update-menus.1 wm-menu-config.1
EXTRA_DIST = $(man_MANS) $(DOCS) menu.sgml $(srcdir)/menu.html $(srcdir)/menu.txt.gz $(srcdir)/menu.info.gz 

menu.html/: menu.sgml
	debiandoc2html menu.sgml

%.info: %.sgml
	debiandoc2info $<

%.txt: %.sgml
	debiandoc2text -O $< > $@

%.gz:   %
	gzip -f9 $<

clean-local: 
	rm -rf menu.html *.info* *.dvi* *.txt*

install-data-local:
	$(INSTALL) -d \
	     $(DESTDIR)$(datadir)/doc/menu/examples \
	     $(DESTDIR)$(datadir)/doc/menu/html \
	     $(DESTDIR)$(pkgdatadir)/default \
	     $(DESTDIR)$(infodir)
	$(INSTALL_DATA) `find $(top_srcdir)/examples/* -type f` $(DESTDIR)$(datadir)/doc/menu/examples
	$(INSTALL_DATA) `find $(top_srcdir)/default/* -type f` $(DESTDIR)$(pkgdatadir)/default
	$(INSTALL_DATA) $(srcdir)/menu.html/* $(DESTDIR)$(datadir)/doc/menu/html
	$(INSTALL_DATA) $(srcdir)/menu.txt.gz $(DESTDIR)$(datadir)/doc/menu/
	$(INSTALL_DATA) $(srcdir)/menu.info.gz $(DESTDIR)$(infodir)

	$(INSTALL_DATA) \
	   $(srcdir)/menu.sgml $(srcdir)/README.changes $(srcdir)/BUGS $(DESTDIR)$(datadir)/doc/menu/
uninstall-local:
	rm -r $(DESTDIR)$(datadir)/doc/menu/
	rm -r $(DESTDIR)$(pkgdatadir)/default
	rm    $(DESTDIR)$(infodir)/menu.info.gz
