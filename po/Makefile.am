MOs = eo.mo
localedir = $(datadir)/locale

EXTRA_DIST = eo.po

all-local: $(MOs)
install-data-local:$(MOs)
	for mo in $(MOs); do \
           lang=`basename $$mo .mo`; \
	   dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
	   $(INSTALL) -d $$dir && \
	   $(INSTALL_DATA) $$mo $$dir/menu.mo; \
	done

%.mo   : %.po
	msgfmt -o $@ $^ 

menu.pot:
	xgettext -o menu.pot -C -k_ ../install-menu/*.[ch]* ../update-menus/*.[ch]*

clean-local:
	rm -f *.mo
