#include <assert.h>
#include "xmock.h"
#define main dwm_session_main_not_called
#include "../../../dwm.c"
#undef main
#include "restore.inc"

static void dock_event(int type, Atom property)
{
 XEvent event = {0};
 event.type = type;
 if (type == MapNotify) event.xmap.window = mock.window;
 if (type == UnmapNotify) event.xunmap.window = mock.window;
 if (type == DestroyNotify) event.xdestroywindow.window = mock.window;
 if (type == PropertyNotify) {
  event.xproperty.window = mock.window;
  event.xproperty.atom = property;
  event.xproperty.state = PropertyNewValue;
 }
 qs_dock_event(&event);
}

int main(void)
{
 Monitor m = {0}, second = {0};
 Client c = {0};
 XEvent event = {0};
 XClientMessageEvent command = {0};
 int writes;
 root = mock.root = 1; mock.window = 20;
 netatom[NetWMWindowType] = mock.type_atom = 2;
 netatom[NetWMWindowTypeDock] = mock.dock_atom = 3;
 netatom[NetWMStrutPartial] = mock.partial_atom = 4;
 netatom[NetWMStrut] = mock.strut_atom = 5;
 qs_ipc.state = mock.state_atom = 6; qs_ipc.utf8 = mock.utf8_atom = 7;
 qs_ipc.command = 8; netatom[NetActiveWindow] = 9;
 mons = selmon = &m;
 m.mw = sw = 1920; m.mh = sh = 1080; bh = 20;
 m.tagset[0] = m.tagset[1] = 1;
 m.lt[0] = m.lt[1] = &layouts[0];
 strcpy(m.ltsymbol, layouts[0].symbol);
 m.barwin = 100;
 /* Checkpoint showbar=1 cannot reserve native space, even without a window. */
 m.showbar = 1; updatebarpos(&m);
 assert(!m.showbar && m.wy == 0 && m.wh == 1080 && m.by == -bh);
 m.barwin = None; drawbar(&m); togglebar(NULL); m.barwin = 100;
 assert(!m.showbar && m.wh == 1080);
 test_restore_hidden_bar(&m);

 /* Override-redirect docks are tracked without becoming Clients. */
 mock.dock = 1; mock.override_redirect = 1; mock.mapped = IsUnmapped;
 mock.partial_count = 12; mock.partial[2] = 30; mock.partial[9] = 1919;
 assert(qs_dock_watch(mock.window));
 assert(!m.clients && !qs_dock_find(mock.window)->mapped && m.wy == 0);
 assert(mock.selected_events & PropertyChangeMask);
 event.xmaprequest.window = mock.window;
 maprequest(&event); assert(mock.maps == 1 && !m.clients);
 dock_event(MapNotify, None); assert(m.wy == 30 && m.wh == 1050);
 mock.partial[2] = 45; dock_event(PropertyNotify, mock.partial_atom);
 assert(m.wy == 45 && m.wh == 1035);
 /* Property deletion and malformed partial properties fall back to legacy. */
 mock.partial_count = 0; mock.legacy_count = 4; mock.legacy[3] = 25;
 dock_event(PropertyNotify, mock.partial_atom); assert(m.wy == 0 && m.wh == 1055);
 mock.partial_count = 3; dock_event(PropertyNotify, mock.partial_atom);
 assert(m.wh == 1055);
 mock.legacy_count = 0; dock_event(PropertyNotify, mock.strut_atom); assert(m.wh == 1080);
 mock.partial_count = 12; dock_event(PropertyNotify, mock.partial_atom); assert(m.wy == 45);
 dock_event(UnmapNotify, None); assert(m.wy == 0 && m.wh == 1080);
 dock_event(MapNotify, None); assert(m.wy == 45);
 dock_event(DestroyNotify, None); assert(!qs_docks && m.wy == 0);
 /* A type installed after creation is discovered through PropertyNotify. */
 mock.dock = 0; assert(!qs_dock_watch(mock.window));
 mock.dock = 1; dock_event(PropertyNotify, mock.type_atom); assert(qs_docks);
 mock.mapped = IsUnmapped; mock.dock = 0;
 dock_event(PropertyNotify, mock.type_atom); assert(!qs_docks);

 /* Exact state is validated with Python; repeated publication makes no writes. */
 c.tags = 3; c.isurgent = 1; c.mon = &m;
 strcpy(c.name, "title "); c.name[6] = 34; c.name[7] = 92; c.name[8] = 10; c.name[9] = 0;
 m.clients = m.sel = &c;
 qs_publish(); writes = mock.property_writes; qs_publish();
 assert(mock.property_writes == writes); puts(mock.state);
 c.isfullscreen = 1; qs_publish(); assert(mock.property_writes == writes + 1);
 puts(mock.state);
 m.tagset[0] = 4; qs_publish(); puts(mock.state); /* Hidden fullscreen is false. */
 m.clients = m.sel = NULL; m.tagset[0] = 1;
 second = m; second.num = 1; second.mx = 1920; second.next = NULL;
 m.next = &second;
 qs_publish(); puts(mock.state);

 command.message_type = qs_ipc.command; command.format = 32;
 command.data.l[0] = 1; command.data.l[1] = 99; command.data.l[2] = 2;
 assert(qs_command(&command)); assert(selmon == &m && m.tagset[0] == 1);
 command.data.l[1] = 1; command.data.l[2] = 1UL << 20;
 qs_command(&command); assert(selmon == &m); /* Mask is zero after TAGMASK. */
 command.data.l[2] = 0; qs_command(&command); assert(selmon == &m);
 command.format = 8; command.data.l[2] = 2; qs_command(&command); assert(selmon == &m);
 command.format = 32; command.data.l[0] = 99; qs_command(&command); assert(selmon == &m);
 command.data.l[0] = 4; qs_command(&command); assert(selmon == &m && !m.showbar);
 command.data.l[1] = 0; command.data.l[0] = 1; command.data.l[2] = TAGMASK + 2;
 qs_command(&command); assert(m.tagset[0] == 1); /* Non-tag bits are masked. */
 command.data.l[0] = 2; qs_command(&command); assert(!m.clients); /* No selection. */
 command.data.l[0] = 3; qs_command(&command); assert(m.lt[m.sellt] == &layouts[1]);
 command.message_type = 99; assert(!qs_command(&command));
 qs_dock_cleanup(); qs_ipc_cleanup(); free(mock.state);
 return 0;
}
