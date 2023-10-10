.POSIX:

PREFIX = /usr/local
DST = $(PREFIX)/share/man/man7
OBJECTS = \
	$(DST)/libweb_handler.7 \
	$(DST)/libweb_html.7 \
	$(DST)/libweb_http.7

all:

install: $(OBJECTS)

$(DST)/%.7: %.7
	mkdir -p $(DST)
	cp $< $@
	chmod 0644 $@
