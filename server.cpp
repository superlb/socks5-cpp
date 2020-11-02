#include "server.h"

bool server::setNonBlocking(int fd)
{
    int flag = fcntl(fd,F_GETFL,0);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag) != -1;
}
bool server::addIntoEpoll(int fd,void* ptr)
{
    setNonBlocking(fd);
    struct epoll_event event;
    event.data.ptr = ptr;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLIN | EPOLLET;
    return epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event) != -1;
}
bool server::delFromEpoll(int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = 0;
    return epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&event) != -1;
}

void server::newConnect(int fd)
{
    addIntoEpoll(fd,NULL);
    Connect* con = new Connect();
    con->state = AUTH;
    con->clientfd = fd;
    fdmap[fd] = con;
}
void server::delConnect(int fd)
{
    delFromEpoll(fd);
    auto it = fdmap.find(fd);
    if(it!=fdmap.end())
    {
        Connect* con = it->second;
        if(con->clientfd==fd)
            con->clientfd = -1;
        if(con->serverfd==fd)
            con->serverfd = -1;
        if(con->clientfd==-1&&con->serverfd==-1)
        {
            delete con;
            fdmap.erase(it);
        }
    }
}

void server::eventHandle(int fd)
{
    auto it = fdmap.find(fd);
    if(it!=fdmap.end())
    {
        Connect* con = it->second;
        if(con->state==AUTH)
        {
            authHandle(fd);
        }
        else if(con->state==ESTABLISHMENT)
        {
            establishmentHandle(fd);
        }
        else if(con->state==FORWARDING)
        {
            forwardingHandle(fd);
        }
    }
}

void server::authHandle(int fd)
{
    //NO AUTH
    constexpr int BUFF_SIZE = 1024;
    uint8_t buf[BUFF_SIZE];
    int len = recv(fd,buf,BUFF_SIZE,MSG_PEEK);
    if(len < 0)
    {
        //TODO
    }
    else if(len == 0)
    {
        delConnect(fd);
        return;
    }
    else if(len < 3)
    {
        return;
    }
    else
    {
        if(buf[0]==0x05)
        {
            recv(fd,buf,BUFF_SIZE,0);
            auto it = fdmap.find(fd);
            if(it!=fdmap.end())
            {
                Connect* con = it->second;
                con->state = ESTABLISHMENT;
            }
            uint8_t reply[2] = {0x05,0x00};
            send(fd,reply,2,0);
        }
    }
}

void server::establishmentHandle(int fd)
{
    constexpr int BUFF_SIZE = 1024;
    uint8_t buf[BUFF_SIZE];
    int len = recv(fd,buf,BUFF_SIZE,MSG_PEEK);
    if(len < 0)
    {
        //TODO
    }
    else if(len == 0)
    {
        delConnect(fd);
        return;
    }
    else if(len < 10)
    {
        return;
    }
    else
    {
        if(buf[0]==0x05)
        {
            if(buf[1]==0x01)//CMD为Connect
            {
                uint8_t addtype = buf[3];
                uint8_t ip[4];
                uint8_t port[2];
                uint8_t addlen;
                if(addtype==0x01)//IPV4
                {
                    addlen=4;
                    memcpy(ip,buf+4,addlen);
                    memcpy(port,buf+4+addlen,2);
                }
                else if(addtype==0x03)//DOMAINNAME
                {
                    addlen = buf[4];
                    uint8_t domainname[256];
                    memcpy(domainname,buf+5,addlen);
                    struct hostent* hostptr = gethostbyname((char*)domainname);
                    memcpy(ip,hostptr->h_addr,hostptr->h_length);
                    memcpy(port,buf+5+addlen,2);
                    //gethostbyname似乎指向的是一个static struct，不需要free
                }
                else if(addtype==0x04)//IPV6
                {
                    
                }
                else
                {
                    //TODO
                }
                if(len < 6 + addlen)
                {
                    return;
                }
                recv(fd,buf,6 + addlen,0);

                struct sockaddr_in server_addr;
                server_addr.sin_family = AF_INET;
                memcpy(&server_addr.sin_addr.s_addr,ip,4);
                server_addr.sin_port = *((uint16_t*)port);
                int serverfd = socket(PF_INET,SOCK_STREAM,0);
                connect(serverfd,(struct sockaddr*)&server_addr,sizeof(server_addr));

                auto it = fdmap.find(fd);
                if(it!=fdmap.end())
                {
                    Connect* con = it->second;
                    con->state = FORWARDING;
                    con->serverfd = serverfd;
                    fdmap[serverfd] = con;
                }

                uint8_t reply[10];
                reply[0] = 0x05;
                reply[3] = 0x01;
                send(fd,reply,10,0);
            }
        }
    }
    
}

void server::forwardingHandle(int fd)
{
    auto it = fdmap.find(fd);
    int sendfd;
    if(it!=fdmap.end())
    {
        Connect* con = it->second;
        if(fd==con->clientfd)
        {
            sendfd = con->serverfd;
        }
        else
        {
            sendfd = con->clientfd;
        }
    }
    constexpr int BUFF_SIZE = 4096;
    uint8_t buf[BUFF_SIZE];
    int len = recv(fd,buf,BUFF_SIZE,0);
    if(len < 0)
    {
        //TODO
    }
    else if(len==0)
    {
        delConnect(fd);
        return;
    }
    else
    {
        send(sendfd,buf,len,0);
    }
    
}

void server::run()
{
    listenfd = socket(PF_INET,SOCK_STREAM,0);
    if(listenfd==-1)
    {
        std::cerr<<"create listenfd fail"<<std::endl;
        return;
    }

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listenfd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
    {
        std::cerr<<"bind fail"<<std::endl;
        return;
    }

    if(listen(listenfd,SOMAXCONN) < 0)
    {
        std::cerr<<"listen fail"<<std::endl;
        return;
    }

    epollfd = epoll_create1(0);
    if(epollfd < 0)
    {
        std::cerr<<"create epollfd fail"<<std::endl;
        return;
    }

    addIntoEpoll(listenfd,NULL);
    forever();
}

void server::forever()
{
    struct epoll_event events[MAX_EPOLL_EVENTS];
    while(1)
    {
        int n = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, 0);
        for(int i=0;i<n;++i)
        {
            if(events[i].data.fd==listenfd) 
            {
                //新连接
                struct sockaddr clientaddr;
                socklen_t len;
                int connectfd = accept(listenfd, &clientaddr, &len);
                newConnect(connectfd);
            }
            else
            {
                //已有连接
                eventHandle(events[i].data.fd);
            }
            
        }
    }
}