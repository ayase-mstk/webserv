#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>

#define BUF_SIZE 100

int	main(int ac, char *av[])
{
	if (ac != 4)
	{
		std::cout << "Usage: %s <ip address> <port> <msg>" << std::endl;
		return (0);
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		std::cout << "Error: socket:" << std::strerror(errno);
		exit (1);
	}
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(av[1]);
	addr.sin_port = htons(atoi(av[2])); // network byte order (big endian) に変換
	// inet_aton("127.0.0.1", &addr.sin_addr);

	// ソケット接続要求
	connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	// 送信
	char	*s_str = av[3];
	send(sockfd, s_str, BUF_SIZE, 0);
	std::cout << "send: " << s_str << std::endl;

	// 受信
	char r_str[BUF_SIZE];
	recv(sockfd, r_str, BUF_SIZE, 0);
	std::cout << "accept: " << r_str << std::endl;

	close(sockfd);

	return (0);
}

int sock = socket(PF_INET, SOCK_STREAM);
bind(sock, addr);
listen(sock);

allsock.add(sock);

while ( 1 ) {
    result = select(sock);
    if ( result > 0 ) {
        int new_sock = accept(sock, &addr);
        allsock.add(new_sock);
    }
    foreach ( sock = allsock ) {
        result = select(sock);
        if ( result > 0 ) {
            char buf[100];
            size_t size = read(new_sock, buf, 100);
            if ( size == 0 ) {
                close(new_sock);
            }
            else {
                write(new_sock, buf, size);
            }
        }
    }
}