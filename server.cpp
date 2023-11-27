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

	// ソケット登録
	bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

	// 受信
	if (listen(sockfd, SOMAXCONN) < 0)
	{
		std::cout << "Error listen:" << std::strerror(errno);
		close(sockfd);
		exit(1);
	}

	struct sockaddr_in	get_addr;
	socklen_t	len = sizeof(struct sockaddr_in);
	int	connect = accept(sockfd, (struct sockaddr *)&get_addr, &len);

	if (connect < 0)
	{
		std::cout << "Error accept:" << std::strerror(errno);
		exit(1);
	}

	// 受信
	char str[12];
	recv (connect, str, 12, 0);
	std::cout << "accept: " << str << std::endl;

	// 再送信
	send(connect, str, 12, 0);
	std::cout << "send: " << str << std::endl;

	close(connect);
	close(sockfd);

	return (0);	
}