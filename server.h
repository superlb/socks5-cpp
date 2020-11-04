#include <unordered_map>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <netinet/in.h>   
#include <arpa/inet.h>
#include <unistd.h>
using std::cout;
using std::endl;
using std::string;

class server
{
private:
    enum Socks5State
    {
        AUTH,           // 未授权
        ESTABLISHMENT,  // 建立连接
        FORWARDING,     // 转发
    };
    struct Connect
    {
        Socks5State state;
        int clientfd;
        int serverfd;
        Connect():state(AUTH),clientfd(-1),serverfd(-1){}
    };
    static constexpr int MAX_EPOLL_EVENTS = 1024;
    int listenfd;
    int epollfd;
    int port;
    string username,password;
    uint8_t authmethod;
    std::unordered_map<int,Connect*> fdmap;
    bool setNonBlocking(int fd);
    bool addIntoEpoll(int fd,void* ptr);
    bool delFromEpoll(int fd);
    void forever();
    void newConnect(int fd);
    void delConnect(int fd);
    void eventHandle(int fd);
    void authHandle(int fd);
    void establishmentHandle(int fd);
    void forwardingHandle(int fd);
public:
    server(int port,uint8_t authmethod,string username,string password)
        :listenfd(-1),epollfd(-1),port(port),authmethod(authmethod),username(username),password(password) {}
    ~server(){}
    void run();
};