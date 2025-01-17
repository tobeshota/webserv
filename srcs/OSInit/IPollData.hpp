#ifndef IPOLL_DATA_HPP
#define IPOLL_DATA_HPP

#include <vector>
#include <poll.h>

class IPollData {
 public:
     virtual ~IPollData() {}
     virtual void poll_data() = 0;
    virtual std::vector<pollfd> get_poll_fds() const = 0;
};

#endif
