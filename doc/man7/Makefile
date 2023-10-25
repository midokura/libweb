.POSIX:

prefix = /usr/local
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
man7dir = $(mandir)/man7
OBJECTS = \
	$(DESTDIR)$(man7dir)/libweb_handler.7 \
	$(DESTDIR)$(man7dir)/libweb_html.7 \
	$(DESTDIR)$(man7dir)/libweb_http.7

all:

install: $(OBJECTS)

$(DESTDIR)$(man7dir)/%.7: %.7
	mkdir -p $(DESTDIR)$(man7dir)
	cp $< $@
	chmod 0644 $@
