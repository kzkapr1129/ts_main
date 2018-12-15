#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

class TcpClient {
public:
  TcpClient* next;

  TcpClient(tcp_pcb* tcp);
  ~TcpClient();

  bool isAvailable();

  err_t write(uint8_t* buf, size_t len);
  err_t read(uint8_t* buf, size_t buf_size, size_t* len);
  err_t readByte(uint8_t* b);

  void close();
  void abort();

  bool isConnected();
  bool isTimeout(unsigned long max_time);

private:
  void freeRxBuf();
  err_t writeSome();

  static err_t onRecv(void *arg, struct tcp_pcb *tpcb, struct pbuf *pb, err_t err);
  static err_t onSent(void *arg, struct tcp_pcb *tpcb, uint16_t len);
  static void onErr(void *arg, err_t err);
  static err_t onPoll(void* arg, tcp_pcb* tpcb);

  tcp_pcb* mClientTcp;

  pbuf* mRxBuf;
  size_t mRxBufOffset;

  uint8_t* mTxBuf;
  size_t mTxBufOffset;
  size_t mTxBufSize;
  unsigned long mRequestTime;
};

#endif
