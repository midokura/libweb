.POSIX:

PROJECT = headers
DEPS = \
	main.o
LIBWEB = ../../libweb.a
DYNSTR = ../../dynstr/libdynstr.a
CFLAGS = -I ../../include -I ../../dynstr/include
LIBWEB_FLAGS = -L ../../ -l web
DYNSTR_FLAGS = -L ../../dynstr -l dynstr

all: $(PROJECT)

clean:
	rm -f $(DEPS)

FORCE:

$(PROJECT): $(DEPS) $(LIBWEB) $(DYNSTR)
	$(CC) $(LDFLAGS) $(DEPS) $(LIBWEB_FLAGS) $(DYNSTR_FLAGS) -o $@

$(LIBWEB): FORCE
	+cd ../../ && $(MAKE)

$(DYNSTR): FORCE
	+cd ../../dynstr && $(MAKE)
