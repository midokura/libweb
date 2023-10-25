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
prefix = /usr/local
exec_prefix = $(prefix)
includedir = $(prefix)/include
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
libdir = $(exec_prefix)/lib
pkgcfgdir = $(libdir)/pkgconfig
O = -O1
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

install: all $(pkgcfgdir)/libweb.pc
	mkdir -p $(DESTDIR)$(includedir)
	cp -R include/libweb $(DESTDIR)$(includedir)
	chmod 0644  $(DESTDIR)$(includedir)/libweb/*.h
	mkdir -p $(DESTDIR)$(libdir)
	cp $(PROJECT_A) $(PROJECT_SO) $(DESTDIR)$(libdir)
	chmod 0755 $(libdir)/$(PROJECT_A) $(DESTDIR)$(libdir)/$(PROJECT_SO)
	ln -fs $(libdir)/$(PROJECT_SO) $(DESTDIR)$(libdir)/$(PROJECT_SO_FQ)
	ln -fs $(libdir)/$(PROJECT_SO) $(DESTDIR)$(libdir)/$(PROJECT_SO_NV)
	+cd doc && $(MAKE) install \
		DESTDIR=$(DESTDIR) \
		prefix=$(prefix) \
		datarootdir=$(datarootdir) \
		mandir=$(mandir)

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

$(pkgcfgdir)/libweb.pc: libweb.pc
	mkdir -p $(pkgcfgdir)
	sed -e 's,/usr/local,$(prefix),' $< > $@
	chmod 0644 $@

-include $(DEPS)
