MOs = ca.mo cs.mo da.mo de.mo eo.mo es.mo fi.mo fr.mo hu.mo it.mo ja.mo nl.mo pl.mo pt_BR.mo zh_CN.mo
POs = ca.po cs.po da.po de.po eo.po es.po fi.po fr.po hu.po it.po ja.po nl.po pl.po pt_BR.po zh_CN.po
localedir = $(datadir)/locale

EXTRA_DIST = $(POs) es.trme menufile2pot menu-sections.pot README trme2po

all-local: $(MOs)
install-data-local:$(MOs)
	for mo in $(MOs); do \
           lang=`basename $$mo .mo`; \
	   dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
	   $(INSTALL) -d $$dir && \
	   $(INSTALL_DATA) $$mo $$dir/menu-sections.mo; \
	done

%.mo   : %.po
	msgfmt -o $@ $^ 

clean-local:
	rm -f *.mo
