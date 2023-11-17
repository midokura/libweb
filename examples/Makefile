.POSIX:

all: \
	headers \
	hello \
	html

clean:
	+cd hello && $(MAKE) clean
	+cd html && $(MAKE) clean

FORCE:

headers: FORCE
	+cd headers && $(MAKE)

hello: FORCE
	+cd hello && $(MAKE)

html: FORCE
	+cd html && $(MAKE)
