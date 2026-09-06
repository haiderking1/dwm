.DEFAULT_GOAL := all
CC ?= cc
CFLAGS ?= -O2
PREFIX ?= /home/soka/.local
DESTDIR ?=

SOURCES = app/main.c command/parse.c memory/meminfo.c state/frame.c \
          x11/command.c x11/watch.c x11/wallpaper.c
OBJECTS = $(SOURCES:%.c=build/%.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean test install
all: dwm-bar-bridge

dwm-bar-bridge: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) -lX11 $(LDLIBS)

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -D_POSIX_C_SOURCE=200809L -Iinclude $(CFLAGS) \
		-std=c99 -Wall -Wextra -Wpedantic -MMD -MP -c $< -o $@

test:
	$(MAKE) -C ../../tests/bar-bridge test CC="$(CC)" CFLAGS="$(CFLAGS)"

# Stage on the destination filesystem, then rename rather than truncate an
# executable that may already be running. No installation during normal builds.
install: dwm-bar-bridge
	@set -eu; \
	dir="$(DESTDIR)$(PREFIX)/bin"; \
	mkdir -p "$$dir"; \
	tmp=$$(mktemp "$$dir/.dwm-bar-bridge.XXXXXX"); \
	trap 'rm -f "$$tmp"' 0; \
	trap 'exit 1' 1 2 3 15; \
	install -m 755 dwm-bar-bridge "$$tmp"; \
	mv -f "$$tmp" "$$dir/dwm-bar-bridge"

clean:
	rm -rf build dwm-bar-bridge

-include $(DEPS)
