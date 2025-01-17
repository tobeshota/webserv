#ifndef POLL_HPP
#define POLL_HPP

#include <vector>
#include <poll.h>

class Poll {
 public:
     virtual ~Poll() {}
     virtual void poll_data() = 0;
    virtual std::vector<pollfd> get_poll_fds() const = 0;
};

#endif