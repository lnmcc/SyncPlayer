/*
 * socket.h
 *
 *  Created: Oct 10, 2013
 *  Author : sijiewang
 *  Email  : lnmcc@hotmail.com
 *  Site   : lnmcc.net
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include "../threadLib/thread.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>

namespace syncplayer {

#define MAX_SOCKET_BUF 1024

class Context;

class Socket : public Thread {

public:
	Socket(Context *context, uint16_t port = 8700);
	~Socket();

	void run();

private:
	bool listenEvent();
	void buildJson();

private:
	Context *m_context;

	uint16_t m_port;
	int m_sockfd;
	struct sockaddr_in m_servAddr;
	struct sockaddr_in m_remoteAddr;

	char m_recvBuf[MAX_SOCKET_BUF];
	std::string m_jsonBuf;
};


} //namespace

#endif /* SOCKET_H_ */
