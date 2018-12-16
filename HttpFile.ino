#define DEBUG
#include "Logger.h"
#include "HttpFile.h"
#include "TcpClient.h"
#include <SD.h>
#include <SPI.h>

#define TMP_BUF_SIZE 32

bool HttpFile::initSDcard(int chip_select_pin) {
  pinMode(chip_select_pin, OUTPUT);
  digitalWrite(chip_select_pin, HIGH);

  return SD.begin(chip_select_pin);
}

HttpFile::HttpFile() {
}

HttpFile::~HttpFile() {
  close();
}

bool HttpFile::open(const char* path) {
  int len = strlen(path);
  if (len == 1 && path[0] == '/') {
    mFile = SD.open("index.htm");
    Serial.println("open index.htm");
  } else if (1 < len) {
    mFile = SD.open(&path[1]);
    LOG("open %s", &path[1]);
  } else {
    close();
    return false;
  }

  if (!mFile) {
    Serial.println("failed to open");
    return false;
  }

  return true;
}

void HttpFile::close() {
  if (mFile) {
    mFile.close();
  }
}

int HttpFile::send(TcpClient* client) {
  if (!client) {
    return 0;
  }

  int nBytes;
  uint8_t tmpBuf[TMP_BUF_SIZE];
  err_t err = ERR_OK;

  err = client->write((uint8_t*)"HTTP/1.1 200 OK Found\r\n", 23);
  if (err != ERR_OK) {
    return 0;
  }

  err = client->write((uint8_t*)"Content-Type: text/html\r\n\r\n", 27);
   if (err != ERR_OK) {
    return 0;
  }

  do {
    nBytes = mFile.read(tmpBuf, TMP_BUF_SIZE);
    if (0 < nBytes) {
      err = client->write(tmpBuf, nBytes);
      if (err != ERR_OK) {
        break;
      }
    }
  } while (nBytes == TMP_BUF_SIZE);

  LOGIF(err != ERR_OK, "failed to send err=%d", err);

  return 0;
}
