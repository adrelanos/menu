MOs = ca.mo cs.mo da.mo de.mo eo.mo el.mo es.mo fi.mo fr.mo he.mo hr.mo hu.mo it.mo ja.mo lt.mo nl.mo nn.mo pl.mo pt_BR.mo ru.mo sv.mo tr.mo uk.mo zh_CN.mo
POs = ca.po cs.po da.po de.po eo.po el.po es.po fi.po fr.po he.po hr.po hu.po it.po ja.po lt.po nl.po nn.po pl.po pt_BR.po ru.po sv.po tr.po uk.po zh_CN.po
localedir = $(datadir)/locale

EXTRA_DIST = $(POs) es.trme menufile2pot menu-sections.pot README trme2po

all-local: $(MOs)
install-data-local:$(MOs) lang.h
	for mo in $(MOs); do \
           lang=`basename $$mo .mo`; \
	   dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
	   $(INSTALL) -d $$dir && \
	   $(INSTALL_DATA) $$mo $$dir/menu-sections.mo; \
	done

lang.h: $(MOs)
	echo "#List of languages for multilingual menu" >lang.h
	echo >>lang.h
	echo 'function sections_translations()="$(MOs) "' \
	  | sed -e 's/.mo /:/g; s/:"/"/' >>lang.h

%.mo   : %.po
	msgfmt -o $@ $^ 

clean-local:
	rm -f *.mo lang.h
