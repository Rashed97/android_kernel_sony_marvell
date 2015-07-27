#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <mach/galois_generic.h>
#include <linux/input.h>
#include <linux/sched.h>
#include <linux/sort.h>

#include <mach/sm.h>
#include <mach/ir_key_def.h>

struct ir_keymap {
	MV_IR_KEY_CODE_t ircode;
	__u32 keycode;
};

#define MAKE_KEYCODE(modifier, scancode) ((modifier<<16)|scancode)

#define MAKE_IR_KEYMAP(i, m, s)		\
	{.ircode = i, .keycode = MAKE_KEYCODE(m, s) }

static inline int galois_ir_get_modifier(struct ir_keymap *keymap)
{
	return (keymap->keycode>>16) & 0xffff;
}

static inline int galois_ir_get_scancode(struct ir_keymap *keymap)
{
	return keymap->keycode & 0xffff;
}

static int __init galois_ir_cmp(const void *x1, const void *x2)
{
	const struct ir_keymap *ir1 = (const struct ir_keymap *)x1;
	const struct ir_keymap *ir2 = (const struct ir_keymap *)x2;

	return ir1->ircode - ir2->ircode;
}

static struct ir_keymap *
galois_ir_bsearch(struct ir_keymap *keymap,
		  int nr_keymap,
		  MV_IR_KEY_CODE_t ircode)
{
	int i, j, m;

	i = 0;
	j = nr_keymap-1;
	while (i <= j) {
		m = (i + j) / 2;
		if (keymap[m].ircode == ircode)
			return &(keymap[m]);
		else if (keymap[m].ircode > ircode)
			j = m-1;
		else
			i = m+1;
	}
	return NULL;
}

/* include key map table */
#ifdef CONFIG_MV88DE3100_IR_MARVELL
#include "ir-marvell.c"
#endif

#ifdef CONFIG_MV88DE3100_IR_VIAPLUS
#include "ir-viaplus.c"
#endif

#define MAX_LEN 10

static DECLARE_WAIT_QUEUE_HEAD(girl_wait);

static DEFINE_SPINLOCK(my_lock);

static struct {
	int number;
	int girl_buf[MAX_LEN];
	int in;
	int out;
} mygirl;

static void girl_key_buf_write(int key)
{
	spin_lock(&my_lock);
	mygirl.girl_buf[mygirl.in] = key;
	mygirl.in++;
	mygirl.number++;
	if (mygirl.in == MAX_LEN)
		mygirl.in = 0;
	spin_unlock(&my_lock);
}

static int girl_key_buf_read(void)
{
	int key;
	spin_lock(&my_lock);
	key = mygirl.girl_buf[mygirl.out];
	mygirl.out++;
	mygirl.number--;
	if (mygirl.out == MAX_LEN)
		mygirl.out = 0;
	spin_unlock(&my_lock);
	return key;
}

static struct input_dev *ir_input = NULL;

static int ir_input_open(struct input_dev *dev)
{
	int msg;
	msg = MV_SM_IR_Linuxready;
	bsm_msg_send(MV_SM_ID_IR, &msg, 4);
	/* The linux ready mark should be set in the power module in new SM,
	 * so I send it to the ID_POWER, but consider that old SM is still in
	 * use, so I can't remove the msg sent to IR before we abandon old SM.
	 */
	bsm_msg_send(MV_SM_ID_POWER, &msg, 4);
	return 0;
}

static void ir_report_key(int ir_key)
{
	struct ir_keymap *km;

	km = galois_ir_bsearch(keymap_tab,
			ARRAY_SIZE(keymap_tab), (ir_key & 0xffff));
	if (km) {
		int scancode = galois_ir_get_scancode(km);

		if (ir_key & 0x80000000)
			input_event(ir_input, EV_KEY, scancode, 2);
		else {
			int pressed = !(ir_key & 0x8000000);
			int modifier = galois_ir_get_modifier(km);

			if (pressed && modifier)
				input_event(ir_input, EV_KEY,
						modifier, pressed);

			input_event(ir_input, EV_KEY, scancode, pressed);

			if (!pressed && modifier)
				input_event(ir_input, EV_KEY,
						modifier, pressed);
		}

		input_sync(ir_input);
	}
}

/*
 * when sm receives msg and the msg is to ir, SM ISR will call this function.
 * SM needs to work under interrupt mode
 */
void girl_sm_int(void)
{
	int msg[8], len, ret, ir_key;

	ret = bsm_msg_receive(MV_SM_ID_IR, msg, &len);

	if(ret != 0 ||  len != 4)
		return;

	ir_key = msg[0];
	ir_report_key(ir_key);
	girl_key_buf_write(ir_key);
	wake_up_interruptible(&girl_wait);
}

static int galois_ir_open(struct inode *inode, struct file *file)
{
	int msg;

	msg = MV_SM_IR_Linuxready;
	bsm_msg_send(MV_SM_ID_IR, &msg, 4);

	return 0;
}


static ssize_t galois_ir_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	int ret;
	int key;

	key = girl_key_buf_read();
	ret = put_user(key, (unsigned int __user *)buf);

	if (ret)
		return ret;
	else
		return sizeof(key);
}

static ssize_t galois_ir_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	printk("error: doesn't support write.\n");
	return -EFAULT;
}

static unsigned int galois_ir_poll(struct file *file, poll_table *wait)
{
	int tmp;
	spin_lock(&my_lock);
	tmp = mygirl.number;
	spin_unlock(&my_lock);
	if (!tmp)
		poll_wait(file, &girl_wait, wait);
	spin_lock(&my_lock);
	tmp = mygirl.number;
	spin_unlock(&my_lock);
	if (tmp)
		return (POLLIN | POLLRDNORM);
	return 0;
}

static struct file_operations galois_ir_fops = {
	.owner		= THIS_MODULE,
	.open		= galois_ir_open,
	.write		= galois_ir_write,
	.read		= galois_ir_read,
	.poll		= galois_ir_poll,
};

static struct miscdevice ir_dev = {
	.minor	= GALOIS_IR_MISC_MINOR,
	.name	= GALOIS_IR_NAME,
	.fops	= &galois_ir_fops,
};

static int __init galois_ir_init(void)
{
	int i, error, scancode, modifier;

	if (misc_register(&ir_dev))
		return -ENODEV;

	ir_input = input_allocate_device();
	if (!ir_input) {
		printk("error: failed to alloc input device for IR.\n");
		misc_deregister(&ir_dev);
		return -ENOMEM;
	}

	ir_input->name = "Inafra-Red";
	ir_input->phys = "Inafra-Red/input0";
	ir_input->id.bustype = BUS_HOST;
	ir_input->id.vendor = 0x0001;
	ir_input->id.product = 0x0001;
	ir_input->id.version = 0x0100;

	ir_input->open = ir_input_open;

	/* sort the keymap in order */
	sort(keymap_tab, ARRAY_SIZE(keymap_tab),
		sizeof(struct ir_keymap), galois_ir_cmp, NULL);

	for (i = 0; i < ARRAY_SIZE(keymap_tab); i++) {
		scancode = galois_ir_get_scancode(&(keymap_tab[i]));
		modifier = galois_ir_get_modifier(&(keymap_tab[i]));
		__set_bit(scancode, ir_input->keybit);
		if (modifier)
			__set_bit(modifier, ir_input->keybit);
	}
	__set_bit(EV_KEY, ir_input->evbit);

	error = input_register_device(ir_input);
	if (error) {
		printk("error: failed to register input device for IR\n");
		misc_deregister(&ir_dev);
		input_free_device(ir_input);
		return error;
	}
	return 0;
}

static void __exit galois_ir_exit(void)
{
	misc_deregister(&ir_dev);
	input_unregister_device(ir_input);
	input_free_device(ir_input);
}

module_init(galois_ir_init);
module_exit(galois_ir_exit);

MODULE_AUTHOR("Marvell-Galois");
MODULE_DESCRIPTION("Galois Infra-Red Driver");
MODULE_LICENSE("GPL");
