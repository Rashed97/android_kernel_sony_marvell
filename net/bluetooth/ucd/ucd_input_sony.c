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

#define	BT_UCD_PSM_SONY		0x1011

#define	SONY_UCDPACKET_LENGTH	1

#define	SONY_KEY_POWER		0x82

#define	VERSION			"0.2"

#define	__ENTER__	printk(KERN_INFO "Enter %s:%d.%s\n", \
				__FILE__, __LINE__, __FUNCTION__);
#define	__EXIT__	printk(KERN_INFO "Exit %s:%d.%s\n", \
				__FILE__, __LINE__, __FUNCTION__);

struct ucd_input_session {
	struct input_dev *input;
	struct timer_list timer;
	unsigned char keytorelease;
};

static struct ucd_input_session	inputsession;

static void key_release_timer(unsigned long arg)
{
	struct ucd_input_session *session = (struct ucd_input_session *)arg;
	struct input_dev *dev = session->input;

	input_report_key(dev, session->keytorelease, 0);
	input_sync(dev);
}

static int ucd_input_receiver(void *context, struct sk_buff *skb)
{
	struct ucd_input_session *session = (struct ucd_input_session *)context;
	struct input_dev *dev = session->input;
	unsigned char *data = skb->data;

	if (*data == SONY_KEY_POWER)
		*data = KEY_POWER;

	/* Each key received assume key press */
	input_report_key(dev, *data, 1);
	input_sync(dev);

	/* and key release on a timer */
	session->keytorelease = *data;
	session->timer.data = (unsigned long)context;
	session->timer.function = key_release_timer;

	mod_timer(&session->timer, jiffies + (50 * HZ / 1000));

	return 0;
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

	init_timer(&session->timer);

	input_set_drvdata(input, session);

	input->name = "sony_ucd_remote";

	input->id.bustype = BUS_BLUETOOTH;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0001;	/**< @todo check this out */

	/* Setup a UCD Keyboard for now. */
	input->evbit[0] = BIT_MASK(EV_KEY);

	for (i = 0; i < 127; i++)
		set_bit(i, input->keybit);

	clear_bit(0, input->keybit);

	err = input_register_device(input);
	if (err) {
		BT_ERR("ucd_input_sony: Failed to register input device\n");
		input_free_device(input);
		session->input = NULL;
	}

	return err;
}

static struct ucd_handler ucd_input_handler = {
	.psm = BT_UCD_PSM_SONY,
	.receiver = ucd_input_receiver,
	.context = &inputsession
};

static int __init ucd_input_sony_init(void)
{
	int err;

	BT_INFO("SONY UCD Input ver %s", VERSION);

	err = ucd_input_setup(&inputsession);

	if (err) {
		BT_ERR("ucd_input_sony: Can't setup input interface\n");
		return err;
	}

	err = l2cap_ucd_register(&ucd_input_handler);

	if (err) {
                BT_ERR("ucd_input_sony: Can't register UCD interface\n");
		goto err_free_dev;
	}

	return 0;

err_free_dev:
	input_unregister_device(inputsession.input);
	inputsession.input = NULL;

	return err;
}

static void __exit ucd_input_sony_exit(void)
{
	if (l2cap_ucd_unregister(&ucd_input_handler)) {
		BT_INFO("ucd_input_sony: Invalid parameters. Cannot unload!");
		return;
	}

	if (inputsession.input)
		input_unregister_device(inputsession.input);

	while (timer_pending(&inputsession.timer))
		mdelay(100);
}

module_init(ucd_input_sony_init);
module_exit(ucd_input_sony_exit);

MODULE_AUTHOR("Marvell Semiconductor Inc.");
MODULE_DESCRIPTION("SONY Bluetooth UCD Input ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
