.DEFAULT_GOAL := test
CC ?= cc
CFLAGS ?= -O2
BRIDGE = ../../bar/bridge
UNIT_SOURCES = unit/main.c unit/commands.c unit/memory.c unit/state.c unit/transport.c
BRIDGE_SOURCES = command/parse.c memory/meminfo.c state/frame.c x11/command.c
OBJECTS = $(UNIT_SOURCES:%.c=build/%.o) $(BRIDGE_SOURCES:%.c=build/bridge/%.o)
DEPS = $(OBJECTS:.o=.d)
TEST_FLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude -I$(BRIDGE)/include \
             -std=c99 -Wall -Wextra -Wpedantic

.PHONY: test clean
test: build/unit-tests
	./build/unit-tests

# No libX11 link: the transport tests provide XSendEvent and XSync stubs.
build/unit-tests: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

build/bridge/%.o: $(BRIDGE)/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(TEST_FLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(TEST_FLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build

-include $(DEPS)
