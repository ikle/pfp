TARGETS = pfp

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)

PREFIX ?= /usr/local

install: $(TARGETS)
	install -D -d $(DESTDIR)/$(PREFIX)/bin
	install -s -m 0755 $^ $(DESTDIR)/$(PREFIX)/bin

pfp: CFLAGS += `pkg-config libpci --cflags --libs`
pfp: pfp-scanner.o pfp-parser.o pfp-rule.o
