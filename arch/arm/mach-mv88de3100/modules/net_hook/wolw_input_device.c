/*
	Copyright (c) 2011, Marvell Semiconductor

	@author Ian Carvalho, Global Edge Software Ltd.
*/

/*
	WOLW UCD Input Module
*/

#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include "wolw_input.h"
#define	WOLW_UCD_ANY_ID		(~0)

#define	WOLW_UCD_PSM_SONY		0x1011

#define	WOLW_UCDPACKET_LENGTH	1

#define	WOLW_KEY_POWER		0x82


#define	__ENTER__	printk(KERN_INFO "Enter %s:%d.%s\n", \
				__FILE__, __LINE__, __FUNCTION__);
#define	__EXIT__	printk(KERN_INFO "Exit %s:%d.%s\n", \
				__FILE__, __LINE__, __FUNCTION__);

struct wolw_input_session {
	struct input_dev *input;
	struct timer_list timer;
	unsigned char keytorelease;
};

static struct wolw_input_session	inputsession;

static void key_release_timer(unsigned long arg)
{
	struct wolw_input_session *session = (struct wolw_input_session *)arg;
	struct input_dev *dev = session->input;

	input_report_key(dev, session->keytorelease, 0);
	input_sync(dev);
}

int wolw_input_receiver(unsigned char data)
{

	if(inputsession.input == NULL)
		return -1;

	if (data == WOLW_KEY_POWER)
		data = KEY_POWER;

	/* Each key received assume key press */
	input_report_key(inputsession.input, data, 1);
	input_sync(inputsession.input);

	/* and key release on a timer */
	inputsession.keytorelease = data;
	inputsession.timer.data = &inputsession;
	inputsession.timer.function = key_release_timer;

	mod_timer(&inputsession.timer, jiffies + (50 * HZ / 1000));

	return 0;
}

int __init wolw_input_setup(void)
{
	struct input_dev *input;
	int err, i;

	input = input_allocate_device();
	if (!input) {
		return -ENOMEM;
	}

	inputsession.input = input;

	init_timer(&inputsession.timer);

	input_set_drvdata(input, &inputsession);

	input->name = "wolw_ucd_remote";

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
		printk("wol2_input_setup: Failed to register input device\n");
		input_free_device(input);
		inputsession.input = NULL;
	}
	//kthread_run(wolw_test_thread, input, "wolw_test");
	return err;
}


void __exit wolw_input_exit(void)
{

	if (inputsession.input)
		input_unregister_device(inputsession.input);

	while (timer_pending(&inputsession.timer))
		mdelay(100);
}
