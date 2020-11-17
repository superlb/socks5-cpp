#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

#include "threadpool.h"


class server {
 private:
  enum Socks5State {
    AUTH,           // 未授权
    ESTABLISHMENT,  // 建立连接
    FORWARDING,     // 转发
  };
  struct Connect {
    Socks5State state;
    int clientfd;
    int serverfd;
    Connect() : state(AUTH), clientfd(-1), serverfd(-1) {}
  };
  static constexpr int MAX_EPOLL_EVENTS = 1024;
  ThreadPool threadpool;
  int listenfd;
  int epollfd;
  int port;
  std::string username, password;
  uint8_t authmethod;
  std::unordered_map<int, Connect*> fdmap;
  bool setNonBlocking(int fd);
  bool addIntoEpoll(int fd, void* ptr);
  bool delFromEpoll(int fd);
  void forever();
  void newConnect(int fd);
  void delConnect(int fd);
  void eventHandle(int fd);
  void authHandle(int fd);
  void establishmentHandle(int fd);
  void forwardingHandle(int fd);

 public:
  server(int port, uint8_t authmethod, std::string username,
         std::string password)
      : listenfd(-1),
        epollfd(-1),
        port(port),
        authmethod(authmethod),
        username(username),
        password(password) {}
  ~server() {}
  void run();
};