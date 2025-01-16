#ifndef ISERVER_LISTEN_HPP
#define ISERVER_LISTEN_HPP

class IServerListen {
 public:
  virtual ~IServerListen() {}
  virtual void server_listen() = 0;
};

#endif
