AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = garapon
garapon_SOURSES = garapon.c
garapon_CFLAGS = -Wall -pipe -fstack-protector-strong

EXTRA_DIST = README

bonnou-arice:
	@test ! -f $(srcdir)/BONNOU && : >$(srcdir)/BONNOU;

bonnou-clean:
	@test -f $(srcdir)/BONNOU && rm -f $(srcdir)/BONNOU;
