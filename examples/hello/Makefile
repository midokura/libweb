.POSIX:

PROJECT = hello
DEPS = \
	main.o
SLWEB = ../../libslweb.a
DYNSTR = ../../dynstr/libdynstr.a
CFLAGS = -I ../../include -I ../../dynstr/include
SLWEB_FLAGS = -L ../../ -l slweb
DYNSTR_FLAGS = -L ../../dynstr -l dynstr

all: $(PROJECT)

clean:
	rm -f $(DEPS)

FORCE:

$(PROJECT): $(DEPS) $(SLWEB) $(DYNSTR)
	$(CC) $(LDFLAGS) $(DEPS) $(SLWEB_FLAGS) $(DYNSTR_FLAGS) -o $@

$(SLWEB): FORCE
	+cd ../../ && $(MAKE)

$(DYNSTR): FORCE
	+cd ../../dynstr && $(MAKE)
