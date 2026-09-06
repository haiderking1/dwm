#include "settings.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput2.h>

static Display *input_display;
static int xi_opcode = -1;
static int xi_error_base;
static Bool xi_ready;
static XErrorHandler previous_error_handler;

typedef struct {
	Atom atom;
	unsigned long count;
	unsigned char *data;
} Property;

/* Drain old errors before trapping only this module's synchronous batch. */
static int
input_error(Display *display, XErrorEvent *error)
{
	if (display != input_display)
		return previous_error_handler ? previous_error_handler(display, error) : 0;
	/* Unplugging between XIQueryDevice and a property request is harmless. */
	if (error->request_code == xi_opcode &&
	    error->error_code == xi_error_base + XI_BadDevice)
		return 0;
	fprintf(stderr, "input: X11 error %u on request %u.%u, resource %lu\n",
	        error->error_code, error->request_code, error->minor_code,
	        error->resourceid);
	return 0;
}

static void
begin_requests(Display *display)
{
	XSync(display, False);
	previous_error_handler = XSetErrorHandler(input_error);
}

static void
end_requests(Display *display)
{
	XSync(display, False);
	XSetErrorHandler(previous_error_handler);
}

static Bool
read_property(Display *display, int device, const char *name, Atom type,
              int format, unsigned long minimum, unsigned long maximum,
              Property *property)
{
	Atom actual_type;
	int actual_format, status;
	unsigned long remaining;

	memset(property, 0, sizeof(*property));
	if (type == None || (property->atom = XInternAtom(display, name, True)) == None)
		return False;
	/* XIGetProperty's length is in four-byte units, including for format 8. */
	status = XIGetProperty(display, device, property->atom, 0,
	                       (long)((maximum * (unsigned long)(format / 8) + 3) / 4),
	                       False, AnyPropertyType, &actual_type, &actual_format,
	                       &property->count, &remaining, &property->data);
	if (status == Success && actual_type == type && actual_format == format &&
	    property->count >= minimum && property->count <= maximum &&
	    remaining == 0 && property->data != NULL)
		return True;
	XFree(property->data);
	property->data = NULL;
	return False;
}

static void
replace_property(Display *display, int device, const Property *property,
                 Atom type, int format, const void *value)
{
	/* XI2 uses packed 32-bit data, not Xlib window properties' native longs. */
	if (memcmp(property->data, value, property->count * (size_t)(format / 8)) != 0)
		XIChangeProperty(display, device, property->atom, type, format,
		                 PropModeReplace, (unsigned char *)value, (int)property->count);
}

static Bool
boolean_list(const Property *property)
{
	unsigned long i;

	for (i = 0; i < property->count; ++i)
		if (property->data[i] > 1)
			return False;
	return True;
}

static void
set_flat_profile(Display *display, int device)
{
	Property available, enabled;
	unsigned char flat[3] = { 0, 1, 0 };

	/* Two entries on older libinput, three with the custom profile. */
	if (!read_property(display, device, "libinput Accel Profiles Available",
	                   XA_INTEGER, 8, 2, 3, &available))
		return;
	if (boolean_list(&available) && available.data[1] == 1 &&
	    read_property(display, device, "libinput Accel Profile Enabled",
	                  XA_INTEGER, 8, available.count, available.count, &enabled)) {
		if (boolean_list(&enabled))
			replace_property(display, device, &enabled, XA_INTEGER, 8, flat);
		XFree(enabled.data);
	}
	XFree(available.data);
}

static void
set_scalar(Display *display, int device, const char *name, Atom type,
           const void *value)
{
	Property property;

	if (read_property(display, device, name, type, 32, 1, 1, &property)) {
		replace_property(display, device, &property, type, 32, value);
		XFree(property.data);
	}
}

static Bool
relative_pointer(const XIDeviceInfo *device)
{
	int i;

	if (!device->enabled || (device->use != XISlavePointer &&
	                        device->use != XIFloatingSlave))
		return False;
	for (i = 0; i < device->num_classes; ++i) {
		XIAnyClassInfo *class_info = device->classes[i];
		if (class_info->type == XIValuatorClass) {
			XIValuatorClassInfo *axis = (XIValuatorClassInfo *)class_info;
			/* Relative scroll axes alone do not make a tablet a mouse. */
			if ((axis->number == 0 || axis->number == 1) && axis->mode == XIModeRelative)
				return True;
		}
	}
	return False;
}

static void
configure_pointers(Display *display, int deviceid)
{
	XIDeviceInfo *devices;
	int count = 0, i;
	Atom float_type;
	int32_t no_profile = -1;
	float zero = 0.0f, one = 1.0f;

	devices = XIQueryDevice(display, deviceid, &count);
	if (!devices) {
		if (deviceid == XIAllDevices)
			fprintf(stderr, "input: cannot query XI2 devices\n");
		return;
	}
	/* Look up atoms again on hotplug; a newly loaded driver can add them. */
	float_type = XInternAtom(display, "FLOAT", True);
	for (i = 0; i < count; ++i) {
		if (!relative_pointer(&devices[i]))
			continue;
		deviceid = devices[i].deviceid;
		/* Includes relative touchpads; never changes tapping or scrolling. */
		set_flat_profile(display, deviceid);
		set_scalar(display, deviceid, "libinput Accel Speed", float_type, &zero);
		set_scalar(display, deviceid, "Device Accel Profile", XA_INTEGER, &no_profile);
		set_scalar(display, deviceid, "Device Accel Constant Deceleration", float_type, &one);
		set_scalar(display, deviceid, "Device Accel Adaptive Deceleration", float_type, &one);
	}
	XIFreeDeviceInfo(devices);
}

static void
configure_keyboard(Display *display)
{
	int opcode, event_base, error_base;
	int major = XkbMajorVersion, minor = XkbMinorVersion;

	XAutoRepeatOn(display);
	if (!XkbQueryExtension(display, &opcode, &event_base, &error_base, &major, &minor)) {
		fprintf(stderr, "input: XKB unavailable; repeat delay and interval unchanged\n");
		return;
	}
	/* Nearest whole millisecond to 1000 / 35 Hz. */
	if (!XkbSetAutoRepeatRate(display, XkbUseCoreKbd, 200, (1000 + 35 / 2) / 35))
		fprintf(stderr, "input: cannot set XKB repeat rate\n");
}

static void
select_events(Display *display, Window root)
{
	XIEventMask *selected, mask;
	int count = 0, i, length = XIMaskLen(XI_HierarchyChanged);
	unsigned char *bits;

	selected = XIGetSelectedEvents(display, root, &count);
	if (count < 0) {
		fprintf(stderr, "input: cannot read XI2 root event selection\n");
		XFree(selected);
		return;
	}
	for (i = 0; i < count; ++i)
		if (selected[i].deviceid == XIAllDevices && selected[i].mask_len > length)
			length = selected[i].mask_len;
	bits = calloc((size_t)length, 1);
	if (!bits) {
		fprintf(stderr, "input: cannot allocate XI2 event mask\n");
		XFree(selected);
		return;
	}
	for (i = 0; i < count; ++i)
		if (selected[i].deviceid == XIAllDevices)
			memcpy(bits, selected[i].mask, (size_t)selected[i].mask_len);
	XISetMask(bits, XI_HierarchyChanged);
	XISetMask(bits, XI_DeviceChanged);
	mask.deviceid = XIAllDevices;
	mask.mask_len = length;
	mask.mask = bits;
	if (XISelectEvents(display, root, &mask, 1) != Success)
		fprintf(stderr, "input: cannot select XI2 hotplug events\n");
	free(bits);
	XFree(selected);
}

void
input_setup(Display *display, Window root)
{
	int event_base, major = 2, minor = 0;

	if (!display)
		return;
	input_display = display;
	xi_ready = False;
	xi_opcode = -1;
	begin_requests(display);
	/* Core baseline/fallback; modern drivers still need their own properties. */
	XChangePointerControl(display, True, True, 1, 1, 0);
	configure_keyboard(display);
	if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &event_base, &xi_error_base)) {
		fprintf(stderr, "input: XInput unavailable; using core pointer control only\n");
	} else if (XIQueryVersion(display, &major, &minor) != Success || major < 2) {
		fprintf(stderr, "input: XI2 unavailable; using core pointer control only\n");
	} else {
		xi_ready = True;
		/* Subscribe before enumerating so hotplug during setup is not lost. */
		select_events(display, root);
		configure_pointers(display, XIAllDevices);
	}
	end_requests(display);
}

void
input_handle_event(Display *display, XEvent *event)
{
	XGenericEventCookie *cookie;
	Bool acquired;
	int i;

	if (!xi_ready || display != input_display || !event || event->type != GenericEvent)
		return;
	cookie = &event->xcookie;
	if (cookie->display != display || cookie->extension != xi_opcode ||
	    (cookie->evtype != XI_HierarchyChanged && cookie->evtype != XI_DeviceChanged))
		return;
	acquired = cookie->data == NULL;
	if (acquired && !XGetEventData(display, cookie))
		return;
	if (cookie->data && cookie->evtype == XI_HierarchyChanged) {
		XIHierarchyEvent *hierarchy = cookie->data;
		const int added_or_enabled = XISlaveAdded | XIDeviceEnabled;

		if (hierarchy->flags & added_or_enabled) {
			begin_requests(display);
			for (i = 0; i < hierarchy->num_info; ++i)
				if (hierarchy->info[i].enabled &&
				    (hierarchy->info[i].flags & added_or_enabled))
					configure_pointers(display, hierarchy->info[i].deviceid);
			end_requests(display);
		}
	} else if (cookie->data) {
		XIDeviceChangedEvent *changed = cookie->data;

		/* Ignore frequent master-device slave switches. No PropertyEvent mask:
		 * our writes must not cause a configuration loop. Writes are idempotent.
		 */
		if (changed->reason == XIDeviceChange) {
			begin_requests(display);
			configure_pointers(display, changed->deviceid);
			end_requests(display);
		}
	}
	if (acquired)
		XFreeEventData(display, cookie);
}
