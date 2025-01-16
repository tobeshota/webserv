#ifndef SERVER_DATA_HPP
#define SERVER_DATA_HPP

#include "IAddressData.hpp"
#include "IServerFD.hpp"
#include "IServerBind.hpp"
#include "IServerListen.hpp"
#include "IServerAccept.hpp"

class ServerData : public IAddressData, public IServerFD, public IServerBind, public IServerListen, public IServerAccept {
 private:
  int server_fd;
  int new_socket;
  struct sockaddr_in address;
  int addrlen;
 public:
  ServerData();
  ~ServerData();
	void set_address_data();
	void set_server_fd();
	void server_bind();
	void server_listen();
	void server_accept();
	int get_server_fd() const;
	int get_new_socket() const;
	struct sockaddr_in get_address() const;
	int get_addrlen() const;
	void set_new_socket(int new_socket);

};

#endif
