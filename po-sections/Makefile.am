MOs = eo.mo es.mo fr.mo hu.mo ja.mo pl.mo 
localedir = $(datadir)/locale

EXTRA_DIST = eo.po es.trme fr.po ja.po menufile2pot menu-sections.pot README trme2po

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

%.po   : %.trme
	$(srcdir)/trme2po <  $^  >  $@

clean-local:
	rm -f *.mo es.po
