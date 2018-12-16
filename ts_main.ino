#define LWIP_INTERNAL

#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include "WiFiServer.h"
#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "TcpServer.h"
#include "HttpParser.h"

#define DEBUG
#include "Logger.h"

#define SSID "esp"
#define PASS "12345678"

static TcpServer srv;

void setup() {
  wdt_disable();

  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAP(SSID, PASS);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  srv.start(IP_ANY_TYPE, 80);
}

void loop() {
  TcpClient* client = srv.available();
  if (client) {
    if (client->isAvailable()) {

      HttpParser parser(client);

      HttpParser::Method m = parser.method();
      const char* path = parser.path();
      LOG("Request: method=%d, path=%s", m, path);

      client->write((uint8_t*)"HTTP/1.1 200 OK\r\n", 17);
      client->write((uint8_t*)"Content-Type: text/html\r\n\r\n", 27);
      client->write((uint8_t*)"<!DOCTYPE html><html><body>hello</body></html>\r\n", 48);

      client->close();
      delete client;
    }
  }

  srv.trim();
}
