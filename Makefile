.POSIX:
.SUFFIXES: .c .o

CC = cc # c99 (default value) does not allow POSIX extensions.
PROJECT = slcl
O = -Og
CFLAGS = $(O) -g -Wall -Idynstr/include -MD -MF -
LIBS = -lcjson -lssl -lm -lcrypto
LDFLAGS = $(LIBS)
DEPS = $(OBJECTS:.o=.d)
OBJECTS = \
	auth.o \
	base64.o \
	cftw.o \
	handler.o \
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
