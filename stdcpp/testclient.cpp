#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>
#include <iostream>

using namespace std;
int main() {
  int status;
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(1080);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  int serverfd = socket(PF_INET, SOCK_STREAM, 0);
  connect(serverfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  return 0;
}