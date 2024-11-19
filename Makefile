.PHONY: all clean install dist

# Top directory for building complete system, fall back to this directory
ROOTDIR    ?= $(shell pwd)

VERSION = 2
NAME    = phytool
PKG     = $(NAME)-$(VERSION)
ARCHIVE = $(PKG).tar.xz

SYS_ROOT=../recipe-sysroot
CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-
CC=${CROSS_COMPILE}gcc
CXX=$(CROSS_COMPILE)g++
LD=${CROSS_COMPILE}ld
AR=$(CROSS_COMPILE)ar
ASAN_FLAGS=
#-fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address -static-libasan
PREFIX ?= /usr/local/
CFLAGS ?= -Wall -Wextra -Werror -g ${ASAN_FLAGS}
LDFLAGS ?= -fPIC -L/usr/local/lib ${ASAN_FLAGS}
LDLIBS  = -lyaml
MANDIR ?= $(PREFIX)/share/man
ifneq (${CROSS_COMPILE}, )
	CFLAGS := ${CFLAGS} --sysroot=${SYS_ROOT}
	LDFLAGS := ${LDFLAGS} --sysroot=${SYS_ROOT}
endif

objs = $(patsubst %.c, %.o, $(wildcard *.c))
hdrs = $(wildcard *.h)

%.o: %.c $(hdrs) Makefile
	@printf "  CC        $(subst $(ROOTDIR)/,,$(shell pwd)/$@)\n"
	@$(CC) $(CFLAGS) -c $< -o $@

phytool: $(objs)
	@printf "  LINK      $(subst $(ROOTDIR)/,,$(shell pwd)/$@)\n"
	@$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

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
