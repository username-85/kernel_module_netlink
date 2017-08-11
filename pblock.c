#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define NETLINK_USER 31
#define MAX_PORTLEN 5
#define MAX_MSGLEN ((MAX_PORTLEN) + 1)

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Error. Usage: %s port\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	struct sockaddr_nl src_addr = {0};
	struct sockaddr_nl dest_addr = {0};
	struct iovec iov = {0};
	struct msghdr msg = {0};
	struct nlmsghdr *nlh = NULL;
	int sock_fd;

	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
	if (sock_fd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) ) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;    /* For Linux Kernel */
	dest_addr.nl_groups = 0; /* unicast */

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_MSGLEN ));
	memset(nlh, 0, NLMSG_SPACE(MAX_MSGLEN ));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_MSGLEN );
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	strncpy(NLMSG_DATA(nlh), argv[1], MAX_PORTLEN);

	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	printf("sending message to kernel\n");
	sendmsg(sock_fd, &msg, 0);

	exit(EXIT_SUCCESS);
}

