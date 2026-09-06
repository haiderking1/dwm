static void
dispatch_event(XEvent *event)
{
	Client *c;
	Monitor *m = &f.monitors[0];
	assert(f.grabbed);
	++f.dispatches;
	switch (event->type) {
	case DestroyNotify:
		destroy_client(event->xdestroywindow.window);
		break;
	case ConfigureRequest:
		c = wintoclient(event->xconfigurerequest.window);
		assert(c && !c->isfloating);
		assert(event->xconfigurerequest.value_mask == (CWX | CWY | CWWidth | CWHeight));
		/* dwm acknowledges but does not apply a tiled client's request. */
		++f.configure_replies;
		break;
	case ConfigureNotify:
		assert(event->xconfigure.window == root);
		/* Model a monitor/workarea update and its immediate arrange. */
		m->slotx = event->xconfigure.x;
		m->sloty = event->xconfigure.y;
		m->slotw = event->xconfigure.width;
		m->sloth = event->xconfigure.height;
		arrange(m);
		break;
	case PropertyNotify:
		if (event->xproperty.atom == FLOAT_LAYOUT) {
			m->sellt = 1;
			arrange(m);
		} else {
			assert(event->xproperty.atom == FULLSCREEN_CHANGE);
			c = wintoclient(event->xproperty.window);
			assert(c);
			c->isfullscreen = 1;
		}
		break;
	case Expose:
		assert(event->xexpose.window == root);
		break;
	default:
		assert(!"unexpected dispatch");
	}
}

static int
qs_dock_event(XEvent *event)
{
	assert(f.grabbed);
	if (event->type != PropertyNotify || event->xproperty.atom != DOCK_CHANGE)
		return 0;
	assert(event->xproperty.window == root);
	++f.dock_events;
	f.monitors[0].sloty = 80;
	arrange(&f.monitors[0]);
	return 1;
}
