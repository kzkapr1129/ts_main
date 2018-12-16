#ifndef __HTTPFILE_H__
#define __HTTPFILE_H__

#include <SD.h>

class TcpClient;
class HttpFile {
public:
  static bool initSDcard(int chip_select_pin);

  HttpFile();
  ~HttpFile();

  bool open(const char* path);
  void close();

  int send(TcpClient* client);

private:
  File mFile;
};

#endif
