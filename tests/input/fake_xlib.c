#include "fake_server.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

FakeServer server;
static char display_storage;
Display *test_display = (Display *)&display_storage;

static int
previous_error(Display *display, XErrorEvent *error)
{
	(void)display;
	(void)error;
	++server.previous_errors;
	return 0;
}

void
fake_reset(void)
{
	memset(&server, 0, sizeof(server));
	server.xi_present = server.xkb_present = server.repeat_ok = True;
	server.cookie_ok = server.float_present = True;
	server.version_major = 2;
	server.handler = previous_error;
}

FakeDevice *
fake_device(int id, int use, int mode)
{
	FakeDevice *device;
	int i;

	assert(server.device_count < 8);
	device = &server.devices[server.device_count++];
	device->info.deviceid = id;
	device->info.use = use;
	device->info.enabled = True;
	device->info.num_classes = 3;
	device->info.classes = device->classes;
	for (i = 0; i < 3; ++i) {
		device->axes[i].type = XIValuatorClass;
		device->axes[i].number = i;
		device->axes[i].mode = mode;
		device->classes[i] = (XIAnyClassInfo *)&device->axes[i];
	}
	return device;
}

FakeProperty *
fake_property(FakeDevice *device, const char *name, Atom type, int format,
              unsigned long count, const void *data)
{
	FakeProperty *property;
	int i, j;
	Atom atom = 100;

	assert(device->property_count < 8);
	for (i = 0; i < server.device_count; ++i)
		for (j = 0; j < server.devices[i].property_count; ++j) {
			FakeProperty *existing = &server.devices[i].properties[j];
			if (strcmp(existing->name, name) == 0) {
				atom = existing->atom;
				goto found;
			}
			if (existing->atom >= atom)
				atom = existing->atom + 1;
		}
found:
	property = &device->properties[device->property_count++];
	property->name = name;
	property->atom = atom;
	property->type = type;
	property->format = format;
	property->count = count;
	assert(count * (unsigned long)(format / 8) <= sizeof(property->data));
	memcpy(property->data, data, count * (size_t)(format / 8));
	return property;
}

void
fake_error(int code, int request, int minor, int device)
{
	XErrorEvent error = {0};

	error.error_code = (unsigned char)code;
	error.request_code = (unsigned char)request;
	error.minor_code = (unsigned char)minor;
	error.resourceid = (XID)device;
	assert(server.handler);
	server.handler(test_display, &error);
}

int
XSync(Display *display, Bool discard)
{
	assert(display == test_display && !discard);
	++server.syncs;
	if (server.pending_error.error_code) {
		server.handler(display, &server.pending_error);
		server.pending_error.error_code = 0;
	}
	return 1;
}

XErrorHandler
XSetErrorHandler(XErrorHandler handler)
{
	XErrorHandler previous = server.handler;
	server.handler = handler;
	return previous;
}

int
XFree(void *data)
{
	free(data);
	return 1;
}

Atom
XInternAtom(Display *display, const char *name, Bool only_if_exists)
{
	int i, j;

	assert(display == test_display && only_if_exists);
	if (strcmp(name, "FLOAT") == 0)
		return server.float_present ? FAKE_FLOAT : None;
	for (i = 0; i < server.device_count; ++i)
		for (j = 0; j < server.devices[i].property_count; ++j)
			if (strcmp(server.devices[i].properties[j].name, name) == 0)
				return server.devices[i].properties[j].atom;
	return None;
}

Bool
XQueryExtension(Display *display, const char *name, int *opcode, int *event, int *error)
{
	assert(display == test_display && strcmp(name, "XInputExtension") == 0);
	*opcode = FAKE_OPCODE;
	*event = 80;
	*error = FAKE_ERROR_BASE;
	return server.xi_present;
}

int
XChangePointerControl(Display *display, Bool accel, Bool threshold, int n, int d, int t)
{
	assert(display == test_display && accel && threshold && n == 1 && d == 1 && t == 0);
	++server.core_calls;
	return 1;
}

int
XAutoRepeatOn(Display *display)
{
	assert(display == test_display);
	++server.repeat_calls;
	return 1;
}

Bool
XkbQueryExtension(Display *display, int *opcode, int *event, int *error, int *major, int *minor)
{
	assert(display == test_display);
	(void)opcode; (void)event; (void)error; (void)major; (void)minor;
	return server.xkb_present;
}

Bool
XkbSetAutoRepeatRate(Display *display, unsigned int device, unsigned int delay, unsigned int interval)
{
	assert(display == test_display && device == XkbUseCoreKbd && delay == 200 && interval == 29);
	++server.rate_calls;
	return server.repeat_ok;
}

Bool
XGetEventData(Display *display, XGenericEventCookie *cookie)
{
	assert(display == test_display && cookie->data == NULL);
	++server.cookie_gets;
	if (server.cookie_ok)
		cookie->data = server.cookie_data;
	return server.cookie_ok;
}

void
XFreeEventData(Display *display, XGenericEventCookie *cookie)
{
	assert(display == test_display);
	++server.cookie_frees;
	cookie->data = NULL;
}
