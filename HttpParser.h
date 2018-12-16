#ifndef __HTTPPARSER_H__
#define __HTTPPARSER_H__

class TcpClient;
class HttpParser {
public:
  enum Method {
    GET,
    UNKNOWN
  };

  HttpParser(TcpClient* client);
  ~HttpParser();

  Method method() const;
  const char* path() const;

private:
  err_t init();
  err_t readDinamicWord(uint8_t*& dst, int* len);
  err_t readStaticWord(uint8_t* dst, int maxlen, int* len);
  bool isDelimiter(uint8_t c) const;

  TcpClient* mClient;
  Method mMethod;
  char* mPath;
};

#endif
