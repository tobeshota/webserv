#include "OSInit.hpp"
#include "ServerData.hpp"
#include "Poll.hpp"

#pragma once


std::vector<pollfd> Poll::get_poll_fds() const {
    return poll_fds;
}
