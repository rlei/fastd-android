/***************************************************************************
 * libancillary - black magic on Unix domain sockets
 * (C) Nicolas George
 ***************************************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* libancillary code from:
 *   http://www.normalesup.org/~george/comp/libancillary/
 * with minor indent/style adjusts to fit fastd project
 */

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "fastd.h"

#define ANCIL_FD_BUFFER(n) \
	struct { \
		struct cmsghdr h; \
		int fd[n]; \
	}

static int ancil_recv_fds_with_buffer(int sock, int *fds, unsigned n_fds, void *buffer) {
	struct msghdr msghdr;
	char nothing;
	struct iovec nothing_ptr;
	struct cmsghdr *cmsg;
	int i;

	nothing_ptr.iov_base = &nothing;
	nothing_ptr.iov_len = 1;
	msghdr.msg_name = NULL;
	msghdr.msg_namelen = 0;
	msghdr.msg_iov = &nothing_ptr;
	msghdr.msg_iovlen = 1;
	msghdr.msg_flags = 0;
	msghdr.msg_control = buffer;
	msghdr.msg_controllen = sizeof(struct cmsghdr) + sizeof(int) * n_fds;
	cmsg = CMSG_FIRSTHDR(&msghdr);
	cmsg->cmsg_len = msghdr.msg_controllen;
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	for (i = 0; i < n_fds; i++) {
		((int *)CMSG_DATA(cmsg))[i] = -1;
	}

	if (recvmsg(sock, &msghdr, 0) < 0) {
		return(-1);
	}
	for (i = 0; i < n_fds; i++) {
		fds[i] = ((int *)CMSG_DATA(cmsg))[i];
	}
	n_fds = (cmsg->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
	return n_fds;
}

static int ancil_recv_fd(int sock, int *fd) {
	ANCIL_FD_BUFFER(1) buffer;

	return ancil_recv_fds_with_buffer(sock, fd, 1, &buffer) == 1 ? 0 : -1;
}

int ancil_send_fds_with_buffer(int sock, const int *fds, unsigned n_fds, void *buffer) {
	struct msghdr msghdr;
	char nothing = '!';
	struct iovec nothing_ptr;
	struct cmsghdr *cmsg;
	int i;

	nothing_ptr.iov_base = &nothing;
	nothing_ptr.iov_len = 1;
	msghdr.msg_name = NULL;
	msghdr.msg_namelen = 0;
	msghdr.msg_iov = &nothing_ptr;
	msghdr.msg_iovlen = 1;
	msghdr.msg_flags = 0;
	msghdr.msg_control = buffer;
	msghdr.msg_controllen = sizeof(struct cmsghdr) + sizeof(int) * n_fds;
	cmsg = CMSG_FIRSTHDR(&msghdr);
	cmsg->cmsg_len = msghdr.msg_controllen;
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	for (i = 0; i < n_fds; i++) {
		((int *)CMSG_DATA(cmsg))[i] = fds[i];
	}
	return sendmsg(sock, &msghdr, 0) >= 0 ? 0 : -1;
}

int ancil_send_fd(int sock, int fd)
{
	ANCIL_FD_BUFFER(1) buffer;

	return ancil_send_fds_with_buffer(sock, &fd, 1, &buffer);
}

/*
 * libancillary end
 */

static int android_ctrl_sock;

static void init_ctrl_sock() {
	/* Must keep consistent with FastdVpnService */
	const char *sockname = "fastd_tun_sock";
	struct sockaddr_un addr;

	if ((android_ctrl_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		exit_errno("could not create unix domain socket");
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	addr.sun_path[0] = 0;     /* Linux's abstract unix domain socket name */
	strncpy(addr.sun_path + 1, sockname, sizeof(addr.sun_path) - 2);
	int socklen = offsetof(struct sockaddr_un, sun_path) + strlen(sockname) + 1;

	if (connect(android_ctrl_sock, (struct sockaddr*)&addr, socklen) == -1) {
		exit_errno("could not connect to Android LocalServerSocket");
	}
}

int receive_android_tunfd(void) {
	if (!android_ctrl_sock) {
		init_ctrl_sock();
	}

	int handle;
	if (ancil_recv_fd(android_ctrl_sock, &handle)) {
		exit_errno("could not receive TUN handle from Android");
	} else {
		pr_debug("received fd: %u", handle);
	}

	/* quick hack for now: sending back pid (instead of writing to pid file) */
	char pid[20];
	snprintf(pid, sizeof(pid), "%u", (unsigned)getpid());
	if (write(android_ctrl_sock, pid, strlen(pid)) != strlen(pid)) {
		pr_error_errno("send pid");
	}

	return handle;
}

bool fast_socket_android_protect(fastd_socket_t * sock) {
	if (!conf.android_tun) {
		return true;
	}

	if (!android_ctrl_sock) {
		init_ctrl_sock();
	}

	pr_debug("sending fd to protect");
	ancil_send_fd(android_ctrl_sock, sock->fd);

	char buf[20];
	if (read(android_ctrl_sock, buf, sizeof(buf)) == -1) {
		pr_error_errno("read ack");
		return false;
	}
	return true;
}
