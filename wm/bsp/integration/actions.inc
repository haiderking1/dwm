static void
bsp_rotate_selected(const Arg *arg)
{
	BspWorkspace *view;
	Client *c = selmon->sel;
	(void)arg;

	if (c && !c->isfloating && !c->isfullscreen
	&& selmon->lt[selmon->sellt]->arrange == dwindle
	&& (view = bsp_active(selmon)) && bsp_tree_rotate(&view->tree, c->win))
		arrange(selmon);
}

static void
bsp_adjust_selected(double delta)
{
	BspWorkspace *view;
	Client *c = selmon->sel;

	if (c && !c->isfloating && !c->isfullscreen && (view = bsp_active(selmon))
	&& bsp_tree_adjust(&view->tree, c->win, BSP_HORIZONTAL, delta))
		arrange(selmon);
}

/* Zoom exchanges the focused leaf with the first spatial leaf (or the next
 * leaf when already first). Client-list order is irrelevant to a saved tree. */
static void
bsp_zoom_selected(void)
{
	BspWorkspace *view;
	BspNode *first, *other;
	Client *c = selmon->sel;

	if (!c || c->isfloating || c->isfullscreen || !(view = bsp_active(selmon)))
		return;
	first = view->tree.root;
	if (!first || !first->child[0])
		return;
	while (first->child[0])
		first = first->child[0];
	other = first;
	if (first->window == c->win) {
		other = first->parent->child[1];
		while (other->child[0])
			other = other->child[0];
	}
	if (bsp_tree_swap(&view->tree, c->win, other->window))
		arrange(selmon);
}
