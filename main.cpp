#include "server.h"
int main()
{
    server myserver(1080);
    myserver.run();
    return 0;
}