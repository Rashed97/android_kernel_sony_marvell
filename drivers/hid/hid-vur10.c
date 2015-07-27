/**
 * HID driver for Vizio vur10 key mappings
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

#include "hid-ids.h"

#define HID_UP_VIZIO_VUR10VENDOR	0xffbc0000
#define vur10_map_key_clear(c)	hid_map_usage_clear(hi, usage, bit, max, \
					EV_KEY, (c))

static int vur10_input_mapping(struct hid_device *hdev, struct hid_input *hi,
		struct hid_field *field, struct hid_usage *usage,
		unsigned long **bit, int *max)
{
	switch (usage->hid & HID_USAGE_PAGE) {
	case HID_UP_VIZIO_VUR10VENDOR:
		switch (usage->hid & HID_USAGE) {
		case 0x48:
			vur10_map_key_clear(KEY_LIST); /* LIST */
			break;
		case 0x5d:
			vur10_map_key_clear(KEY_YELLOW); /* YELLOW */
			break;
		case 0x5e:
			vur10_map_key_clear(KEY_BLUE); /* BLUE */
			break;
		case 0x5b:
			vur10_map_key_clear(KEY_RED); /* RED */
			break;
		case 0x5c:
			vur10_map_key_clear(KEY_GREEN); /* GREEN */
			break;
		case 0x0d:
			vur10_map_key_clear(KEY_HOME); /* HOME */
			break;
		case 0x82:
			vur10_map_key_clear(KEY_FN_F1); /* PIP */
			break;
		case 0x90:
			vur10_map_key_clear(KEY_FN_F2); /* SWAP */
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

static const struct hid_device_id vur10_devices[] = {
	/* vizio vur10 Bluetooth Remote*/
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_VIZIO, USB_DEVICE_ID_VIZIO_VUR10_REMOTE) },
	{ }
};
MODULE_DEVICE_TABLE(hid, vur10_devices);

static struct hid_driver vur10_driver = {
	.name = "vizio vur10",
	.id_table = vur10_devices,
	.input_mapping = vur10_input_mapping,
};

static int __init vur10_init(void)
{
	return hid_register_driver(&vur10_driver);
}

static void __exit vur10_exit(void)
{
	hid_unregister_driver(&vur10_driver);
}

module_init(vur10_init);
module_exit(vur10_exit);
MODULE_LICENSE("GPL v2");
