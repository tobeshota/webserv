#ifndef ISERVER_ACCEPT_HPP
#define ISERVER_ACCEPT_HPP

class IServerAccept {
 public:
  virtual ~IServerAccept() {}
  virtual void server_accept() = 0;
};

#endif
