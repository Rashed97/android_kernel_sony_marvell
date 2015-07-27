/*
	Copyright (c) 2011, Marvell Semiconductor

	@author Ian Carvalho, Global Edge Software Ltd.

	@todo Marvell to provide copyright notice that is to be placed
	at the top of this file.
*/

/*
	Bluetooth UCD Subsystem header file
*/

#ifndef __L2CAP_UCD_H
#define __L2CAP_UCD_H

#include <linux/types.h>
#include <linux/list.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/l2cap.h>

#define	BT_UCD_VERSION			"0.1"

#ifndef	NO
#define	NO	(0)
#endif

#ifndef	YES
#define	YES	(1)
#endif

struct ucd_handler {
	struct list_head	list;

	u16	psm;
	int	(*receiver)(void *context, struct sk_buff *skb);
	void	*context;
};

int l2cap_ucd_register(struct ucd_handler *h);
int l2cap_ucd_unregister(struct ucd_handler *handler);

#endif /* __L2CAP_UCD_H */

