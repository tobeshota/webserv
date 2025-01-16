#ifndef ISERVER_FD_HPP
#define ISERVER_FD_HPP

class IServerFD {
 public:
  virtual ~IServerFD() {}
  virtual void set_server_fd() = 0;
};

#endif
