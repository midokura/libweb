.POSIX:
.SUFFIXES: .c .o

PROJECT = slcl
O = -Og
CDEFS = -D_FILE_OFFSET_BITS=64 # Required for large file support on 32-bit.
CFLAGS = $(O) $(CDEFS) -g -Wall -Idynstr/include -MD -MF -
LIBS = -lcjson -lssl -lm -lcrypto
LDFLAGS = $(LIBS)
DEPS = $(OBJECTS:.o=.d)
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
	dynstr/dynstr.o

all: $(PROJECT)

clean:
	rm -f $(OBJECTS) $(DEPS)

$(PROJECT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@ > $(@:.o=.d)

-include $(DEPS)
