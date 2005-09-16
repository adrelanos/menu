bin_SCRIPTS = su-to-root
EXTRA_DIST = $(bin_SCRIPTS)

install-exec-local:
	test -d "$(DESTDIR)$(sbindir)" || mkdir "$(DESTDIR)$(sbindir)"
	ln -f -s ../bin/su-to-root $(DESTDIR)$(sbindir)/su-to-root
