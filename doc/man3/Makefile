.POSIX:

prefix = /usr/local
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
man3dir = $(mandir)/man3
OBJECTS = \
	$(DESTDIR)$(man3dir)/handler_add.3 \
	$(DESTDIR)$(man3dir)/handler_alloc.3 \
	$(DESTDIR)$(man3dir)/handler_free.3 \
	$(DESTDIR)$(man3dir)/handler_listen.3 \
	$(DESTDIR)$(man3dir)/handler_loop.3 \
	$(DESTDIR)$(man3dir)/html_node_add_attr.3 \
	$(DESTDIR)$(man3dir)/html_node_add_child.3 \
	$(DESTDIR)$(man3dir)/html_node_add_sibling.3 \
	$(DESTDIR)$(man3dir)/html_node_alloc.3 \
	$(DESTDIR)$(man3dir)/html_node_free.3 \
	$(DESTDIR)$(man3dir)/html_node_set_value.3 \
	$(DESTDIR)$(man3dir)/html_node_set_value_unescaped.3 \
	$(DESTDIR)$(man3dir)/html_serialize.3 \
	$(DESTDIR)$(man3dir)/http_alloc.3 \
	$(DESTDIR)$(man3dir)/http_cookie_create.3 \
	$(DESTDIR)$(man3dir)/http_decode_url.3 \
	$(DESTDIR)$(man3dir)/http_encode_url.3 \
	$(DESTDIR)$(man3dir)/http_free.3 \
	$(DESTDIR)$(man3dir)/http_response_add_header.3 \
	$(DESTDIR)$(man3dir)/http_update.3

all:

install: $(OBJECTS)

$(DESTDIR)$(man3dir)/%.3: %.3
	mkdir -p $(DESTDIR)$(man3dir)
	cp $< $@
	chmod 0644 $@
