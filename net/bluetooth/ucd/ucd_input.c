/*
	Copyright (c) 2011, Marvell Semiconductor

	@author Ian Carvalho, Global Edge Software Ltd.
*/

/*
	Bluetooth UCD Input Module
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/l2cap.h>
#include <net/bluetooth/l2cap_ucd.h>

#define	BT_UCD_ANY_ID		(~0)

#define	BT_UCD_PSM_INPUT	19

#define	LINUX_EVENT_LENGTH	16

#define	HID_KEY_REPORT		0x01
#define	HID_MOUSE_REPORT	0x02

#define	VERSION			"0.1"

#define	__ENTER__	printk(KERN_INFO "Enter %s:%d.%s\n", \
				__FILE__, __LINE__, __FUNCTION__);
#define	__EXIT__	printk(KERN_INFO "Exit %s:%d.%s\n", \
				__FILE__, __LINE__, __FUNCTION__);

#define	POSTPACK	__attribute__((packed))

struct kbreport {
	unsigned char	modifiers;
	unsigned char	reserved;
	unsigned char	keys[6];
} POSTPACK;

struct mousereport {
	unsigned char	buttons;
	signed char	x;
	signed char	y;
	signed char	wheel;
} POSTPACK;

struct btreport {
	unsigned char	bt_thdr;	/* BT transaction header */
	unsigned char	bt_reportid;
	union {
		struct kbreport kreport;
		struct mousereport mreport;
	};
} POSTPACK;

struct ucd_input_session {
	struct input_dev *input;

	unsigned char	keys[6];
};

/* Table to convert HID codes to Linux Key Event codes */

static unsigned char keycode[256] = {
	  0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
	 50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
	  4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
	 27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
	 65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
	105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
	 72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
	191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
	115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,
	122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
	150,158,159,128,136,177,178,176,142,152,173,140
};

static struct ucd_input_session	inputsession;

static u8 toomanykeys[6] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };

static int ucd_input_report_receiver(void *context, struct sk_buff *skb)
{
	struct ucd_input_session *session = (struct ucd_input_session *)context;
	struct input_dev *dev = session->input;
	struct btreport *report = (struct btreport *)skb->data;
	unsigned char *keys = session->keys;
	unsigned char *inkeys = report->kreport.keys;
	int i;
	unsigned char byte;

	switch (report->bt_reportid) {
	case HID_KEY_REPORT :
		/* process modifier key status... */
		byte = report->kreport.modifiers;
		for (i = 0; i < 8; i++) {
			input_report_key(dev, keycode[i+224], byte & 1);
			byte >>= 1;
		}

		/* check for too many keys pressed */
		if (!memcmp(report->kreport.keys, toomanykeys, 6))
			break;

		/* process received key codes */
		for (i = 0; i < 6; i++) {
			if (keys[i] > 3 && memscan(inkeys, keys[i], 6) ==
								inkeys+6) {
				if (keycode[keys[i]])
					input_report_key(dev,
							keycode[keys[i]], 0);
				else
					BT_ERR("Unknown scancode %#x released",
								keys[i]);
			}

			if (inkeys[i] > 3 && memscan(keys, inkeys[i], 6) ==
								keys+6) {
				if (keycode[inkeys[i]])
					input_report_key(dev,
							keycode[inkeys[i]], 1);
				else
					BT_ERR("Unknown scancode %#x pressed",
								inkeys[i]);
			}
		}

		memcpy(keys, report->kreport.keys, sizeof(session->keys));
		break;

	case HID_MOUSE_REPORT :
		byte = report->mreport.buttons;

		input_report_key(dev, BTN_LEFT,   byte & 0x01);
		input_report_key(dev, BTN_RIGHT,  byte & 0x02);
		input_report_key(dev, BTN_MIDDLE, byte & 0x04);
		input_report_key(dev, BTN_SIDE,   byte & 0x08);
		input_report_key(dev, BTN_EXTRA,  byte & 0x10);

		input_report_rel(dev, REL_X, report->mreport.x);
		input_report_rel(dev, REL_Y, report->mreport.y);

		if (skb->len - 1 > 3)
			input_report_rel(dev, REL_WHEEL, report->mreport.wheel);
		break;
	}

	input_sync(dev);

	return 1;
}

/* Getting Linux key events directly */

static int ucd_input_event_receiver(void *context, struct sk_buff *skb)
{
	struct ucd_input_session *session = (struct ucd_input_session *)context;
	struct input_dev *dev = session->input;
	struct input_event *ev = (struct input_event *)skb->data;

	BT_INFO("t=%d c=%d v=%d", ev->type, ev->code, ev->value);

	switch (ev->type) {
	case EV_KEY :
		input_report_key(dev, ev->code, ev->value);
		break;
	case EV_REL :
		input_report_rel(dev, ev->code, ev->value);
		break;
	}

	input_sync(dev);

	return 0;
}

static int ucd_input_receiver(void *context, struct sk_buff *skb)
{
	BT_INFO("skb->len=%d ", skb->len);

	if (skb->len == LINUX_EVENT_LENGTH)
		return ucd_input_event_receiver(context, skb);

	return ucd_input_report_receiver(context, skb);
}

static int __init ucd_input_setup(struct ucd_input_session *session)
{
	struct input_dev *input;
	int err, i;

	input = input_allocate_device();
	if (!input) {
		return -ENOMEM;
	}

	session->input = input;

	input_set_drvdata(input, session);

	input->name = "Bluetooth UCD Input";

	input->id.bustype = BUS_BLUETOOTH;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;	/**< @todo check this out */

	/* Setup a UCD Keyboard for now. */
	input->evbit[0] = BIT_MASK(EV_KEY);

	for (i = 0; i < sizeof(keycode); i++)
		set_bit(keycode[i], input->keybit);
	clear_bit(0, input->keybit);

	err = input_register_device(input);
	if (err) {
		BT_ERR("ucd_input.c: Failed to register input device\n");
		input_free_device(input);
		session->input = NULL;
	}

	return err;
}

static struct ucd_handler ucd_input_handler = {
	.psm = BT_UCD_PSM_INPUT,
	.receiver = ucd_input_receiver,
	.context = &inputsession
};

static int __init ucd_input_init(void)
{
	int err;

	BT_INFO("UCD (Unicast Connectionless Data) ver %s", VERSION);

	err = ucd_input_setup(&inputsession);

	if (err) {
		BT_ERR("ucd_input.c: Can't setup input interface\n");
		return err;
	}

	err = l2cap_ucd_register(&ucd_input_handler);

	if (err) {
                BT_ERR("ucd_input.c: Can't register UCD interface\n");
		goto err_free_dev;
	}

	return 0;

err_free_dev:
	input_unregister_device(inputsession.input);
	inputsession.input = NULL;

	return err;
}

static void __exit ucd_input_exit(void)
{
	/** @todo figure out how to prevent a module unload */
	if (l2cap_ucd_unregister(&ucd_input_handler)) {
		BT_INFO("ucd_input: Invalid parameters. Cannot unload!");
		return;
	}

	if (inputsession.input)
		input_unregister_device(inputsession.input);
}

module_init(ucd_input_init);
module_exit(ucd_input_exit);

MODULE_AUTHOR("Marvell Semiconductor Inc.");
MODULE_DESCRIPTION("Bluetooth UCD Input ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");

