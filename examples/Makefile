.POSIX:

all: \
	hello \
	html

clean:
	+cd hello && $(MAKE) clean
	+cd html && $(MAKE) clean

FORCE:

hello: FORCE
	+cd hello && $(MAKE)

html: FORCE
	+cd html && $(MAKE)
