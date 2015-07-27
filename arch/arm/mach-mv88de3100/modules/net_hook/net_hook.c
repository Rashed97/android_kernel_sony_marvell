#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/tcp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_bridge.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/init.h>

#include <linux/input.h>
#include "wolw_input.h"

#define VERSION         "0.2"

static struct nf_hook_ops nfho;

////////////////
#define UDP_HDRLEN			8
#define MAGIC_PACKET_PORT	7
#define MAGIC_PACKET_COUNT	16
#define MAGIC_PACKET_SIZE	102
#define MAC_ADDR_LENTH		6

//////////////
static unsigned int hook_func(unsigned int hooknum,
						struct sk_buff *skb,
						const struct net_device *in,
						const struct net_device *out,
						int (*okfn)(struct sk_buff *))
{
	struct udphdr *udp_header;          // UDP header struct
	struct iphdr *ip_header;            // IP header struct
	struct sk_buff *sb = skb;
	unsigned int len;
	int i;
	unsigned char m[MAC_ADDR_LENTH] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, dstmem[MAGIC_PACKET_SIZE], *ptr;

	ip_header = (struct iphdr *)skb_network_header(sb);

	if (!sb)
		return NF_ACCEPT;

	if (ip_header->protocol == IPPROTO_UDP) {
		udp_header = (struct udphdr *)(skb_transport_header(sb) + ip_hdrlen(sb));

		if (udp_header)
			if (MAGIC_PACKET_PORT == ntohs(udp_header->dest)) {
				memcpy(dstmem, m, MAC_ADDR_LENTH);
				memcpy(m, in->dev_addr, MAC_ADDR_LENTH);
				for (i = 1; i <= MAGIC_PACKET_COUNT; i++)
					memcpy(dstmem + i*MAC_ADDR_LENTH, m, MAC_ADDR_LENTH);

				len = ip_hdrlen(sb);
				skb_pull(sb, len);
				skb_pull(sb, UDP_HDRLEN);

				/*start to match magic packet*/
				int ret = memcmp(dstmem, sb->data, MAGIC_PACKET_SIZE);

				if (ret == 0) {
					printk(KERN_INFO"pass!");
					wolw_input_receiver(KEY_ESC);
				} else
					printk(KERN_INFO"not pass!");
			}
	}
	return NF_ACCEPT;
}


static int __init mod_init_func(void)
{
	int i = 0;

	i = wolw_input_setup();
	if (i < 0) {
		pr_info("input device init error!\n");
		return i;
	}
	pr_info("ip filter init....\n");

	nfho.hook = hook_func;
	nfho.hooknum = 0;
	nfho.pf = PF_INET;
	nfho.priority = NF_IP_PRI_FIRST;
	nf_register_hook(&nfho);

	return 0;
}

static void __exit mod_exit_func(void)
{
	wolw_input_exit();
	nf_unregister_hook(&nfho);
	pr_info("ip filter remove...\n");
}

module_init(mod_init_func);
module_exit(mod_exit_func);
MODULE_AUTHOR("Marvell Semiconductor Inc.");
MODULE_DESCRIPTION("SONY Bluetooth UCD Input ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
