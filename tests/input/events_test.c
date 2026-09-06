#include "fake_server.h"
#include "../../input/settings.h"

#include <assert.h>
#include <string.h>

static XEvent
generic_event(int type)
{
	XEvent event;

	memset(&event, 0, sizeof(event));
	event.xcookie.type = GenericEvent;
	event.xcookie.display = test_display;
	event.xcookie.extension = FAKE_OPCODE;
	event.xcookie.evtype = type;
	return event;
}

static void
setup_and_failures(void)
{
	XErrorHandler old;
	int failure;

	fake_reset();
	old = server.handler;
	server.selection_count = 1;
	XISetMask(server.selected, XI_RawMotion);
	/* Errors queued before setup belong to the caller, not this module. */
	server.pending_error.error_code = BadWindow;
	input_setup(test_display, 1);
	assert(server.previous_errors == 1 && server.handler == old);
	assert(server.core_calls == 1 && server.repeat_calls == 1 && server.rate_calls == 1);
	assert(server.select_calls == 1);
	assert(XIMaskIsSet(server.selected, XI_RawMotion));
	assert(XIMaskIsSet(server.selected, XI_HierarchyChanged));
	assert(XIMaskIsSet(server.selected, XI_DeviceChanged));
	assert(!XIMaskIsSet(server.selected, XI_PropertyEvent));

	for (failure = 0; failure < 6; ++failure) {
		fake_reset();
		old = server.handler;
		switch (failure) {
		case 0: server.xi_present = False; break;
		case 1: server.version_status = BadRequest; break;
		case 2: server.version_major = 1; break;
		case 3: server.xkb_present = False; break;
		case 4: server.repeat_ok = False; break;
		case 5: server.selection_count = -1; break;
		}
		input_setup(test_display, 1);
		assert(server.core_calls == 1 && server.repeat_calls == 1);
		assert(server.handler == old);
		assert(server.rate_calls == (failure != 3));
		assert(server.select_calls == (failure == 3 || failure == 4));
	}
}

static void
cookies(void)
{
	XEvent event, before;
	XIDeviceChangedEvent changed = {0};
	int i, queries;

	fake_reset();
	input_setup(test_display, 1);
	queries = server.query_count;
	for (i = 0; i < 4; ++i) {
		event = generic_event(XI_DeviceChanged);
		if (i == 0) event.type = KeyPress;
		if (i == 1) event.xcookie.extension++;
		if (i == 2) event.xcookie.evtype = XI_PropertyEvent;
		if (i == 3) event.xcookie.display = NULL;
		before = event;
		input_handle_event(test_display, &event);
		assert(memcmp(&event, &before, sizeof(event)) == 0);
	}
	assert(server.cookie_gets == 0 && server.cookie_frees == 0);
	event = generic_event(XI_DeviceChanged);
	server.cookie_ok = False;
	input_handle_event(test_display, &event);
	assert(server.cookie_gets == 1 && server.cookie_frees == 0);
	server.cookie_ok = True;
	changed.reason = XISlaveSwitch;
	server.cookie_data = &changed;
	input_handle_event(test_display, &event);
	assert(server.cookie_gets == 2 && server.cookie_frees == 1 && event.xcookie.data == NULL);
	assert(server.query_count == queries);
	event.xcookie.data = &changed;
	input_handle_event(test_display, &event);
	assert(server.cookie_gets == 2 && server.cookie_frees == 1);
	assert(event.xcookie.data == &changed && server.query_count == queries);
	input_handle_event(NULL, &event);
	input_handle_event(test_display, NULL);
}

static void
hotplug_and_races(void)
{
	XIHierarchyInfo info[3] = {{0}};
	XIHierarchyEvent hierarchy = {0};
	XIDeviceChangedEvent changed = {0};
	XEvent event;
	XErrorHandler old;
	FakeDevice *device;
	FakeProperty *property;
	float speed = 0.5f;
	int queries;

	fake_reset();
	old = server.handler;
	server.float_present = False;
	input_setup(test_display, 1);
	server.float_present = True;
	device = fake_device(10, XISlavePointer, XIModeRelative);
	property = fake_property(device, "libinput Accel Speed", FAKE_FLOAT, 32, 1, &speed);
	info[0].deviceid = 10;
	info[0].enabled = True;
	info[0].flags = XISlaveAdded | XIDeviceEnabled;
	info[1].deviceid = 11;
	info[1].flags = XIDeviceDisabled;
	info[2].deviceid = 12;
	info[2].flags = XISlaveRemoved;
	hierarchy.flags = XISlaveAdded | XIDeviceEnabled | XIDeviceDisabled | XISlaveRemoved;
	hierarchy.num_info = 3;
	hierarchy.info = info;
	server.cookie_data = &hierarchy;
	event = generic_event(XI_HierarchyChanged);
	input_handle_event(test_display, &event);
	assert(server.query_count == 2 && property->writes == 1);
	assert(server.cookie_gets == 1 && server.cookie_frees == 1 && server.handler == old);

	/* Enabling an existing pointer applies settings, but only changed values. */
	info[0].flags = hierarchy.flags = XIDeviceEnabled;
	input_handle_event(test_display, &event);
	assert(server.query_count == 3 && property->writes == 1);
	memcpy(property->data, &speed, sizeof(speed));
	input_handle_event(test_display, &event);
	assert(property->writes == 2);

	/* A removal-only notification neither queries devices nor writes settings. */
	queries = server.query_count;
	hierarchy.flags = XISlaveRemoved;
	input_handle_event(test_display, &event);
	assert(server.query_count == queries);

	/* The device disappeared before its added/enabled event was handled. */
	hierarchy.flags = XIDeviceEnabled;
	info[0].deviceid = 99;
	input_handle_event(test_display, &event);
	assert(server.previous_errors == 0 && server.handler == old);

	/* Driver class changes are handled, unlike normal slave switches. */
	event = generic_event(XI_DeviceChanged);
	changed.reason = XIDeviceChange;
	changed.deviceid = 10;
	event.xcookie.data = &changed;
	memcpy(property->data, &speed, sizeof(speed));
	input_handle_event(test_display, &event);
	assert(property->writes == 3 && event.xcookie.data == &changed);

	/* Removal between device enumeration and the synchronous property read. */
	memcpy(property->data, &speed, sizeof(speed));
	server.remove_on_read = True;
	input_handle_event(test_display, &event);
	assert(property->writes == 3 && server.previous_errors == 0 && server.handler == old);
	server.remove_on_read = False;

	/* An asynchronous property write failure is caught before handler restore. */
	memcpy(property->data, &speed, sizeof(speed));
	server.remove_on_write = True;
	input_handle_event(test_display, &event);
	assert(server.pending_error.error_code == 0);
	assert(server.previous_errors == 0 && server.handler == old);
	fake_error(BadWindow, 1, 0, 1);
	assert(server.previous_errors == 1);
}

void
test_events(void)
{
	setup_and_failures();
	cookies();
	hotplug_and_races();
}
