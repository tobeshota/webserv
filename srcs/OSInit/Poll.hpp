#ifndef POLL_HPP
#define POLL_HPP

#include <vector>
#include <poll.h>

class Poll {
protected:
    std::vector<pollfd> poll_fds;
public:
     virtual ~Poll() {}
    virtual std::vector<pollfd> get_poll_fds() const;
};

#endif