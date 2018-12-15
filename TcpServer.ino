#define LWIP_INTERNAL

#define DEBUG
#include "Logger.h"

#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "TcpServer.h"
#include <stdarg.h>

#define TIMEOUT_NO_UNSWER 500

TcpServer::TcpServer() : mServerTcp(NULL), mListenTcp(NULL), mClients(NULL) {
}

TcpServer::~TcpServer() {
  stop();
}

err_t TcpServer::start(const ip_addr_t* addr, int port) {
  mServerTcp = tcp_new();
  if (!mServerTcp) {
    return ERR_MEM;
  }

  mServerTcp->so_options |= SOF_REUSEADDR;

  err_t err = tcp_bind(mServerTcp, addr, port);
  if (err != ERR_OK) {
    stop();
    return err;
  }

  tcp_pcb* listen_pcb = tcp_listen(mServerTcp);
  if (!listen_pcb) {
    stop();
    return ERR_VAL;
  }

  mListenTcp = listen_pcb;

  tcp_accept(mListenTcp, onAccept);
  tcp_arg(mListenTcp, this);

  return ERR_OK;
}

void TcpServer::stop() {
  while (mClients) {
    TcpClient* c = mClients;    
    if (c) {
      c->abort();
      mClients = c->next;
      delete c;
    } else {
      mClients = NULL;
    }
  }
  
  if (mListenTcp) {
    tcp_close(mListenTcp);
    mListenTcp = NULL;
  }

  if (mServerTcp) {
    tcp_close(mServerTcp);
    mServerTcp = NULL;
  }
}

TcpClient* TcpServer::available() {
  if (mClients == NULL) {
    return NULL;
  }

  TcpClient* activeClient = NULL;
  TcpClient* pre = NULL;
  TcpClient* top = mClients;
  while (top) {
    if (top->isAvailable()) {
      activeClient = top;

      if (activeClient == mClients) {
        mClients = mClients->next;
      } else if (pre) {
        pre->next = activeClient->next;
      }
      break;
    }

    pre = top;
    top = top->next;
  }

  LOGIF(activeClient, "client will be available: client=%p", activeClient);
  return activeClient;
}

void TcpServer::trim() {
  if (mClients && mClients->isTimeout(TIMEOUT_NO_UNSWER)) {
    TcpClient* c = mClients;
    mClients = mClients->next;

    c->abort();
    LOG("trim client: client=%p", c);
    delete c;
  }
}

long TcpServer::onAccept(void *arg, tcp_pcb* newpcb, long err) {
  TcpServer* self = static_cast<TcpServer*>(arg);

  int numClients = 1;
  
  if (err == ERR_OK) {
    if (newpcb) {
      // TcpClientの作成
      TcpClient* c = new TcpClient(newpcb);
      if (c) {
        if (!self->mClients) {
          // キューが空。キューの先頭に追加する
          self->mClients = c;
        } else {
          // キューの末尾に追加
          TcpClient* tail = self->mClients;
          while (tail->next) {
            tail = tail->next;
            numClients++;
          }
          tail->next = c;
          numClients++;
        }

        LOG("accepted client=%p, num=%d", c, numClients);
  
      } else {
        // TcpClientの作成に失敗した場合
        tcp_abort(newpcb); // RSTを送信
      }
    }
  } else if (newpcb) {
    // accept失敗したがpcbを取得できた場合 (起きないと思うけど・・・)
    tcp_abort(newpcb); // RSTを送信
  }
  
  return 0;
}
