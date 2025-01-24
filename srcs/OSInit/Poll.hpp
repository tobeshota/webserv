#ifndef POLL_HPP
#define POLL_HPP

#include <poll.h>

#include <vector>

class Poll {
 protected:
  std::vector<pollfd> poll_fds;

 public:
  virtual ~Poll() {}
  virtual std::vector<pollfd> get_poll_fds() const;
};

#endif
