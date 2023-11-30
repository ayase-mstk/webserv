#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>

#define BUF_SIZE 100
#define WORKER_CONNECTIONS 256

int clnt_cnt=0;
int clnt_socks[WORKER_CONNECTIONS];

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);

int	main(int ac, char *av[])
{
	int	serv_sock, clnt_sock;
	struct sockaddr_in	serv_addr, clnt_addr;
	socklen_t	clnt_addr_size;
	fd_set	readfds;

	if(ac != 3) {
		printf("Usage : %s <ip address> <port>\n", av[0]);
		exit(1);
	}

	serv_sock = socket(AF_INET, SOCK_STREAM, 0);
	// serv_sock = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
	if (serv_sock == -1)
	{
		std::cout << "Error: socket:" << std::strerror(errno);
		exit (1);
	}


	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(av[1]);
	serv_addr.sin_port = htons(atoi(av[2]));

	// ソケット登録
	if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		std::cout << "Error: bind:" << std::strerror(errno);
		close(serv_sock);
		exit (1);
	}

	// 受信
	if (listen(serv_sock, SOMAXCONN) < 0)
	{
		std::cout << "Error listen:" << std::strerror(errno);
		close(serv_sock);
		exit(1);
	}

	FD_ZERO(&readfds);

	FD_SET(serv_sock, &readfds);


	int	msg_len = 0;
	char msg[BUF_SIZE];

	clnt_sock = select(serv_sock + 1, &readfds, NULL, NULL, 0);
	if (FD_ISSET(serv_sock, &readfds))
	{
		memset(&clnt_addr, 0, clnt_addr_size = sizeof(clnt_addr));
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		recv(clnt_sock, msg, BUF_SIZE, 0);
		std::cout << "message receive: " << msg << std::endl;
		send(clnt_sock, "message received\n", 18, 0);
	}

	// send(serv_sock, )
	
	close(serv_sock);
	close(clnt_sock);
	return 0;
}
