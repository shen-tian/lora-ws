#include <RH_RF95.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>

// Builtin LED
#define LED_PIN 13

//LoRA radio for feather m0
// INT is different for 32u4 feather
#define RFM95_CS 12
#define RFM95_RST 11
#define RFM95_INT 10

// Change to 434.0 or other frequency, must match RX's freq!
// #define RF95_FREQ 915.0
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

bool sending = false;

#define CALLSIGN_LEN 4
#define CALLSIGN "BUTT"

#define MAGIC_NUMBER_LEN 2

uint8_t MAGIC_NUMBER[MAGIC_NUMBER_LEN] = {0x2c, 0x0b};

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

float currentFreq = RF95_FREQ;

void transmitData(float lat, float lon)
{

  char buff[20];

  sprintf(buff, "Using %.2fmhz", currentFreq);
  Serial.println(buff);

  uint8_t len = 2 * sizeof(int32_t) + sizeof(uint8_t) + CALLSIGN_LEN + MAGIC_NUMBER_LEN;
  uint8_t radiopacket[len];
  for (int i = 0; i < MAGIC_NUMBER_LEN; i++)
  {
    radiopacket[i] = MAGIC_NUMBER[i];
  }
  for (uint8_t i = 0; i < CALLSIGN_LEN; i++)
  {
    radiopacket[MAGIC_NUMBER_LEN + i] = '0';
  }
  void *p = radiopacket + MAGIC_NUMBER_LEN + CALLSIGN_LEN;
  *(int32_t *)p = (int32_t)(lat * 1e6);
  p = (int32_t *)p + 1;
  *(int32_t *)p = (int32_t)(lon * 1e6);
  p = (int32_t *)p + 1;
  *(uint8_t *)p = true;

  Serial.println("Saying Hai");

  sending = true;
  digitalWrite(LED_BUILTIN, HIGH);
  rf95.send((uint8_t *)radiopacket, len);
  rf95.waitPacketSent();
  digitalWrite(LED_BUILTIN, LOW);
  sending = false;
}

void initRadio()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // Manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init())
  {
    Serial.println("Radio init issue");
    while (1)
      ;
  }

  if (!rf95.setFrequency(RF95_FREQ))
  {
    Serial.println("Freq setting issue");
    while (1)
      ;
  }

  //rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);

  RH_RF95::ModemConfig config;

  config.reg_1d = 0x70 + 0x02;
  config.reg_1e = 0x70 + 0x04;
  config.reg_26 = 0x00;

  rf95.setModemRegisters(&config);

  rf95.setTxPower(23, false);

  Serial.println("Radio initiated!");
}

void setup()
{

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  Serial.println("Hii");

  initRadio();
}

char readString[256];

void loop()
{

  if (Serial.available() > 0)
  {
    int len = Serial.readBytesUntil('\n', readString, 256);

    if (len > 0)
    {
      readString[len] = 0;
      Serial.println(len);
      Serial.print("Read:");
      Serial.print(readString);
      Serial.println("!!");

      DynamicJsonBuffer jsonBuffer;
      JsonObject &root = jsonBuffer.parseObject(readString);

      float lat = 0;
      float lon = 0;

      // Test if parsing succeeds.
      if (!root.success())
      {
        Serial.println("parseObject() failed");
      }
      else
      {
        Serial.println("success");
        lat = root["lat"];
        lon = root["lon"];
      }

      transmitData(lat, lon);
    }
  }
}
