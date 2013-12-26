/*
 * socket.cpp
 *
 *  Created: Oct 10, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#include "socket.h"
#include "../eventLib/event.h"

#include <strings.h>
#include <iostream>
#include <unistd.h>
#include <netinet/tcp.h>

namespace syncplayer {

#define MAX_CONNECTIONS 10

Socket::Socket(Context *contex, uint16_t port) {

	m_port = port;
	m_context = contex;
}

Socket::~Socket() {

}

bool Socket::listenEvent() {

	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (m_sockfd == -1) {
		std::cerr << __FILE__ << ":" << __LINE__ << ": create socket error"
				<< std::endl;
		return false;
	}
	bzero(&m_servAddr, sizeof(m_servAddr));
	m_servAddr.sin_family = AF_INET;
	m_servAddr.sin_port = htons(m_port);
	m_servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int opt = TCP_NODELAY;
	if (setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int))
			== -1) {
		std::cerr << __FILE__ << ":" << __LINE__ << ": setsockopt error"
				<< std::endl;
		close(m_sockfd);
		return false;
	}

	if (bind(m_sockfd, (struct sockaddr*) &m_servAddr, sizeof(struct sockaddr))
			== -1) {

		std::cerr << __FILE__ << ":" << __LINE__ << ": socket bind error"
				<< std::endl;
		close(m_sockfd);
		return false;
	}

	if (listen(m_sockfd, MAX_CONNECTIONS) == -1) {
		std::cerr << __FILE__ << ":" << __LINE__ << ": socket listen error"
				<< std::endl;
		close(m_sockfd);
		return false;
	}

	std::cerr << __FILE__ << ":" << __LINE__ << ": listenning event ... ..."
			<< std::endl;

	int connfd;
	sockaddr_in cliaddr;
	size_t sin_size = sizeof(struct sockaddr_in);

	//FIXME:这里可以记一下客户端IP
	while (true) {

		if ((connfd = accept(m_sockfd, (struct sockaddr*) &cliaddr, &sin_size))
				== -1) {

			std::cerr << __FILE__ << ":" << __LINE__ << ": connect failed"
					<< std::endl;
			continue;
		}

		std::cerr << __FILE__ << ":" << __LINE__ << "receive a connection from "
				<< inet_ntoa(cliaddr.sin_addr) << std::endl;


		while (true) {

			bzero(m_recvBuf, MAX_SOCKET_BUF);
			//FIX:如果命令行过长，是需要多次recv的
			ssize_t recvLen = recv(connfd, m_recvBuf, MAX_SOCKET_BUF - 1, 0);
			if (recvLen == -1) {
				std::cerr << __FILE__ << ":" << __LINE__ << ": receive data error"
						<< std::endl;
				//关闭本次链接
				break;
			} else if (recvLen == 0) {
				continue;
			}

			if (strncasecmp(m_recvBuf, "#End", 4) == 0) {
				//不用担心这个string的内存释放，parseString2Event会处理的
				std::string *jsonStr = new std::string(m_jsonBuf);
				m_jsonBuf.clear();
				std::cerr << "jsonStr: " << *jsonStr << std::endl;
				m_context->getEventParse()->parseString2Event(jsonStr);

			} else {
				m_jsonBuf.append(m_recvBuf);
			}
		} //end while inner
		close(connfd);
	}
	std::cerr << __FILE__<< __LINE__ << ":" << "Socket closed" << std::endl;
	close(m_sockfd);
	return false;
}

void Socket::run() {

	listenEvent();
}

} //namespace

