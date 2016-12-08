#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

char *buffer;
char r_buffer[1024];

int main(int argc, char *argv[])
{
	int fd_sock, cli_sock;
	int port_num, ret;
	struct sockaddr_in addr;
	size_t len;

	// arg parsing
	if (argc != 3) {
		printf("usage: cli srv_ip_addr port\n");
		return 0;
	}
	port_num = atoi(argv[2]);

	// socket creation
	fd_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_sock == -1) {
		perror("socket");
		return 0;
	}

	// addr binding, and connect
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port_num);
	inet_pton(AF_INET, argv[1], &addr.sin_addr);

	ret = connect(fd_sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1) {
		perror("connect");
		close(fd_sock);
		return 0;
	}

	while (1) {
		buffer = NULL;
		printf("send$ ");
		ret = getline(&buffer, &len, stdin);
		if (ret == -1) { // EOF
			perror("getline");
			close(fd_sock);
			break;
		}
		write(fd_sock, buffer, len);

		memset(r_buffer, 0, sizeof(r_buffer));
		len = read(fd_sock, r_buffer, sizeof(r_buffer));
		printf("server$ %s\n", r_buffer);
		if (len <= 0) { // EOF
			perror("read");
			close(fd_sock);
			break;
		}
	}
	return 0;
}

