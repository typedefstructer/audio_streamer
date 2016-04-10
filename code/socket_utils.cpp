void InitSocket()
{
	WSADATA wd;
	WSAStartup(MAKEWORD(2, 2), &wd);
}

int socket_listen(char *ip, char *port)
{
	addrinfo *server = 0, server_hints = {};
	server_hints.ai_family = AF_INET;
	server_hints.ai_socktype = SOCK_STREAM;
	server_hints.ai_protocol = IPPROTO_TCP;
	server_hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, port, &server_hints, &server);

	int server_socket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	bind(server_socket, server->ai_addr, (int)server->ai_addrlen);
	listen(server_socket, 16);

	return server_socket;
}
