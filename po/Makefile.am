MOs = cs.mo da.mo de.mo el.mo es.mo fi.mo fr.mo gl.mo it.mo ja.mo nl.mo nb.mo nn.mo pl.mo pt.mo pt_BR.mo ru.mo sv.mo tr.mo zh_CN.mo zh_TW.mo
POs = cs.po da.po de.po el.po es.po fi.po fr.po gl.po it.po ja.po nl.po nb.po nn.po pl.po pt.po pt_BR.po ru.po sv.po tr.po zh_CN.po zh_TW.po
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
