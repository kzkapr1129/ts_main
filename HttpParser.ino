#define DEBUG
#include "Logger.h"
#include "TcpClient.h"
#include "HttpParser.h"

#define TMP_METHOD_BUF 8
#define TMP_BUF_SIZE 32

HttpParser::HttpParser(TcpClient* client)
    : mClient(client), mMethod(HttpParser::UNKNOWN), mPath(NULL)  {
  err_t err = init();
  LOGIF((err != ERR_OK), "failed HttpParser init: err=%d", err);

  // 使われなかったヘッダ情報を読み出し捨てる
  while (true) {
    uint8_t b;
    err_t err = client->readByte(&b);
    if (err != ERR_OK) {
      break;
    }

    if (!b) {
      break; // reached EOF
    }
  }
}

HttpParser::~HttpParser() {
  free(mPath);
}

HttpParser::Method HttpParser::method() const {
}

const char* HttpParser::path() const {
  return mPath;
}

err_t HttpParser::init() {
  uint8_t methodStr[TMP_METHOD_BUF];
  int methodStrLen = 0;
  err_t err = readStaticWord(methodStr, TMP_METHOD_BUF, &methodStrLen);
  if (err != ERR_OK) {
    return err;
  }

  if (methodStrLen == 3 && !memcmp("GET", methodStr, 3)) {
    mMethod = GET;
  } else {
    mMethod = UNKNOWN;
    return ERR_VAL;
  }

  int pathLen = 0;
  uint8_t* path = NULL;
  err = readDinamicWord(path, &pathLen);
  if (err != ERR_OK) {
    free(path);
    return err;
  }

  mPath = (char*)path;

  return ERR_OK;
}

err_t HttpParser::readDinamicWord(uint8_t*& dst, int* len) {  
  uint8_t tmpBuf[TMP_BUF_SIZE];
  uint8_t* tot_str = NULL;

  int tmpBufLen = 0;
  int offset = 0;

  err_t err = ERR_OK;
  do {
    err = readStaticWord(tmpBuf, TMP_BUF_SIZE, &tmpBufLen);
    if (err != ERR_OK) {
      break;
    }

    if (tmpBufLen <= 0) {
      err = ERR_VAL;
      break;
    }

    uint8_t* tmp = (uint8_t*)realloc(tot_str, offset + tmpBufLen + 1);
    if (tmp == NULL) {
      free(tot_str);
      tot_str = NULL;
      offset = 0;
      err = ERR_MEM;
      break;
    }

    tot_str = tmp;
    memcpy(tot_str + offset, tmpBuf, tmpBufLen);
    offset = offset + tmpBufLen;

  } while (tmpBufLen >= TMP_BUF_SIZE);

  if (tot_str) {
    dst = tot_str;
    dst[offset] = 0;
    *len = offset;
  } else {
    dst = NULL;
    *len = 0;
  }

  return err;
}

err_t HttpParser::readStaticWord(uint8_t* dst, int maxlen, int* len) {
  if (!dst || maxlen <= 0 || !len) {
    return ERR_ARG;
  }

  err_t err = ERR_OK;
  int offset = 0;
  while (true) {
    uint8_t c;
    err = mClient->readByte(&c);
    if (err != ERR_OK) {
      break;
    }
    
    if (isDelimiter(c)) {
      break;
    }

    dst[offset++] = c;

    if (maxlen <= offset) {
      break;
    }
  }

  *len =  offset;

  return err;
}

bool HttpParser::isDelimiter(uint8_t c) const {
  if (c == 0x20) { // space
    return true;
  } else if (c == 0x2c) { // comma
    return true;
  } else if (c == 0x3b) { // colon
    return true;
  } else if (c == 0x3b) { // semi-colon
    return true;
  } else if (c == '(') {
    return true;
  } else if (c == ')') {
    return true;
  }

  return false;
}
