#define LWIP_INTERNAL

//#define DEBUG
#include "Logger.h"

#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "TcpClient.h"
#include "coredecls.h"

#include <Arduino.h>

#define POLLING_INTERVAL 10 // 5 seconds

extern "C" void esp_yield();
extern "C" void esp_schedule();

TcpClient::TcpClient(tcp_pcb* tcp)
    : next(NULL), mClientTcp(tcp),
      mRxBuf(NULL), mRxBufOffset(0),
      mTxBuf(NULL), mTxBufOffset(0), mTxBufSize(0) {
 
  if (mClientTcp) {
    tcp_arg(mClientTcp, this);
    tcp_recv(mClientTcp, onRecv);
    tcp_sent(mClientTcp, onSent);
    tcp_err(mClientTcp, onErr);
    tcp_poll(mClientTcp, onPoll, POLLING_INTERVAL);
  }

  mRequestTime = millis();

  LOG("create TcpClient %p", this);
}

TcpClient::~TcpClient() {
  close();
  LOG("free TcpClient %p", this);
}

bool TcpClient::isAvailable() {
  if (mRxBuf) {
    return true;
  } else {
    return false;
  }
}

bool TcpClient::isTimeout(unsigned long max_time) {
  unsigned long elapsedTime = millis() - mRequestTime;
  return max_time <= elapsedTime;
}

err_t TcpClient::write(uint8_t* buf, size_t len) {
  mTxBuf = buf;
  mTxBufOffset = 0;
  mTxBufSize = len;
  
  do {
    writeSome();

    if (mTxBufSize <= mTxBufOffset || !isConnected()) {
      break;
    }

    esp_yield();
    
  } while (true);

  
  return ERR_OK;
}

err_t TcpClient::read(uint8_t* buf, size_t mem_size, size_t* len) {
  if (mRxBuf == NULL) {
    if (len) *len = 0;
    return ERR_OK;
  } else if (mRxBuf->tot_len <= mRxBufOffset) {
    auto head = mRxBuf;
    mRxBuf = mRxBuf->next;
    mRxBufOffset = 0;
    pbuf_ref(mRxBuf);
    pbuf_free(head);

    if (!mRxBuf) {
      return ERR_OK;
    }
  }

  // 読み込み可能なバイト数
  size_t leftBufBytes = mem_size;

  uint8_t* dst = buf;
  while (mRxBuf != NULL && leftBufBytes > 0) {
    size_t rxBufSize = mRxBuf->tot_len - mRxBufOffset;
    size_t consumeSize = min(rxBufSize, leftBufBytes);

    os_memcpy(dst, reinterpret_cast<char*>(mRxBuf->payload) + mRxBufOffset, consumeSize);
    dst += consumeSize;
    leftBufBytes -= consumeSize;
    mRxBufOffset += consumeSize;
    
    if (mRxBuf->tot_len - mRxBufOffset <= 0) {
      auto head = mRxBuf;
      mRxBuf = mRxBuf->next;
      mRxBufOffset = 0;
      pbuf_ref(mRxBuf);
      pbuf_free(head);
    }
  }
  
  size_t total_consume_bytes = mem_size - leftBufBytes;

  if (len) {
    *len = total_consume_bytes;
  }
  
  tcp_recved(mClientTcp, total_consume_bytes);

  return ERR_OK;
}


err_t TcpClient::readByte(uint8_t* b) {
  if (b) *b = 0;

  if (mRxBuf == NULL) {
    return ERR_OK;
  } else if (mRxBuf->tot_len <= mRxBufOffset) {
    auto head = mRxBuf;
    mRxBuf = mRxBuf->next;
    mRxBufOffset = 0;
    pbuf_ref(mRxBuf);
    pbuf_free(head);

    if (!mRxBuf) {
      return ERR_OK;
    }
  }

  if (b) {
    uint8_t* buf = reinterpret_cast<uint8_t*>(mRxBuf->payload);
    *b = *(buf + mRxBufOffset);
    tcp_recved(mClientTcp, 1);
    mRxBufOffset++;
  }

  return ERR_OK;
}
  
void TcpClient::close() {
  if (!mClientTcp) {
    return;
  }
  
  tcp_arg(mClientTcp, NULL);
  tcp_sent(mClientTcp, NULL);
  tcp_recv(mClientTcp, NULL);
  tcp_err(mClientTcp, NULL);
  tcp_poll(mClientTcp, NULL, 0);
  err_t err = tcp_close(mClientTcp);
  if (err != ERR_OK) {
    tcp_abort(mClientTcp);
  }

  mClientTcp = NULL;

  freeRxBuf();
}

void TcpClient::abort() {
    if (!mClientTcp) {
    return;
  }
  
  tcp_arg(mClientTcp, NULL);
  tcp_sent(mClientTcp, NULL);
  tcp_recv(mClientTcp, NULL);
  tcp_err(mClientTcp, NULL);
  tcp_poll(mClientTcp, NULL, 0);
  tcp_abort(mClientTcp);

  mClientTcp = NULL;

  freeRxBuf();
}

bool TcpClient::isConnected() {
  return mClientTcp != NULL;
}

void TcpClient::freeRxBuf() {
  while (mRxBuf) {
    auto head = mRxBuf;
    mRxBuf = mRxBuf->next;
    pbuf_free(head);
  }
}

err_t TcpClient::writeSome() {
  size_t leftSize = mTxBufSize - mTxBufOffset;
  size_t writeBytes = min(leftSize, (size_t)tcp_sndbuf(mClientTcp));

  err_t err = tcp_write(mClientTcp, mTxBuf + mTxBufOffset, writeBytes, 0);
  if (err != ERR_OK) {
    return err;
  }
  
  mTxBufOffset += writeBytes;

  return ERR_OK;
}

err_t TcpClient::onRecv(void *arg, struct tcp_pcb *tpcb, pbuf *pb, err_t err) {
  TcpClient* self = static_cast<TcpClient*>(arg);
  
  if (pb == NULL) {
    LOG("closed pcb=%p", tpcb);
    self->abort();
    return ERR_ABRT;
  }

  if (self->mRxBuf) {
    pbuf_cat(self->mRxBuf, pb);
  } else {
    self->mRxBuf = pb;
    self->mRxBufOffset = 0;
  }

  LOG("recv: bytes=%d pcb=%p", pb->tot_len, pb);
  
  return ERR_OK;
}
err_t TcpClient::onSent(void *arg, struct tcp_pcb *tpcb, uint16_t len) {
  TcpClient* self = static_cast<TcpClient*>(arg);

  if (self->mTxBuf && self->mTxBufOffset < self->mTxBufSize) {
    esp_schedule();
  }
  
  return ERR_OK;
}

void TcpClient::onErr(void *arg, err_t err) {
  TcpClient* self = static_cast<TcpClient*>(arg);
  if (err != ERR_OK) {
    self->abort();
  }
}

err_t TcpClient::onPoll(void* arg, tcp_pcb* tpcb) {
  return ERR_OK;
}
