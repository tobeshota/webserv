# webserv
**Webserv is a HTTP/1.1 server in C++98📡**

---

# Description
<p align="center">
 <img width="480" alt="demo" src="https://github.com/user-attachments/assets/a7766069-205a-479e-8f9b-b20359da822c">
</p>

We implemented the main features of _nginx_.  
It is to learn about the functionality and architecture of HTTP1.1 servers.

For more:
* [Requirements Specification](https://github.com/tobeshota/webserv/wiki/Requirements-Specification)
* [Corresponding setting item](https://github.com/tobeshota/webserv/wiki/Corresponding-setting-item)
* [Architecture diagram](https://github.com/tobeshota/webserv/wiki/Architecture-diagram)
* [Steps to create an HTTP 1.1 server](https://github.com/tobeshota/webserv/wiki/Steps-to-create-an-HTTP-1.1-server)

# Usage
### Requirements
- [ ] `docker` (∵ the execution environment is a Docker container)
```shell
brew install --cask docker
```

### Installation

```shell
# Clone this repository
git clone https://github.com/tobeshota/webserv
# Change directory to this repository
cd webserv
# Launch the Docker container
make run
# Compile to create an executable file in the Docker container: webserv
make
# Execute in the Docker container: ./webserv [configuration file]
./webserv ./conf/webserv.conf
```
