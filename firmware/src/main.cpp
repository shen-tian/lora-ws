#include <SPI.h>
#include <RH_RF95.h>
#include <SSD1306.h>
#include <WiFi.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <rBase64.h>

#include "fonts.h"

const char *ssid = "SSID";
const char *password = "password";

// Hardware config
#define SCK     5    // GPIO5  -- SCK
#define MISO    19   // GPIO19 -- MISO
#define MOSI    27   // GPIO27 -- MOSI
#define SS      18   // GPIO18 -- CS
#define RST     12   // GPIO14 -- RESET (If Lora does not work, replace it with GPIO14)
#define DI0     26   // GPIO26 -- IRQ(Interrupt Request)

#define SDA     21
#define SCL     22

AsyncWebServer server(80);

RH_RF95 rf95(SS, DI0);

SSD1306 display(0x3c, SDA, SCL);

//U8G2_SSD1306_128X64_NONAME_F_HW_I2C display()


// state stuff

float loraFreq = 915.0;

int lastRssi;

char foo[50];

void initRadio() {
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);

  digitalWrite(RST, LOW);
  delay(10);
  digitalWrite(RST, HIGH);
  delay(10);

  Serial.print("LoRa init: ");
  if (rf95.init())
    Serial.println("Success");
  else
    Serial.println("Failed");

  Serial.print("LoRa set freq: ");
  if (rf95.setFrequency(loraFreq))
    Serial.println(loraFreq);
  else
    Serial.println("Failed");

  Serial.print("LoRa config modem: ");
  if (rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128))
    Serial.println("Success");
}

void initDisplay(){
  display.init();
  display.setFont(Roboto_Mono_12);
}

void initNetwork(){
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void printToOled(int line, const char* format, ...){
  va_list arguments;
  char buff[50];
  va_start(arguments, format);
  vsprintf(buff, format, arguments);
  va_end(arguments);
  display.drawString(0, line * 15, buff);
}

void updateScreen()
{
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  printToOled(0, "RSSI: %d", lastRssi);
  printToOled(1, "%s", ssid);
  IPAddress ip = WiFi.localIP();
  printToOled(2, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  printToOled(3, "%.2fMhz SF 7", loraFreq);
  display.display();
}

void loraRecv() {
  if (rf95.available()) {
    // Should be a message for us now
    byte buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len))
    {
      //digitalWrite(led, HIGH);
      Serial.println("Message received");
      Serial.print("RSSI: ");

      lastRssi = rf95.lastRssi();

      Serial.println(rf95.lastRssi(), DEC);

      Serial.print("Length: ");
      Serial.println(len);

      rbase64.encode(buf,len);
      Serial.println(rbase64.result());

      //digitalWrite(led, LOW);
    }
    else
    {
      Serial.println("recv failed");
    }
  }
}

void loraSend() {
  int inputLength = String(foo).length();
  if (inputLength > 0) {
    int decodedLength = rbase64_dec_len(foo, inputLength);
    char buffer[decodedLength];
    Serial.println("Sending packet: ");
    Serial.println(decodedLength);
    rbase64_decode(buffer, foo, inputLength);
    rf95.send((uint8_t*) buffer, decodedLength);
    rf95.waitPacketSent();
    foo[0] = 0x00;
  }
}

void setup() {

  //pinMode(led, OUTPUT);
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available

  initRadio();
  initDisplay();
  initNetwork();

  server.on("/message", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("payload")) {
      String newMessage = request->getParam("payload")->value();
      // handle URL encoding
      newMessage.replace("-", "+");
      newMessage.replace("_", "/");
      newMessage.replace(".", "=");

      Serial.print("Sending: ");
      Serial.println(newMessage);

      sprintf(foo, "%s", newMessage.c_str());
    }
    request->send(200, "text/plain", "Fine");
  });

  server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("freq")) {
      float newFrequency = request->getParam("freq")->value().toFloat();
      if (rf95.setFrequency(newFrequency)) {
        loraFreq = newFrequency;
        Serial.print("Set frequency to: ");
        Serial.println(newFrequency);
      }
    }
    request->send(200, "text/plain", "Fine");
  });

  server.begin();
}

void loop()
{
  loraRecv();
  loraSend();

  updateScreen();
}
