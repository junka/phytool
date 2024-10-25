.PHONY: all clean install dist

# Top directory for building complete system, fall back to this directory
ROOTDIR    ?= $(shell pwd)

VERSION = 2
NAME    = phytool
PKG     = $(NAME)-$(VERSION)
ARCHIVE = $(PKG).tar.xz

PREFIX ?= /usr/local/
CFLAGS ?= -Wall -Wextra -Werror
LDLIBS  = 
MANDIR ?= $(PREFIX)/share/man

objs = $(patsubst %.c, %.o, $(wildcard *.c))
hdrs = $(wildcard *.h)

%.o: %.c $(hdrs) Makefile
	@printf "  CC      $(subst $(ROOTDIR)/,,$(shell pwd)/$@)\n"
	@$(CC) $(CFLAGS) -c $< -o $@

phytool: $(objs)
	@printf "  CC      $(subst $(ROOTDIR)/,,$(shell pwd)/$@)\n"
	@$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

all: phytool

clean:
	@rm -f *.o
	@rm -f $(TARGET)

dist:
	@echo "Creating $(ARCHIVE), with $(ARCHIVE).md5 in parent dir ..."
	@git archive --format=tar --prefix=$(PKG)/ v$(VERSION) | xz >../$(ARCHIVE)
	@(cd .. && md5sum $(ARCHIVE) > $(ARCHIVE).md5)

install: phytool
	@cp phytool $(DESTDIR)/$(PREFIX)/bin/
	@mkdir -p $(DESTDIR)/$(MANDIR)/man8/
	@cp -p phytool.8 -p $(DESTDIR)/$(MANDIR)/man8/
