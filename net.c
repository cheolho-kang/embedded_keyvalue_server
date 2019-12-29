#include "net.h"
int tcpConnect(int port) {
  int server_fd;
  char ip[IP_STR_LEN];

  struct sockaddr_in server_addr, client_addr;
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {  // 소켓 생성
    chkangLog(LOG_ERROR, "Can't open stream socket.");
    exit(-1);
  }
  memset(&server_addr, 0x00, sizeof(server_addr));
  // server_Addr 을 NULL로 초기화

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);
  // server_addr 셋팅

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {  // bind() 호출
    chkangLog(LOG_ERROR, "Can't bind local address.");
    exit(-1);
  }
  if (listen(server_fd, 5) < 0) {  //소켓을 수동 대기모드로 설정
    chkangLog(LOG_ERROR, "Can't listening connect.");
    exit(-1);
  }
  //   int flags;
  //   flags |= O_NONBLOCK;
  //   if (fcntl(server_fd, F_SETFL, flags) == -1) {
  //     chkangLog(LOG_ERROR, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
  //     exit(-1);
  //   }
  inet_ntop(AF_INET, &server_addr.sin_addr.s_addr, ip, sizeof(ip));
  chkangLog(LOG_NOTICE, "Running %s:%d", ip, port);
  return server_fd;
}

int tcpAccept(int server_fd) {
  // int tcpAccept(void *arg) {
  // int server_fd = (int)arg;
  struct sockaddr_in client_addr;
  char ip[IP_STR_LEN];
  int port;
  int len = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
  if (client_fd == -1) {
    if (errno == EINTR)
      return 0;
    else {
      chkangLog(LOG_ERROR, "Accept failed: %s", strerror(errno));
      exit(-1);
    }
  }
  port = ntohs(client_addr.sin_port);
  inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip, sizeof(ip));
  chkangLog(LOG_NOTICE, "Accepted %s:%d", ip, port);
  return client_fd;
}

void tcpRecv(int cfd) {
  int rst;
  char msg[128];
  rst = read(cfd, msg, sizeof(msg));
  if (rst <= 0)
    chkangLog(LOG_ERROR, "[%d] recv error", cfd);
  else  //어떤 아이피로부터 무슨 내용이 왔는지 출력
        // chkangLog(LOG_NOTICE, "[%d] %s\n", cfd, msg);
    printf("[%d] %s\n", cfd, msg);
  // send와 같은 write를 사용하여 보낸 내용 되돌려줌
  // write(cfd, msg, strlen(msg) + 1);
}

void tcpSend(int cfd, char *msg, int msg_len) {
  int rst;
  rst = write(cfd, msg, msg_len);
  if (rst <= 0)
    chkangLog(LOG_ERROR, "[%d] send error", cfd);
  else  //어떤 아이피로부터 무슨 내용이 왔는지 출력
        // chkangLog(LOG_NOTICE, "[%d] %s\n", cfd, msg);
    printf("[%d] %s\n", cfd, msg);
  // send와 같은 write를 사용하여 보낸 내용 되돌려줌
  // write(cfd, msg, strlen(msg) + 1);
}