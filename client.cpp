#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>

int	main()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		std::cout << "Error: socket:" << std::strerror(errno);
		exit (1);
	}
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234); // network byte order (big endian) に変換
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	// inet_aton("127.0.0.1", &addr.sin_addr);

	// ソケット接続要求
	connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	// 送信
	char	s_str[] = "Hello World!";
	send(sockfd, s_str, 12, 0);
	std::cout << "send: " << s_str << std::endl;

	// 受信
	char r_str[12];
	recv(sockfd, r_str, 12, 0);
	std::cout << "accept: " << r_str << std::endl;

	close(sockfd);

	return (0);
}