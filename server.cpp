#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex;

void * handle_clnt(void * arg);
void send_msg(char * msg, int len);

int	main(int ac, char *av[])
{
	int	serv_sock, clnt_sock;
	struct sockaddr_in	serv_addr, clnt_addr;
	socklen_t	clnt_addr_size;
	pthread_t	t_id;

	if(ac != 2) {
		printf("Usage : %s <port>\n", av[0]);
		exit(1);
	}

	pthread_mutex_init(&mutex, NULL);
	serv_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1)
	{
		std::cout << "Error: socket:" << std::strerror(errno);
		exit (1);
	}

	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(av[1]));
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

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

	while (1)
	{
		clnt_addr_size = sizeof(struct sockaddr_in);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

		pthread_mutex_lock(&mutex);
		clnt_socks[clnt_cnt++] = clnt_sock;
		pthread_mutex_unlock(&mutex);

		pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
		pthread_detach(t_id);
		printf("Connected clinet IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
	}
	close(serv_sock);
	return 0;
}

void	*handle_clnt(void *arg)
{
	int	clnt_sock = *((int *)arg);
	int	msg_len = 0;
	char msg[BUF_SIZE];

	sleep(5);

	if (clnt_sock < 0)
	{
		std::cout << "Error accept:" << std::strerror(errno);
		exit(1);
	}

	while ((msg_len = recv(clnt_sock, msg, sizeof(msg), 0)) != 0)
	{
		if (msg_len == -1)
		{
			std::cout << "Error recv:" << std::strerror(errno);
			exit(1);
		}
		send_msg(msg, msg_len);
	}
	
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < clnt_cnt; i++)
	{
		if (clnt_sock == clnt_socks[i])
		{
			while (i++ < clnt_cnt - 1)
				clnt_socks[i] = clnt_socks[i + 1];
			break ;
		}
	}

	clnt_cnt--;
	pthread_mutex_unlock(&mutex);
	close(clnt_sock);
	return (NULL);
}

void	send_msg(char *msg, int	len)
{
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < clnt_cnt; i++)
	{
		send(clnt_socks[i], msg, len, 0);
	}
	pthread_mutex_unlock(&mutex);
}
