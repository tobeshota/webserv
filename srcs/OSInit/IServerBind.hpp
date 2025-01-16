#ifndef ISERVER_BIND_HPP
#define ISERVER_BIND_HPP

class IServerBind {
 public:
  virtual ~IServerBind() {}
  virtual void server_bind() = 0;
};

#endif
