#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#define NETLINK_USER 31
#define MAX_PORT_NUM 65535

static struct nf_hook_ops nfho;
struct sock *nl_sk = NULL;

static unsigned mod_blocked_port;
static struct rw_semaphore rw_sem;

static unsigned set_mod_blocked_port(unsigned port)
{
	if (port > MAX_PORT_NUM)
		return -1;

	down_write(&rw_sem);
	mod_blocked_port = port;
	up_write(&rw_sem);

	return 0;
}

static unsigned get_mod_blocked_port(void)
{
	unsigned ret;

	down_read(&rw_sem);
	ret = mod_blocked_port;
	up_read(&rw_sem);

	return ret;
}

static void nl_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	unsigned long port;

	nlh = (struct nlmsghdr *)skb->data;
	port = simple_strtoul( (char *)nlmsg_data(nlh), NULL, 0);

	if (set_mod_blocked_port(port) == 0)
		printk(KERN_INFO "will be blocking port:%lu\n", port);
	else
		printk(KERN_ALERT "could not block port:%lu\n", port);
}

unsigned int port_block_hook(void *priv, struct sk_buff *skb,
                             const struct nf_hook_state *state)
{
	struct iphdr *ip_header;
	struct tcphdr *tcp_header;
	struct udphdr *udp_header;
	unsigned blocked_port;
	unsigned dest_port;
	unsigned src_port;

	if (!skb)
		return NF_ACCEPT;

	blocked_port = get_mod_blocked_port();
	if (blocked_port == 0)
		return NF_ACCEPT;

	ip_header = (struct iphdr *)skb_network_header(skb);

	switch (ip_header->protocol) {
	case 6: //TCP
		tcp_header = (struct tcphdr*)
		             ( ((char *)ip_header) + ip_header->ihl * 4);
		dest_port  = (unsigned)ntohs(tcp_header->dest);
		src_port   = (unsigned)ntohs(tcp_header->source);

		if (dest_port == blocked_port) {
			printk(KERN_INFO "TCP packet blocked: src %u, dest %u",
			       src_port, dest_port);
			return NF_DROP;
		}
		break;

	case 17: //UDP
		udp_header = (struct udphdr*)
		             ( ((char *)ip_header) + ip_header->ihl * 4);
		dest_port  = (unsigned)ntohs(udp_header->dest);
		src_port   = (unsigned)ntohs(udp_header->source);

		if (dest_port == blocked_port) {
			printk(KERN_INFO "UDP packet blocked: src %u, dest %u",
			       src_port, dest_port);
			return NF_DROP;
		}
		break;

	default:
		; //nothing to do
	}

	return NF_ACCEPT;
}

static int __init init_mod(void)
{
	// netlink
	struct netlink_kernel_cfg cfg = {
		.input = nl_recv_msg,
	};

	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	if (!nl_sk) {
		printk(KERN_ALERT "netlink_kernle_create error\n");
		return -1;
	}

	// netfilter
	nfho.hook = port_block_hook;
	nfho.hooknum = NF_INET_PRE_ROUTING; //called right after packet recieved, first hook in Netfilter
	nfho.pf = PF_INET;                  //IPV4 packets
	nfho.priority = NF_IP_PRI_FIRST;    //highest priority

	if (nf_register_hook(&nfho)) {
		printk (KERN_ERR "nf_register_hook fail\n");
		return -1;
	}

	return 0;
}

static void __exit exit_mod(void)
{
	netlink_kernel_release(nl_sk);
	nf_unregister_hook(&nfho);
}

module_init(init_mod);
module_exit(exit_mod);
MODULE_LICENSE("GPL");

