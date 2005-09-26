MOs = bg.mo ca.mo cs.mo da.mo de.mo eo.mo eu.mo el.mo es.mo fi.mo fr.mo gl.mo he.mo hr.mo hu.mo id.mo it.mo ja.mo lt.mo nl.mo nb.mo nn.mo pl.mo pt.mo pt_BR.mo ro.mo ru.mo sk.mo sv.mo tr.mo uk.mo vi.mo zh_CN.mo zh_TW.mo
POs = bg.po ca.po cs.po da.po de.po eo.po eu.po el.po es.po fi.po fr.po gl.po he.po hr.po hu.po id.po it.po ja.po lt.po nl.po nb.po nn.po pl.po pt.po pt_BR.po ro.po ru.po sk.po sv.po tr.po uk.po vi.po zh_CN.po zh_TW.po
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
