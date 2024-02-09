TARGETS = pfp pfp-convert

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)

PREFIX ?= /usr/local

install: $(TARGETS)
	install -D -d $(DESTDIR)/$(PREFIX)/bin
	install -s -m 0755 $^ $(DESTDIR)/$(PREFIX)/bin

pfp: CFLAGS += `pkg-config libpci --cflags`
pfp: LDLIBS += `pkg-config libpci --libs`
pfp: pfp-scanner.o pfp-parser.o pfp-rule.o pfp-rule-fill.o
