# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c dwm.c util.c bar/wm/geometry.c bar/wm/json.c input/settings.c startup/autostart.c reload/build.c $(wildcard wm/bsp/core/*.c)
OBJ = ${SRC:.c=.o}

all: dwm bar-bridge

bar-bridge:
	${MAKE} -C bar/bridge

.c.o:
	${CC} -c ${CFLAGS} -o $@ $<

${OBJ}: config.h config.mk config/commands.h config/keys.h
dwm.o: input/settings.h wm/fullscreen.h startup/autostart.h config/autostart.h config/reload.h reload/build.h reload/state.h reload/save.inc reload/restore.inc reload/control.inc
dwm.o: wm/geometry/request.inc wm/mouse/events.inc
dwm.o: wm/drag/placement.inc wm/drag/mouse.inc wm/drag/bsp.inc
dwm.o: wm/bsp/bsp.h wm/bsp/hooks.h $(wildcard wm/bsp/integration/*.inc)
$(patsubst %.c,%.o,$(wildcard wm/bsp/core/*.c)): wm/bsp/bsp.h $(wildcard wm/bsp/core/*.h)
dwm.o: wm/resize/math.h wm/resize/layout.inc wm/resize/topology.inc wm/resize/precision.inc wm/resize/boundary.inc wm/resize/mouse.inc
dwm.o: bar/wm/docks.inc bar/wm/ipc.inc bar/wm/geometry.h bar/wm/json.h
bar/wm/geometry.o: bar/wm/geometry.h
bar/wm/json.o: bar/wm/json.h
reload/build.o: reload/build.h
startup/autostart.o: startup/autostart.h
input/settings.o: input/settings.h

config.h:
	cp config.def.h $@

dwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	${MAKE} -C bar/bridge clean
	rm -f dwm ${OBJ} dwm-${VERSION}.tar.gz

dist: clean
	mkdir -p dwm-${VERSION}
	cp -R LICENSE Makefile README config.def.h config.mk\
		dwm.1 drw.h util.h drw.c dwm.c util.c dwm.png transient.c bar config session input wm startup reload tests dwm-${VERSION}
	tar -cf dwm-${VERSION}.tar dwm-${VERSION}
	gzip dwm-${VERSION}.tar
	rm -rf dwm-${VERSION}

install: all
	${MAKE} -C bar/bridge install PREFIX="${PREFIX}" DESTDIR="${DESTDIR}"
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp dwm ${DESTDIR}${PREFIX}/bin/dwm.new
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm.new
	mv -f ${DESTDIR}${PREFIX}/bin/dwm.new ${DESTDIR}${PREFIX}/bin/dwm
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm\
		${DESTDIR}${MANPREFIX}/man1/dwm.1

.PHONY: all bar-bridge clean dist install uninstall
