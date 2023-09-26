.POSIX:

PREFIX = /usr/local
DST = $(PREFIX)/share/man/man3
OBJECTS = \
	$(DST)/handler_add.3 \
	$(DST)/handler_alloc.3 \
	$(DST)/handler_free.3 \
	$(DST)/handler_listen.3 \
	$(DST)/html_node_add_attr.3 \
	$(DST)/html_node_add_child.3 \
	$(DST)/html_node_add_sibling.3 \
	$(DST)/html_node_alloc.3 \
	$(DST)/html_node_free.3 \
	$(DST)/html_node_set_value.3 \
	$(DST)/html_node_set_value_unescaped.3 \
	$(DST)/html_serialize.3 \
	$(DST)/http_alloc.3 \
	$(DST)/http_cookie_create.3 \
	$(DST)/http_encode_url.3 \
	$(DST)/http_free.3 \
	$(DST)/http_response_add_header.3 \
	$(DST)/http_update.3

all:

install: $(OBJECTS)

$(DST)/%.3: %.3
	mkdir -p $(DST)
	cp $< $@
	chmod 0644 $@
