/*
	Copyright (c) 2011, Marvell Semiconductor

	@author Ian Carvalho, Global Edge Software Ltd.

	@todo Marvell to provide copyright notice that is to be placed
	at the top of this file.
*/

/*
	Bluetooth UCD Subsystem functions
*/

#ifndef _L2CAP_UCD_H_
#define	_L2CAP_UCD_H_

#include <linux/spinlock.h>

static spinlock_t	handlers_lock;
static struct list_head	handlers;

static inline struct ucd_handler *ucd_find_handler(u16 psm)
{
	struct list_head	*l;
	struct ucd_handler	*entry;

	list_for_each(l, &handlers) {
		entry = list_entry(l, struct ucd_handler, list);

		if (entry->psm == psm)
			return entry;
	}

	return NULL;
}

static inline struct ucd_handler *ucd_add_handler(struct ucd_handler *h)
{
	list_add((struct list_head *)h, &handlers);

	return h;
}

static inline struct ucd_handler *ucd_del_handler(struct ucd_handler *h)
{
	list_del((struct list_head *)h);

	return h;
}

int l2cap_ucd_register(struct ucd_handler *h)
{
	int	ret;

	spin_lock(&handlers_lock);

	if (h->psm <= 0 || ucd_find_handler(h->psm)) {
		ret = -1; /* PSM invalid or already in use */
		goto done;
	}

	ucd_add_handler(h);

	ret = 0;

done:
	spin_unlock(&handlers_lock);

	return ret;
}
EXPORT_SYMBOL(l2cap_ucd_register);

int l2cap_ucd_unregister(struct ucd_handler *handler)
{
	int ret;
	struct ucd_handler *h;

	spin_lock(&handlers_lock);

	if (!(h = ucd_find_handler(handler->psm)) || h != handler) {
		ret = -1;	/* mismatched unregister */
		goto done;
	}

	ucd_del_handler(h);

	ret = 0;

done:
	spin_unlock(&handlers_lock);

	return ret;
}
EXPORT_SYMBOL(l2cap_ucd_unregister);

int l2cap_ucd_receive(struct l2cap_conn *conn, u16 psm,
							struct sk_buff *skb)
{
	struct ucd_handler *h;

	if ((h = ucd_find_handler(psm)) == NULL)
		return NO;

	h->receiver(h->context, skb);

	return YES;
}

int l2cap_ucd_init(void)
{
	BT_INFO("UCD (Unicast Connectionless Data) ver %s", BT_UCD_VERSION);

	INIT_LIST_HEAD(&handlers);
	spin_lock_init(&handlers_lock);

	return 0;
}

int l2cap_ucd_uninit(void)
{
	if (list_empty(&handlers))
		return 0;

	return -1;
}

#endif /* _L2CAP_UCD_H_ */

