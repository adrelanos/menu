MOs = @MO_SECTIONS@
POs = @PO_SECTIONS@
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
