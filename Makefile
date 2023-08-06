.POSIX:

PROJECT = libslweb.a
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

clean:
	rm -f $(OBJECTS) $(DEPS)
	+cd examples && $(MAKE) clean

FORCE:

examples: FORCE
	+cd examples && $(MAKE)

$(PROJECT): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

-include $(DEPS)
