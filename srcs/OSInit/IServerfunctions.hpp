#ifndef ISERVER_FUNCTIONS_HPP
#define ISERVER_FUNCTIONS_HPP

class IServerFunctions {
 public:
    virtual ~IServerFunctions() {}
    virtual void server_bind() = 0;
    virtual void server_listen() = 0;
    virtual void server_accept() = 0;
    virtual void set_address_data() = 0;
    virtual void set_server_fd() = 0;
    virtual int get_server_fd() const = 0;
    virtual int get_new_socket() const = 0;
    virtual struct sockaddr_in get_address() const = 0;
    virtual int get_addrlen() const = 0;
    virtual void set_new_socket(int new_socket) = 0;

};

#endif
