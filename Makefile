.POSIX:

PROJECT = libslweb.a
PREFIX = /usr/local
DST = $(PREFIX)/lib
PC_DST = $(DST)/pkgconfig
O = -Og
CDEFS = -D_FILE_OFFSET_BITS=64 # Required for large file support on 32-bit.
CFLAGS = $(O) $(CDEFS) -g -Iinclude -Idynstr/include -MD -MF $(@:.o=.d)
DEPS = $(OBJECTS:.o=.d)
OBJECTS = \
	handler.o \
	html.o \
	http.o \
	server.o \
	wildcard_cmp.o

all: $(PROJECT)

install: all $(PC_DST)/slweb.pc
	mkdir -p $(PREFIX)/include
	cp -R include/slweb $(PREFIX)/include
	chmod 0644 $(PREFIX)/include/slweb/*.h
	mkdir -p $(DST)
	cp $(PROJECT) $(DST)
	chmod 0755 $(DST)/$(PROJECT)

clean:
	rm -f $(OBJECTS) $(DEPS)
	+cd examples && $(MAKE) clean

FORCE:

examples: FORCE
	+cd examples && $(MAKE)

$(PROJECT): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(PC_DST)/slweb.pc: slweb.pc
	mkdir -p $(PC_DST)
	sed -e 's,/usr/local,$(PREFIX),' $< > $@
	chmod 0644 $@

-include $(DEPS)
