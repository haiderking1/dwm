#include "fake_server.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

Status
XIQueryVersion(Display *display, int *major, int *minor)
{
	assert(display == test_display && *major == 2 && *minor == 0);
	*major = server.version_major;
	return server.version_status;
}

XIDeviceInfo *
XIQueryDevice(Display *display, int id, int *count)
{
	XIDeviceInfo *result;
	int i;

	assert(display == test_display);
	++server.query_count;
	result = calloc(8, sizeof(*result));
	assert(result);
	*count = 0;
	for (i = 0; i < server.device_count; ++i)
		if (id == XIAllDevices || id == server.devices[i].info.deviceid)
			result[(*count)++] = server.devices[i].info;
	if (*count == 0 && id != XIAllDevices) {
		free(result);
		fake_error(FAKE_ERROR_BASE + XI_BadDevice, FAKE_OPCODE, 48, id);
		return NULL;
	}
	return result;
}

void
XIFreeDeviceInfo(XIDeviceInfo *devices)
{
	free(devices);
}

static FakeProperty *
find_property(int device, Atom atom)
{
	int i, j;
	for (i = 0; i < server.device_count; ++i)
		if (server.devices[i].info.deviceid == device)
			for (j = 0; j < server.devices[i].property_count; ++j)
				if (server.devices[i].properties[j].atom == atom)
					return &server.devices[i].properties[j];
	return NULL;
}

Status
XIGetProperty(Display *display, int device, Atom atom, long offset, long length,
              Bool delete_property, Atom requested_type, Atom *type, int *format,
              unsigned long *count, unsigned long *remaining, unsigned char **data)
{
	FakeProperty *property = find_property(device, atom);
	unsigned long size, capacity;

	assert(display == test_display && offset == 0 && !delete_property);
	assert(requested_type == AnyPropertyType && length > 0);
	++server.property_reads;
	*data = NULL;
	*count = *remaining = 0;
	*type = None;
	*format = 0;
	if (server.remove_on_read) {
		fake_error(FAKE_ERROR_BASE + XI_BadDevice, FAKE_OPCODE, 59, device);
		return FAKE_ERROR_BASE + XI_BadDevice;
	}
	if (!property)
		return Success;
	++property->reads;
	if (property->status != Success)
		return property->status;
	*type = property->type;
	*format = property->format;
	size = (unsigned long)(property->format / 8);
	capacity = (unsigned long)length * 4 / size;
	*count = property->count < capacity ? property->count : capacity;
	*remaining = property->remaining + (property->count - *count) * size;
	if (!property->null_data) {
		*data = malloc(*count * size + 1);
		assert(*data);
		memcpy(*data, property->data, *count * size);
	}
	return Success;
}

void
XIChangeProperty(Display *display, int device, Atom atom, Atom type, int format,
                 int mode, unsigned char *data, int count)
{
	FakeProperty *property = find_property(device, atom);

	assert(display == test_display && property && property->reads > 0);
	assert(type == property->type && format == property->format && mode == PropModeReplace);
	assert((unsigned long)count == property->count);
	if (server.remove_on_write) {
		server.pending_error.error_code = FAKE_ERROR_BASE + XI_BadDevice;
		server.pending_error.request_code = FAKE_OPCODE;
		server.pending_error.minor_code = 57;
		server.pending_error.resourceid = (XID)device;
		return;
	}
	memcpy(property->data, data, (size_t)count * (size_t)(format / 8));
	++property->writes;
	++server.writes;
}

XIEventMask *
XIGetSelectedEvents(Display *display, Window root, int *count)
{
	XIEventMask *mask;

	assert(display == test_display && root == 1);
	*count = server.selection_count;
	if (*count <= 0)
		return NULL;
	assert(*count == 1);
	mask = malloc(sizeof(*mask) + FAKE_MASK_LEN);
	assert(mask);
	mask->deviceid = XIAllDevices;
	mask->mask_len = FAKE_MASK_LEN;
	mask->mask = (unsigned char *)(mask + 1);
	memcpy(mask->mask, server.selected, FAKE_MASK_LEN);
	return mask;
}

int
XISelectEvents(Display *display, Window root, XIEventMask *mask, int count)
{
	assert(display == test_display && root == 1 && count == 1);
	assert(mask->deviceid == XIAllDevices && mask->mask_len <= FAKE_MASK_LEN);
	assert(server.query_count == 0); /* Subscribe before enumeration. */
	memset(server.selected, 0, FAKE_MASK_LEN);
	memcpy(server.selected, mask->mask, (size_t)mask->mask_len);
	++server.select_calls;
	return Success;
}
