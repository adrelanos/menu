MOs = da.mo pl.mo
POs = da.po pl.po
localedir = $(datadir)/locale

EXTRA_DIST = $(POs)

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

menu.pot::
	xgettext -o menu.pot -C -k_ -c ../install-menu/*.[ch]* ../update-menus/*.[ch]* --msgid-bugs-address=menu@packages.debian.org --copyright-holder="Debian menu team"

clean-local:
	rm -f *.mo
