MOs = eo.mo es.mo fr.mo hu.mo ja.mo pl.mo pt_BR.mo
POs = eo.po es.po fr.po hu.po ja.po pl.po pt_BR.po
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
