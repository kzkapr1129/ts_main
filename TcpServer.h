#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "TcpClient.h"

class TcpServer {
public:
  TcpServer();
  ~TcpServer();
  
  err_t start(const ip_addr_t* addr, int port);
  void stop();

  TcpClient* available();
  void trim();

private:
  static long onAccept(void *arg, tcp_pcb* newpcb, long err);

  tcp_pcb* mServerTcp;
  tcp_pcb* mListenTcp;
  TcpClient* mClients;
};

#endif
