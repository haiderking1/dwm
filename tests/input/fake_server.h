#ifndef INPUT_TEST_FAKE_SERVER_H
#define INPUT_TEST_FAKE_SERVER_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput2.h>

#define FAKE_OPCODE 131
#define FAKE_ERROR_BASE 160
#define FAKE_FLOAT 90
#define FAKE_MASK_LEN XIMaskLen(XI_LASTEVENT)

typedef struct {
	const char *name;
	Atom atom, type;
	int format;
	unsigned long count, remaining;
	unsigned char data[32];
	int reads, writes, status;
	Bool null_data;
} FakeProperty;

typedef struct {
	XIDeviceInfo info;
	XIValuatorClassInfo axes[3];
	XIAnyClassInfo *classes[3];
	FakeProperty properties[8];
	int property_count;
} FakeDevice;

typedef struct {
	Bool xi_present, xkb_present, repeat_ok, cookie_ok, float_present;
	int version_status, version_major, selection_count;
	FakeDevice devices[8];
	int device_count, query_count, property_reads, writes;
	int core_calls, repeat_calls, rate_calls, select_calls;
	int cookie_gets, cookie_frees, previous_errors, syncs;
	Bool remove_on_read, remove_on_write;
	unsigned char selected[FAKE_MASK_LEN];
	void *cookie_data;
	XErrorHandler handler;
	XErrorEvent pending_error;
} FakeServer;

extern FakeServer server;
extern Display *test_display;
void fake_reset(void);
FakeDevice *fake_device(int id, int use, int mode);
FakeProperty *fake_property(FakeDevice *device, const char *name, Atom type,
                            int format, unsigned long count, const void *data);
void fake_error(int code, int request, int minor, int device);
void test_properties(void);
void test_events(void);

#endif
