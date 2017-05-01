#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <sys/wait.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/un.h>  
#include <linux/socket.h>
#include <poll.h>

#define SCM_CREDENTIALS 0x02
struct uevent {
	const char *action;
	const char *path;
	const char *subsystem;
	const char *firmware;
	int major;
	int minor;
	const char *keyevent;
};

struct ucred {
	__u32   pid;
	__u32   uid;
	__u32   gid;
};

static int open_uevent_socket(void)
{
	struct sockaddr_nl addr;
	int sz = 64*1024;
	int on = 1;
	int s;

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = 0xffffffff;

	s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if(s < 0)
		return -1;

	setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));
	setsockopt(s, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

	if(bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		close(s);
		return -1;
	}

	return s;
}

static void parse_event(const char *msg, struct uevent *uevent)
{
#if 1
	uevent->action = "";
	uevent->path = "";
	uevent->subsystem = "";
	uevent->firmware = "";
	uevent->major = -1;
	uevent->minor = -1;

	while(*msg) {
		printf("%s\n", msg);
		if(!strncmp(msg, "ACTION=", 7)) {
			msg += 7;
			uevent->action = msg;
		} else if(!strncmp(msg, "DEVPATH=", 8)) {
			msg += 8;
			uevent->path = msg;
		} else if(!strncmp(msg, "SUBSYSTEM=", 10)) {
			msg += 10;
			uevent->subsystem = msg;
		} else if(!strncmp(msg, "FIRMWARE=", 9)) {
			msg += 9;
			uevent->firmware = msg;
		} else if(!strncmp(msg, "MAJOR=", 6)) {
			msg += 6;
			uevent->major = atoi(msg);
		} else if(!strncmp(msg, "MINOR=", 6)) {
			msg += 6;
			uevent->minor = atoi(msg);
		} else if(!strncmp(msg, "ALARM=", 6)) { //ALARM事件过滤此字段
			msg += 6;
			uevent->keyevent = msg;
			printf("msg=%s\n", msg);
		}

		while(*msg++)
			;
	}

	printf("event { '%s', '%s', '%s', '%s', %d, %d }\n",
			uevent->action, uevent->path, uevent->subsystem,
			uevent->firmware, uevent->major, uevent->minor);
#endif
}

#define UEVENT_MSG_LEN 1024
void handle_device_fd(int fd)
{
	printf("enter %s\n", __func__);
	char msg[UEVENT_MSG_LEN+2];
	char cred_msg[CMSG_SPACE(sizeof(struct ucred))];
	struct iovec iov = {msg, sizeof(msg)};
	struct sockaddr_nl snl;
	struct msghdr hdr = {&snl, sizeof(snl), &iov, 1, cred_msg, sizeof(cred_msg), 0};
	while (1) {
		struct pollfd fds;
		int nr;

		fds.fd = fd;
		fds.events = POLLIN;
		fds.revents = 0;
		nr = poll(&fds, 1, -1); //非阻塞

		if(nr > 0 && (fds.revents & POLLIN)) {
			ssize_t n = recvmsg(fd, &hdr, 0);
			if (n <= 0) {
				break;
			}

			if ((snl.nl_groups != 1) || (snl.nl_pid != 0)) {
				continue;
			}

			struct cmsghdr * cmsg = CMSG_FIRSTHDR(&hdr);
			if (cmsg == NULL || cmsg->cmsg_type != SCM_CREDENTIALS) {
				continue;
			}

			struct ucred * cred = (struct ucred *)CMSG_DATA(cmsg);
			if (cred->uid != 0) {
				continue;
			}

			if(n >= UEVENT_MSG_LEN)
				continue;

			msg[n] = '\0';
			msg[n+1] = '\0';

			struct uevent uevent;
			parse_event(msg, &uevent);
		}
	}
}

int main(void)
{
	int fd = 0;

	fd = open_uevent_socket();
	if (fd < 0) {
		printf("error!\n");
		return -1;
	}
	handle_device_fd(fd);
}
