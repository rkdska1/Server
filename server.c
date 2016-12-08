#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#define SERVER_STRING "Server: simple-http-server\r\n"


//modified get_line 

static void put_file_data(int clientfd, FILE * file)
{
	char buf[1024];

	fgets(buf, sizeof(buf), file);
	while (!feof(file))
	{
		send(clientfd, buf, strlen(buf), 0);
		fgets(buf, sizeof(buf), file);
	}
}


static void put_headers(int clientfd){
	
	char buf[1024];

	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(clientfd, buf, strlen(buf), 0);
	strcpy(buf, SERVER_STRING);
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(clientfd, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(clientfd, buf, strlen(buf), 0);
}

 
static void put_unimplemented(int clientfd){
	
	char buf[1024];

	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, SERVER_STRING);
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(clientfd, buf, strlen(buf), 0);
}

static void put_not_found(int clientfd){
	
	char buf[1024];

	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, SERVER_STRING);
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "your request because the resource specified\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "is unavailable or nonexistent.\r\n");
	send(clientfd, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(clientfd, buf, strlen(buf), 0);
}


static int get_line(int sockfd, char *buf, int size){
	
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sockfd, &c, 1, 0);
		if(n > 0)
		{
			if(c == '\r')
			{
				n = recv(sockfd, &c, 1, MSG_PEEK);
				if((n > 0) && (c == '\n')) recv(sockfd, &c, 1, 0);
				else c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
		{
			c = '\n';
		}
	}
	buf[i] = '\0';

	return i;
}

static void serve_file(int clientfd, const char *filename){
	
	FILE *resource = fopen(filename, "r");
	if(resource == NULL)
	{
		put_not_found(clientfd);
	}
	else
	{
		put_headers(clientfd);
		put_file_data(clientfd, resource);
	}
	fclose(resource);
}

static void accept_request(void *arg){
	
	int clientfd = *(int *)arg;

	char buf[1024];
	int nchars = get_line(clientfd, buf, sizeof(buf));

	fprintf(stdout, "%s", buf);

	
	size_t i = 0;
	size_t j = 0;
	char method[255];
	while (!isspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++;
		j++;
	}
	method[i] = '\0';

	if(strcasecmp(method, "GET"))
	{
		put_unimplemented(clientfd);
		return;
	}


	i = 0;
	while (isspace(buf[j]) && (j < sizeof(buf)))
		j++;

	char url[255];
	while (!isspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
	{
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = '\0';

	char path[512];
	sprintf(path, "htdocs%s", url);

	
	if(path[strlen(path) - 1] == '/') strcat(path, "index.html");

	struct stat st;
	if(stat(path, &st) == -1)
	{
		while ((nchars > 0) && strcmp("\n", buf)) 
		{
			nchars = get_line(clientfd, buf, sizeof(buf));
			fprintf(stdout, "%s", buf);
		}

		put_not_found(clientfd);
	}
	else
	{
		if((st.st_mode & S_IFMT) == S_IFDIR)
			strcat(path, "/index.html");

		while ((nchars > 0) && strcmp("\n", buf))	
		{
			nchars = get_line(clientfd, buf, sizeof(buf));
			fprintf(stdout, "%s", buf);
		}

		serve_file(clientfd, path);
	}

	fprintf(stdout, "\n");
	close(clientfd);
}

static void panic(const char *sc){
	
	perror(sc);
	exit(1);
}

static int run_server(u_short *port){
	
	int sock = 0;
	struct sockaddr_in name;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
	{
		panic("socket");
	}

	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);

	int val = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof val) < 0)
	{
		close(sock);
		panic("setsockopt");
		return -1;
	}

	if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0)
	{
		panic("bind");
	}

	if(listen(sock, 5) < 0)
	{
		panic("listen");
	}

	return sock;
}

static void print_usage(){
	
	printf("usage: http-server [PORT-NUM]\n");
}


int main(int argc, char** argv){
	
	
	if(argc != 2) 
	{
		print_usage();
		return 0;
	}

	signal(SIGPIPE, SIG_IGN);

	u_short port = atoi(argv[1]);
	int server_sock = run_server(&port);
	printf("httpd running on port %d\n", port);


	while (1){
		
		struct sockaddr_in sockaddr;
		socklen_t client_name_len = sizeof(sockaddr);
		int client_sock = accept(server_sock, (struct sockaddr *)&sockaddr, &client_name_len);
		if(client_sock == -1)
		{
			panic("accept");
		}

		printf(">>New Client Has Requested!!\n");

		pthread_t newthread;
		if(pthread_create(&newthread, NULL, (void *)accept_request, (void *)&client_sock) != 0)
		{
			perror("pthread_create");
		}
	}

	close(server_sock);
	return (0);
}

