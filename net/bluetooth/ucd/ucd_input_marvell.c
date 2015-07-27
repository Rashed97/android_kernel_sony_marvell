/*
   Copyright (c) 2012, Marvell Semiconductor

   Bluetooth UCD Input Module
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/l2cap.h>
#include <net/bluetooth/l2cap_ucd.h>
#include <linux/unaligned/le_byteshift.h>

/*Default ucd psm*/
int bt_ucd_psm_marvell = 0x1011;

/*Default ucd packet length*/
int marvell_ucd_packet_length = 1;

/*Default ucd power key value*/
int marvell_ucd_key_power = 0x82;


#define VERSION "0.1"

struct ucd_input_session {
    struct input_dev *input;
    struct timer_list timer;
    unsigned char keytorelease;
};

static struct ucd_input_session inputsession;

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
    u16 power_key;

    if (1 == marvell_ucd_packet_length){
        if (*data == marvell_ucd_key_power)
            *data = KEY_POWER;
    }
    else if (2 == marvell_ucd_packet_length){
        power_key = get_unaligned_le16(skb->data);
        if (power_key == marvell_ucd_key_power)
            *data = KEY_POWER;
    }
    else {
        BT_ERR("ucd_input_receiver: unsupported ucd packet length\n");
        return -EINVAL;
    }


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

    input->name = "marvell_ucd_remote";

    input->id.bustype = BUS_BLUETOOTH;
    input->id.vendor = 0x0001;
    input->id.product = 0x0001;
    input->id.version = 0x0001;

    /* Setup a UCD Keyboard for now. */
    input->evbit[0] = BIT_MASK(EV_KEY);

    for (i = 0; i < 127; i++)
        set_bit(i, input->keybit);

    clear_bit(0, input->keybit);

    err = input_register_device(input);
    if (err) {
        BT_ERR("ucd_input_marvell: Failed to register input device\n");
        input_free_device(input);
        session->input = NULL;
    }

    return err;
}

static struct ucd_handler ucd_input_handler;

static int __init ucd_input_marvell_init(void)
{
    int err;

    BT_INFO("MARVELL UCD Input ver %s", VERSION);

    if (marvell_ucd_packet_length > 2){
        BT_ERR("ucd_input_marvell_init: unsupported ucd packet length\n");
        return -EINVAL;
    }

    err = ucd_input_setup(&inputsession);

    if (err) {
        BT_ERR("ucd_input_marvell_init: Can't setup input interface\n");
        return err;
    }

    ucd_input_handler.psm = bt_ucd_psm_marvell;
    ucd_input_handler.receiver = ucd_input_receiver;
    ucd_input_handler.context = &inputsession;

    err = l2cap_ucd_register(&ucd_input_handler);

    if (err) {
        BT_ERR("ucd_input_marvell_init: Can't register UCD interface\n");
        goto err_free_dev;
    }

    return 0;

err_free_dev:
    input_unregister_device(inputsession.input);
    inputsession.input = NULL;

    return err;
}

static void __exit ucd_input_marvell_exit(void)
{
    if (l2cap_ucd_unregister(&ucd_input_handler)) {
        BT_INFO("ucd_input_marvell: Invalid parameters. Cannot unload!");
        return;
    }

    if (inputsession.input)
        input_unregister_device(inputsession.input);

    while (timer_pending(&inputsession.timer))
        mdelay(100);
}

module_init(ucd_input_marvell_init);
module_exit(ucd_input_marvell_exit);

MODULE_AUTHOR("Marvell Semiconductor Inc.");
MODULE_DESCRIPTION("MARVELL Bluetooth UCD Input ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
module_param(bt_ucd_psm_marvell, int, 1);
MODULE_PARM_DESC(bt_ucd_psm_marvell,
        "Bluetooth UCD psm value, default is 0x1011");
module_param(marvell_ucd_packet_length, int, 1);
MODULE_PARM_DESC(marvell_ucd_packet_length,
        "Bluetooth UCD packet length, should be no longer than 2, default is 1");
module_param(marvell_ucd_key_power, int, 1);
MODULE_PARM_DESC(marvell_ucd_key_power,
        "Bluetooth UCD power key value, default is 0x82");

