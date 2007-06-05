MOs = @MO_SUTOROOT@
POs = @PO_SUTOROOT@
localedir = $(datadir)/locale

EXTRA_DIST = $(POs)

all-local: $(MOs)
install-data-local:$(MOs)
	for mo in $(MOs); do \
           lang=`basename $$mo .mo`; \
	   dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
	   $(INSTALL) -d $$dir && \
	   $(INSTALL_DATA) $$mo $$dir/su-to-root.mo; \
	done

%.mo   : %.po
	msgfmt -o $@ $^ 

su-to-root.pot::
	xgettext -o su-to-root.pot -LShell -k -ktransl --msgid-bugs-address=menu@packages.debian.org --copyright-holder="Debian menu team" ../scripts/su-to-root

clean-local:
	rm -f *.mo
