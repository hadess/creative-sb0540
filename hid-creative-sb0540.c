/*
 * HID driver for the Creative SB0540 receiver
 *
 * Copyright (C) 2019 Red Hat Inc. All Rights Reserved
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include "hid-ids.h"

MODULE_AUTHOR("Bastien Nocera <hadess@hadess.net>");
MODULE_DESCRIPTION("HID Creative SB0540 receiver");
MODULE_LICENSE("GPL");

//FIXME implement
static const unsigned short creative_sb0540_key_table[] = {
	KEY_RESERVED,
};

struct creative_sb0540 {
	struct input_dev *input_dev;
	struct hid_device *hid;
	unsigned short keymap[ARRAY_SIZE(creative_sb0540_key_table)];
};

static inline u64 reverse(u64 data, int bits)
{
	int i;
	u64 c;

	c = 0;
	for (i = 0; i < bits; i++) {
		c |= (u64) (((data & (((u64) 1) << i)) ? 1 : 0)) << (bits - 1 - i);
	}
	return (c);
}

static int get_key(u64 keycode)
{
	/* From remotes/creative/lircd.conf.alsa_usb in lirc */

	//FIXME impl

	return 0;

}

static int creative_sb0540_raw_event(struct hid_device *hid, struct hid_report *report,
	 u8 *data, int len)
{
	struct creative_sb0540 *creative_sb0540 = hid_get_drvdata(hid);
	u64 code, main_code;
	int key;

	hid_err(hid, "creative_sb0540_raw_event: len %d\n", len);

	if (len != 6)
		goto out;

	hid_err(hid, "data: 0x%x\n", data[5]);

	/* From daemons/hw_hiddev.c sb0540_rec() in lirc */
	code = reverse(data[5], 8);
	main_code = (code << 8) + ((~code) & 0xff);

	/* Flip to get values in the same format as
	 * remotes/creative/lircd.conf.alsa_usb in lirc */
	main_code = ((main_code & 0xff) << 8) + ((main_code & 0xff00) >> 8);

	hid_err(hid, "main_code: 0x%llX\n", main_code);

	key = get_key(main_code);

	if (!key) {
		hid_err(hid, "Could not get a key for main_code %llX\n", main_code);
		goto out;
	}

	input_report_key(creative_sb0540->input_dev, key, 1);
	input_report_key(creative_sb0540->input_dev, key, 0);
	input_sync(creative_sb0540->input_dev);

out:
	/* let hidraw and hiddev handle the report */
	return 0;
}

static int creative_sb0540_input_configured(struct hid_device *hid,
		struct hid_input *hidinput)
{
	struct input_dev *input_dev = hidinput->input;
	struct creative_sb0540 *creative_sb0540 = hid_get_drvdata(hid);
	int i;

	creative_sb0540->input_dev = input_dev;

	input_dev->keycode = creative_sb0540->keymap;
	input_dev->keycodesize = sizeof(unsigned short);
	input_dev->keycodemax = ARRAY_SIZE(creative_sb0540->keymap);

	input_dev->evbit[0] = BIT(EV_KEY) | BIT(EV_REP);

	memcpy(creative_sb0540->keymap, creative_sb0540_key_table, sizeof(creative_sb0540->keymap));
	for (i = 0; i < ARRAY_SIZE(creative_sb0540_key_table); i++)
		set_bit(creative_sb0540->keymap[i], input_dev->keybit);
	clear_bit(KEY_RESERVED, input_dev->keybit);

	return 0;
}

static int creative_sb0540_input_mapping(struct hid_device *hid,
		struct hid_input *hi, struct hid_field *field,
		struct hid_usage *usage, unsigned long **bit, int *max)
{
	return -1;
}

static int creative_sb0540_probe(struct hid_device *hid, const struct hid_device_id *id)
{
	int ret;
	struct creative_sb0540 *creative_sb0540;

	creative_sb0540 = kzalloc(sizeof(struct creative_sb0540), GFP_KERNEL);
	if (!creative_sb0540) {
		ret = -ENOMEM;
		goto allocfail;
	}

	creative_sb0540->hid = hid;

	/* force input as some remotes bypass the input registration */
	hid->quirks |= HID_QUIRK_HIDINPUT_FORCE;

	hid_set_drvdata(hid, creative_sb0540);

	ret = hid_parse(hid);
	if (ret) {
		hid_err(hid, "parse failed\n");
		goto fail;
	}

	ret = hid_hw_start(hid, HID_CONNECT_DEFAULT | HID_CONNECT_HIDDEV_FORCE);
	if (ret) {
		hid_err(hid, "hw start failed\n");
		goto fail;
	}

	return 0;
fail:
	kfree(creative_sb0540);
allocfail:
	return ret;
}

static void creative_sb0540_remove(struct hid_device *hid)
{
	struct creative_sb0540 *creative_sb0540 = hid_get_drvdata(hid);
	hid_hw_stop(hid);
	kfree(creative_sb0540);
}

static const struct hid_device_id creative_sb0540_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_CREATIVELABS, USB_DEVICE_ID_CREATIVE_SB0540) },
	{ }
};
MODULE_DEVICE_TABLE(hid, creative_sb0540_devices);

static struct hid_driver creative_sb0540_driver = {
	.name = "creative-sb0540",
	.id_table = creative_sb0540_devices,
	.raw_event = creative_sb0540_raw_event,
	.input_configured = creative_sb0540_input_configured,
	.probe = creative_sb0540_probe,
	.remove = creative_sb0540_remove,
	.input_mapping = creative_sb0540_input_mapping,
};
module_hid_driver(creative_sb0540_driver);
