#include "fake_server.h"
#include "../../input/settings.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static void
refresh(int device)
{
	XIDeviceChangedEvent changed = {0};
	XEvent event = {0};

	changed.deviceid = device;
	changed.reason = XIDeviceChange;
	event.xcookie.type = GenericEvent;
	event.xcookie.display = test_display;
	event.xcookie.extension = FAKE_OPCODE;
	event.xcookie.evtype = XI_DeviceChanged;
	event.xcookie.data = &changed;
	input_handle_event(test_display, &event);
}

static void
profiles(void)
{
	int count, malformed;
	unsigned char available[4] = {1, 1, 1, 1};
	unsigned char enabled[4] = {1, 0, 0, 0};
	unsigned char flat[3] = {0, 1, 0};
	FakeDevice *device;
	FakeProperty *a, *e;

	for (count = 2; count <= 3; ++count) {
		fake_reset();
		device = fake_device(10, XISlavePointer, XIModeRelative);
		fake_property(device, "libinput Accel Profiles Available", XA_INTEGER, 8,
		              (unsigned long)count, available);
		e = fake_property(device, "libinput Accel Profile Enabled", XA_INTEGER, 8,
		                  (unsigned long)count, enabled);
		input_setup(test_display, 1);
		assert(e->writes == 1 && memcmp(e->data, flat, (size_t)count) == 0);
		refresh(10);
		assert(e->writes == 1); /* No redundant writes on DeviceChanged. */
	}
	for (malformed = 0; malformed < 13; ++malformed) {
		fake_reset();
		device = fake_device(10, XISlavePointer, XIModeRelative);
		a = fake_property(device, "libinput Accel Profiles Available", XA_INTEGER, 8, 2, available);
		e = fake_property(device, "libinput Accel Profile Enabled", XA_INTEGER, 8, 2, enabled);
		switch (malformed) {
		case 0: a->type = XA_STRING; break;
		case 1: a->format = 32; break;
		case 2: a->count = 1; break;
		case 3: a->count = 4; break;
		case 4: a->remaining = 1; break;
		case 5: a->data[1] = 0; break;
		case 6: a->data[0] = 2; break;
		case 7: e->count = 3; break;
		case 8: e->type = XA_STRING; break;
		case 9: e->format = 16; break;
		case 10: e->data[0] = 2; break;
		case 11: a->status = BadAtom; break;
		case 12: e->null_data = True; break;
		}
		input_setup(test_display, 1);
		assert(server.writes == 0);
	}
}

static void
scalars(void)
{
	const char *names[] = {
		"libinput Accel Speed", "Device Accel Profile",
		"Device Accel Constant Deceleration", "Device Accel Adaptive Deceleration"
	};
	float initial = 0.75f, zero = 0.0f, one = 1.0f;
	int32_t profile = 0, disabled = -1;
	FakeDevice *device;
	FakeProperty *properties[4], *property;
	int i, malformed;

	fake_reset();
	device = fake_device(10, XISlavePointer, XIModeRelative);
	for (i = 0; i < 4; ++i)
		properties[i] = fake_property(device, names[i], i == 1 ? XA_INTEGER : FAKE_FLOAT,
		                              32, 1, i == 1 ? (void *)&profile : (void *)&initial);
	input_setup(test_display, 1);
	assert(server.writes == 4);
	assert(memcmp(properties[0]->data, &zero, 4) == 0);
	assert(memcmp(properties[1]->data, &disabled, 4) == 0);
	assert(memcmp(properties[2]->data, &one, 4) == 0);
	assert(memcmp(properties[3]->data, &one, 4) == 0);
	refresh(10);
	assert(server.writes == 4);

	for (i = 0; i < 4; ++i)
		for (malformed = 0; malformed < 8; ++malformed) {
			fake_reset();
			device = fake_device(10, XISlavePointer, XIModeRelative);
			property = fake_property(device, names[i], i == 1 ? XA_INTEGER : FAKE_FLOAT,
			                         32, 1, &initial);
			switch (malformed) {
			case 0: property->type = XA_STRING; break;
			case 1: property->format = 16; break;
			case 2: property->count = 0; break;
			case 3: property->count = 2; break;
			case 4: property->remaining = 4; break;
			case 5: property->status = BadAtom; break;
			case 6: property->null_data = True; break;
			case 7: property->atom = None; break;
			}
			input_setup(test_display, 1);
			assert(server.writes == 0);
		}
	fake_reset();
	server.float_present = False;
	device = fake_device(10, XISlavePointer, XIModeRelative);
	fake_property(device, names[0], FAKE_FLOAT, 32, 1, &initial);
	input_setup(test_display, 1);
	assert(server.writes == 0);
}

static void
pointer_filter(void)
{
	FakeDevice *device;
	FakeProperty *properties[7];
	float speed = 0.5f;
	int i;

	fake_reset();
	for (i = 0; i < 7; ++i) {
		device = fake_device(10 + i, XISlavePointer, XIModeRelative);
		if (i == 2) {
			device->axes[0].mode = device->axes[1].mode = XIModeAbsolute;
			/* A relative scroll axis must not include an absolute tablet. */
		}
		if (i == 3) device->info.use = XIMasterPointer;
		if (i == 4) device->info.use = XISlaveKeyboard;
		if (i == 5) device->info.enabled = False;
		if (i == 6) device->info.use = XIFloatingSlave;
		properties[i] = fake_property(device, "libinput Accel Speed", FAKE_FLOAT, 32, 1, &speed);
	}
	input_setup(test_display, 1);
	for (i = 0; i < 7; ++i) {
		assert(properties[i]->writes == (i == 0 || i == 1 || i == 6));
		assert(properties[i]->reads == (i == 0 || i == 1 || i == 6));
	}
}

void
test_properties(void)
{
	profiles();
	scalars();
	pointer_filter();
}
