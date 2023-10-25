.POSIX:

prefix = /usr/local
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man

all:

install: all
	+cd man3 && $(MAKE) install \
		DESTDIR=$(DESTDIR) \
		prefix=$(prefix) \
		datarootdir=$(datarootdir) \
		mandir=$(mandir)
	+cd man7 && $(MAKE) install \
		DESTDIR=$(DESTDIR) \
		prefix=$(prefix) \
		datarootdir=$(datarootdir) \
		mandir=$(mandir)
