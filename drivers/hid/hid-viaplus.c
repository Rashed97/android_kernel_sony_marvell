/**
 * HID driver for vizio viaplus key mappings
 *
 * Copyright (C) 2011, Marvell International Ltd.
 *
 * This software file (the "File") is distributed by Marvell International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available by writing to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
 * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
 *
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 **/

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/ktime.h>

#include "hid-ids.h"
#define HID_UP_VIZIO_VIAPLUS_VENDOR	0xffbc0000
#define HID_UP_VIZIO_MSVENDOR		0xff000000
#define HID_UP_VIZIO_GENDESK		0x00010000

#define map_key_clear(c)		\
	hid_map_usage_clear(hi, usage, bit, max, EV_KEY, (c))

struct viaplus_parameters {
	int is_first_event;
};


static int viaplus_probe(struct hid_device *hdev,
		const struct hid_device_id *id)
{
	int ret = 0;
	struct viaplus_parameters *fevt;

	fevt = kzalloc(sizeof(*fevt), GFP_KERNEL);
	if (fevt == NULL) {
		dev_err(&hdev->dev, "can't alloc viaplus flag\n");
		return -ENOMEM;
	}

	fevt->is_first_event = 1;

	hid_set_drvdata(hdev, fevt);

	ret = hid_parse(hdev);
	if (ret) {
		dev_err(&hdev->dev, "parse failed\n");
		goto err_free;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		dev_err(&hdev->dev, "hw start failed\n");
		goto err_free;
	}

	return 0;
err_free:
	kfree(fevt);
	return ret;

}

static void viaplus_remove(struct hid_device *hdev)
{
	hid_hw_stop(hdev);
	kfree(hid_get_drvdata(hdev));
}

static int input_mapping(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	switch (usage->hid & HID_USAGE_PAGE) {
	case HID_UP_CONSUMER:
		switch (usage->hid & HID_USAGE) {
		case 0x020:
			map_key_clear(KEY_BATTERY); /* Battery */
			break;
		case 0x067:
			map_key_clear(KEY_F6); /* PIP */
			break;
		case 0x084:
			map_key_clear(KEY_FN_F5); /* DASH */
			break;
		case 0x082:
			map_key_clear(KEY_TV_INPUT); /* INPUT */
			break;
		case 0x0b7:
			map_key_clear(KEY_STOP); /* STOP */
			break;
		case 0x209:
			map_key_clear(KEY_PROGRAM_INFO); /* INFO */
			break;
		case 0x030:
			map_key_clear(KEY_TV_POWER);  /*Map to TV Power for XRA RC*/
			break;
		default:
			return 0;
		}
		break;
	case HID_UP_VIZIO_VIAPLUS_VENDOR:
		switch (usage->hid & HID_USAGE) {
		case 0x48:
			map_key_clear(KEY_LIST); /* LIST */
			break;
		case 0x0d:
			map_key_clear(KEY_HOME); /* HOME */
			break;
		default:
			return 0;
		}
		break;
	case HID_UP_VIZIO_MSVENDOR:
		switch (usage->hid & HID_USAGE) {
		case 0x01:
			map_key_clear(KEY_FN_F1); /* AMAZON */
			break;
		case 0x02:
			map_key_clear(KEY_FN_F2); /* NETFLIX */
			break;
		case 0x03:
			map_key_clear(KEY_FN_F3); /* VUDU */
			break;
		case 0x04:
			map_key_clear(KEY_FN_F4); /* QUICK */
			break;
		default:
			return 0;
		}
		break;
	case HID_UP_VIZIO_GENDESK:
		switch (usage->hid & HID_USAGE) {
		case 0x82:
			map_key_clear(KEY_POWER); /* POWER */
			break;
		default:
			return 0;
		}
		break;
	default:
		return 0;
	}

	/* return a 1 means a matching mapping is found, otherwise
	 * need HID driver to search mapping in hid-input.c
	 */
	return 1;
}

static int viaplus_event(struct hid_device *hdev, struct hid_field *field,
			                        struct hid_usage *usage, __s32 value)
{
	char event_string[20] = {0};
	char *envp[] = { event_string, NULL };
	struct viaplus_parameters *fevt = hid_get_drvdata(hdev);;

	if (!(hdev->claimed & HID_CLAIMED_INPUT) || !field->hidinput ||
			!usage->type)
		return 0;

	if (fevt->is_first_event) {
		fevt->is_first_event = 0;
		msleep(80);
	}

	if (usage->hid == (HID_UP_CONSUMER | 0x0020)){
		sprintf(event_string, "BATTERY=%d", value);
		kobject_uevent_env(&(hdev->dev.kobj), KOBJ_CHANGE, envp);
		return 1;
	}
	return 0;
}

static const struct hid_device_id viaplus_devices[] = {
	/* Vizio viaplus Bluetooth Remote*/
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_VIZIO, USB_DEVICE_ID_VIZIO_VIAPLUS_REMOTE) },
	{ }
};
MODULE_DEVICE_TABLE(hid, viaplus_devices);

static struct hid_driver viaplus_driver = {
	.name = "vizio viaplus",
	.id_table = viaplus_devices,
	.probe = viaplus_probe,
	.remove = viaplus_remove,
	.input_mapping = input_mapping,
	.event = viaplus_event,
};

static int __init init(void)
{
	return hid_register_driver(&viaplus_driver);
}

static void __exit exit(void)
{
	hid_unregister_driver(&viaplus_driver);
}

module_init(init);
module_exit(exit);
MODULE_LICENSE("GPL v2");
