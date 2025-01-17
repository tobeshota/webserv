#ifndef SERVER_DATA_HPP
#define SERVER_DATA_HPP

#include "IServerFunctions.hpp"
#include "IPollData.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>

class ServerData : public IPollData, public IServerFunctions {
 private:
  int server_fd;
  int new_socket;
  struct sockaddr_in address;
  int addrlen;
  std::vector<pollfd> poll_fds;
 public:
  ServerData();
  ~ServerData();
	void set_address_data();
	void set_server_fd();
	void server_bind();
	void server_listen();
	void server_accept();
	void poll_data();
	int get_server_fd() const;
	int get_new_socket() const;
	struct sockaddr_in get_address() const;
	int get_addrlen() const;
	void set_new_socket(int new_socket);
	std::vector<pollfd> get_poll_fds() const;

};

#endif
