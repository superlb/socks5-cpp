#include <unistd.h>
#include <sstream>
#include "server.h"

int main(int argc, char **argv)
{
    int port = 1080;
    uint8_t authmethod = 0x00;
    std::string username = "root",password = "123456";
    int ch;
    while((ch=getopt(argc,argv,"ap:u:w:"))!=-1)
    {
        if(ch=='p')
        {
            std::stringstream mystream;
            mystream<<optarg<<std::endl;
            mystream>>port;
        }
        else if(ch=='a')
        {
            authmethod = 0x02;
        }
        else if(ch=='u')
        {
            username = optarg;
        }
        else if(ch=='w')
        {
            password = optarg;
        }
        else if(ch=='?')
        {
            std::cout<<"unknown option:"<<ch<<std::endl;
            std::cout<<"server ends"<<std::endl;
            return 0;
        }
    }
    server myserver(port,authmethod,username,password);
    myserver.run();
    return 0;
}