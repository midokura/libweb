.POSIX:

PREFIX = /usr/local
DST = $(PREFIX)/share/man/man7
OBJECTS = \
	$(DST)/slweb_handler.7 \
	$(DST)/slweb_html.7 \
	$(DST)/slweb_http.7

all:

install: $(OBJECTS)

$(DST)/%.7: %.7
	mkdir -p $(DST)
	cp $< $@
	chmod 0644 $@
