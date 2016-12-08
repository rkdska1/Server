#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

char buffer[1024];

int main(int argc, char *argv[])
{
	int fd_sock, cli_sock;
	int port_num, ret;
	struct sockaddr_in addr;
	ssize_t len;

	// arg parsing
	if (argc != 2) {
		printf("usage: srv port\n");
		return 0;
	}
	port_num = atoi(argv[1]);

	// socket creation
	fd_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_sock == -1) {
		perror("socket");
		return 0;
	}

	// addr binding
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons (INADDR_ANY);
	addr.sin_port = htons (port_num);
	ret = bind (fd_sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		close(fd_sock);
		return 0;
	}

	ret = listen(fd_sock, 0);
	if (ret == -1) {
		perror("listen");
		close(fd_sock);
		return 0;
	}

	cli_sock = accept(fd_sock, (struct sockaddr *)NULL, NULL);
	if (cli_sock == -1) {
		perror("accept");
		close(fd_sock);
		return 0;
	}

	while (1) {
		memset(buffer, 0, sizeof(buffer));
		len = read(cli_sock, buffer, sizeof(buffer));
		if (len <= 0) { // EOF
			perror("read");
			close(cli_sock);
			break;
		}
		write(cli_sock, buffer, len);
	}
	close(fd_sock);
	return 0;
}

