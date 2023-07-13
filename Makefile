.POSIX:

PROJECT = slcl
O = -Og
CDEFS = -D_FILE_OFFSET_BITS=64 # Required for large file support on 32-bit.
CFLAGS = $(O) $(CDEFS) -g -Wall -Idynstr/include -MD -MF $(@:.o=.d)
LIBS = -lcjson -lssl -lm -lcrypto
DEPS = $(OBJECTS:.o=.d)
DYNSTR = dynstr/libdynstr.a
DYNSTR_FLAGS = -Ldynstr -ldynstr
OBJECTS = \
	auth.o \
	base64.o \
	cftw.o \
	handler.o \
	hex.o \
	html.o \
	http.o \
	jwt.o \
	main.o \
	page.o \
	server.o \
	style.o \
	wildcard_cmp.o \

all: $(PROJECT)

clean:
	rm -f $(OBJECTS) $(DEPS)

$(PROJECT): $(OBJECTS) $(DYNSTR)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LIBS) $(DYNSTR_FLAGS) -o $@

$(DYNSTR):
	+cd dynstr && $(MAKE)

-include $(DEPS)
