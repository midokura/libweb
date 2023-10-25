.POSIX:

PROJECT = libweb
PROJECT_A = $(PROJECT).a
MAJOR_VERSION = 0
MINOR_VERSION = 1
PATCH_VERSION = 0
VERSION = $(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_VERSION)
PROJECT_SO = $(PROJECT).so.$(VERSION)
PROJECT_SO_FQ = $(PROJECT).so.$(MAJOR_VERSION)
PROJECT_SO_NV = $(PROJECT).so
PREFIX = /usr/local
DST = $(PREFIX)/lib
PC_DST = $(DST)/pkgconfig
O = -Og
CDEFS = -D_FILE_OFFSET_BITS=64 # Required for large file support on 32-bit.
CFLAGS = $(O) $(CDEFS) -g -Iinclude -Idynstr/include -fPIC -MD -MF $(@:.o=.d)
LDFLAGS = -shared
DEPS = $(OBJECTS:.o=.d)
OBJECTS = \
	handler.o \
	html.o \
	http.o \
	server.o \
	wildcard_cmp.o

all: $(PROJECT_A) $(PROJECT_SO)

install: all $(PC_DST)/libweb.pc
	mkdir -p $(PREFIX)/include
	cp -R include/libweb $(PREFIX)/include
	chmod 0644 $(PREFIX)/include/libweb/*.h
	mkdir -p $(DST)
	cp $(PROJECT_A) $(PROJECT_SO) $(DST)
	chmod 0755 $(DST)/$(PROJECT_A) $(DST)/$(PROJECT_SO)
	ln -fs $(DST)/$(PROJECT_SO) $(DST)/$(PROJECT_SO_FQ)
	ln -fs $(DST)/$(PROJECT_SO) $(DST)/$(PROJECT_SO_NV)
	+cd doc && $(MAKE) PREFIX=$(PREFIX) install

clean:
	rm -f $(OBJECTS) $(DEPS)
	+cd examples && $(MAKE) clean

FORCE:

examples: FORCE
	+cd examples && $(MAKE)

$(PROJECT_A): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(PROJECT_SO): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(PC_DST)/libweb.pc: libweb.pc
	mkdir -p $(PC_DST)
	sed -e 's,/usr/local,$(PREFIX),' $< > $@
	chmod 0644 $@

-include $(DEPS)
