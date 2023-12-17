#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <vector>
#include <signal.h>
#include <sys/epoll.h>

#define BUF_SIZE 100
#define WORKER_PROCESSES 1
#define WORKER_CONNECTIONS 256
#define MAX_EVENTS 10

volatile sig_atomic_t e_flag = 0;

void	finish_server(int sig);

int	main(int ac, char *av[])
{
	int	sd_listen[WORKER_PROCESSES];
	struct sockaddr_in	serv_addr, clnt_addr;
	socklen_t	clnt_addr_size;
	
	signal(SIGINT, finish_server);

	if(ac != 3) {
		printf("Usage : %s <ip address> <port>\n", av[0]);
		exit(1);
	}

	std::cout << "if you want to finish, please push Ctrl+c" << std::endl;

	for (int i = 0; i < WORKER_PROCESSES; i++)
	{
		// sd_listen = socket(AF_INET, SOCK_STREAM, 0);
		sd_listen[i] = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
		if (sd_listen[i] == -1)
		{
			std::cout << "Error: socket:" << std::strerror(errno);
			exit (1);
		}
		
		memset(&serv_addr, 0, sizeof(struct sockaddr_in));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(av[1]);
		serv_addr.sin_port = htons(atoi(av[2]));

		// ソケット登録
		if (bind(sd_listen[i], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		{
			std::cout << "Error: bind:" << std::strerror(errno);
			close(sd_listen[i]);
			exit (1);
		}

		// 受信
		if (listen(sd_listen[i], SOMAXCONN) < 0)
		{
			std::cout << "Error listen:" << std::strerror(errno);
			close(sd_listen[i]);
			exit(1);
		}
	}


	/* epoll_waitの結果の格納先 */
	struct epoll_event  events[WORKER_CONNECTIONS];
	/* epollインスタンスを参照するファイルディスクリプタ */
	int                 epfd;
	/* ファイルディスクリプタと紐付けるイベント情報 */
	struct epoll_event  ev;

	/* epollインスタンスを作成 */
	epfd = epoll_create(WORKER_CONNECTIONS);
	if ( epfd < 0)
	{
		std::cout << "Error epoll_create:" << std::strerror(errno);
		exit(1);
	}

	for (int i = 0; i < WORKER_PROCESSES; i++)
	{
		/* listen用ソケットに紐付けるイベント情報を設定する */
		memset(&ev, 0, sizeof(struct epoll_event)); /* イベント情報の初期化 */
		ev.events = EPOLLIN | EPOLLET;    /* 入力待ち（読み込み待ち） */
		ev.data.fd = sd_listen[i];
		
		/* epollインスタンスに、sd_listenと上記のイベント情報とを追加する */
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, sd_listen[i], &ev) == -1)
		{
			std::cout << "Error epoll_ctl:" << std::strerror(errno);
			close(epfd);
			exit(1);
		}
	}

	int	msg_len = 0, recv_value, nfds;
	char msg[BUF_SIZE];
	int	accept_sock;
	fd_set	readfds, writefds;

	while (1) {
		// first phase

		// second phase
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			std::cout << "Error epoll_wait:" << std::strerror(errno);
			close(epfd);
			exit(EXIT_FAILURE);
		}

		// third phase (event phase)
		for (int i = 0; i < nfds; i++) // ここでイベントが発生しているかどうかをO(N)で見ている
		{
			if (events[i].data.fd == sd_listen[0])
			{
				memset(&clnt_addr, 0, clnt_addr_size = sizeof(clnt_addr));
				accept_sock = accept(events[i].data.fd, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
				if (accept_sock == -1) {
					std::cout << "Error accept:" << std::strerror(errno);
					close(epfd);
					exit(EXIT_FAILURE);
				}
				fcntl(accept_sock, F_SETFL, fcntl(accept_sock, F_GETFL) | O_NONBLOCK);
				memset(&ev, 0, sizeof(struct epoll_event));
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = accept_sock;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, accept_sock, &ev) == -1)
				{
					std::cout << "Error epoll_ctl:" << std::strerror(errno);
					close(epfd);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				if (events[i].events & EPOLLIN)
				{
					recv_value = recv(events[i].data.fd, msg, BUF_SIZE, MSG_DONTWAIT);
					if (recv_value > 0)
					{
						std::cout << "message receive: " << msg << std::endl;
					}
					memset(&ev, 0, sizeof(struct epoll_event));
					ev.events = EPOLLOUT | EPOLLET;
					ev.data.fd = events[i].data.fd;
					if (epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev) == -1)
					{
						std::cout << "Error epoll_ctl:" << std::strerror(errno);
						close(epfd);
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					send(events[i].data.fd, "message received\n", 18, 0);
					close(events[i].data.fd);
					if (epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1)
					{
						std::cout << "Error epoll_ctl:" << std::strerror(errno);
						close(epfd);
						exit(EXIT_FAILURE);
					}
				}
			}
		}
		if (e_flag == 1)
			break ;
	}

	close(epfd);
	std::cout << "safely ended" << std::endl;
	return 0;
}

void	finish_server(int sig)
{
	e_flag = 1;
}
