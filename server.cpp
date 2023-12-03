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
#include <vector>
#include <signal.h>

#define BUF_SIZE 5
#define WORKER_CONNECTIONS 256

volatile sig_atomic_t e_flag = 0;

struct sock_info {
	int		sock;
	bool	is_listen;
	bool	received;
};

struct socket_array {
	size_t				num;
	std::vector<struct sock_info>	sockets;
	int					fd_max;
};

void	finish_server(int sig);

int	main(int ac, char *av[])
{
	sock_info	serv_sock;
	struct sockaddr_in	serv_addr, clnt_addr;
	socklen_t	clnt_addr_size;
	struct socket_array	all_socks;
	
	all_socks.fd_max = -1;
	all_socks.num = 0;
	signal(SIGINT, finish_server);

	if(ac != 3) {
		std::cout << "[[ Usage : " << av[0] << " <ip address> <port> ]]" << std::endl;
		exit(1);
	}

	std::cout << "[[ if you want to finish, please push Ctrl+c ]]" << std::endl << std::endl;

	// serv_sock.sock = socket(AF_INET, SOCK_STREAM, 0);
	serv_sock.sock = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
	if (serv_sock.sock == -1)
	{
		std::cout << "Error: socket:" << std::strerror(errno);
		exit (1);
	}
	
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(av[1]);
	serv_addr.sin_port = htons(atoi(av[2]));

	// ソケット登録
	if (bind(serv_sock.sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
	{
		std::cout << "Error: bind:" << std::strerror(errno);
		close(serv_sock.sock);
		exit (1);
	}

	// 受信
	if (listen(serv_sock.sock, SOMAXCONN) < 0)
	{
		std::cout << "Error listen:" << std::strerror(errno);
		close(serv_sock.sock);
		exit(1);
	}

	serv_sock.received = false;
	serv_sock.is_listen = true;
	all_socks.sockets.push_back(serv_sock);
	all_socks.fd_max = std::max(all_socks.fd_max, serv_sock.sock);
	all_socks.num++;


	int	msg_len = 0, recv_value, flag;
	char msg[BUF_SIZE];
	sock_info	new_sock;
	fd_set	readfds, writefds;
	while (1) {
		// first phase
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		for (int i = 0; i < all_socks.num; i++)
		{
			if (!all_socks.sockets[i].is_listen && all_socks.sockets[i].received)
			{
				FD_SET(all_socks.sockets[i].sock, &writefds);
			}
			else
			{
				FD_SET(all_socks.sockets[i].sock, &readfds);
			}
		}

		// second phase
		select(all_socks.fd_max + 1, &readfds, &writefds, NULL, 0);

		// third phase (event phase)
		for (int i = 0; i < all_socks.num; i++) // ここでイベントが発生しているかどうかをO(N)で見ている
		{
			if (all_socks.sockets[i].is_listen)
			{
				if (FD_ISSET(all_socks.sockets[i].sock, &readfds))
				{
					FD_CLR(all_socks.sockets[i].sock, &readfds);
					memset(&clnt_addr, 0, clnt_addr_size = sizeof(clnt_addr));
					new_sock.sock = accept(all_socks.sockets[i].sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
					if (new_sock.sock > 0)
					{
						new_sock.is_listen = false;
						new_sock.received = false;
						all_socks.sockets.push_back(new_sock);
						all_socks.fd_max = std::max(all_socks.fd_max, new_sock.sock);
						all_socks.num++;
					}
				}
			}
			else if (!all_socks.sockets[i].received)
			{
				if (FD_ISSET(all_socks.sockets[i].sock, &readfds))
				{
					FD_CLR(all_socks.sockets[i].sock, &readfds);
					recv_value = recv(all_socks.sockets[i].sock, msg, BUF_SIZE, MSG_DONTWAIT);
					if (recv_value > 0)
					{
						std::cout << "message receive: " << msg << std::endl;
					}
					if (recv_value == 0)
						all_socks.sockets[i].received = true;
				}
			}
			else
			{
				if (FD_ISSET(all_socks.sockets[i].sock, &writefds))
				{
					FD_CLR(all_socks.sockets[i].sock, &writefds);
					send(all_socks.sockets[i].sock, "message received\n", 18, 0);
					close(all_socks.sockets[i].sock);
					all_socks.sockets.erase(all_socks.sockets.begin() + i);
					all_socks.num--;
					for (int j = 0; j < all_socks.num; j++)
					{
						all_socks.fd_max = std::max(all_socks.fd_max, all_socks.sockets[j].sock);
					}
				}
			}
		}
		if (e_flag == 1)
			break ;
	}

	for (int i = 0; i < all_socks.num; i++)
		close(all_socks.sockets[i].sock);
	std::cout << std::endl << "[[ safely ended ]]" << std::endl;
	return 0;
}

void	finish_server(int sig)
{
	e_flag = 1;
}
