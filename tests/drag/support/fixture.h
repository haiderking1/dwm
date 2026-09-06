#ifndef DRAG_TEST_FIXTURE_H
#define DRAG_TEST_FIXTURE_H

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct Client Client;
typedef struct Monitor Monitor;
typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

struct Client {
	int x, y, w, h, bw;
	int isfloating, isfullscreen;
	unsigned int tags;
	Client *next, *snext;
	Monitor *mon;
};

struct Monitor {
	Client *clients, *stack, *sel;
	unsigned int tagset[2], seltags, sellt;
	const Layout *lt[2];
};

#define ISVISIBLE(C) ((C)->tags & (C)->mon->tagset[(C)->mon->seltags])
#define WIDTH(C) ((C)->w + 2 * (C)->bw)
#define HEIGHT(C) ((C)->h + 2 * (C)->bw)
#define CAPACITY 8

typedef struct {
	Monitor monitors[2];
	Client clients[CAPACITY];
	size_t count;
} Fixture;

static void
arrange_fixture(Monitor *mon)
{
	(void)mon;
	assert(!"placement must not invoke arrange");
}

static const Layout tiled_layout = { "tile", arrange_fixture };
static const Layout floating_layout = { "float", NULL };

static void
fixture_init(Fixture *f, size_t left, size_t right)
{
	size_t m, i, offset = 0;
	const size_t counts[2] = { left, right };

	assert(left + right <= CAPACITY);
	memset(f, 0, sizeof(*f));
	f->count = left + right;
	for (m = 0; m < 2; ++m) {
		Monitor *mon = &f->monitors[m];
		mon->seltags = mon->sellt = 1;
		mon->lt[0] = &floating_layout;
		mon->lt[1] = &tiled_layout;
		for (i = 0; i < counts[m]; ++i) {
			Client *c = &f->clients[offset + i];
			c->mon = mon;
			c->tags = 3u << (2u * (unsigned int)(offset + i));
			mon->tagset[1] |= c->tags;
			c->x = 10 * (int)i;
			c->y = -20;
			c->w = 30;
			c->h = 20;
			c->bw = 2;
			c->next = i + 1 < counts[m] ? c + 1 : NULL;
			/* Stack order deliberately differs from client order. */
			c->snext = i > 0 ? c - 1 : NULL;
		}
		if (counts[m]) {
			mon->clients = &f->clients[offset];
			mon->stack = &f->clients[offset + counts[m] - 1];
			mon->sel = mon->stack;
		}
		offset += counts[m];
	}
}

/* Exact bounded walks catch missing nodes, duplicates and cycles. */
static void
expect_list(Client *head, Client *const *expected, size_t count, int stack)
{
	size_t i;
	for (i = 0; i < count; ++i) {
		assert(head == expected[i]);
		assert(head != NULL);
		head = stack ? head->snext : head->next;
	}
	assert(head == NULL);
}

static size_t
capture_list(Client *head, Client **nodes, int stack)
{
	size_t count = 0, i;
	while (head) {
		assert(count < CAPACITY);
		for (i = 0; i < count; ++i)
			assert(nodes[i] != head);
		nodes[count++] = head;
		head = stack ? head->snext : head->next;
	}
	return count;
}

static void
expect_unchanged(const Fixture *f, const Fixture *before)
{
	assert(memcmp(f, before, sizeof(*f)) == 0);
}

#endif
