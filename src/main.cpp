#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include "credentials.h"

MFRC522 rfid; 
MFRC522::MIFARE_Key key; 
byte nuidPICC[4] = {0x0, 0x0, 0x0, 0x0};
String strRFID = "";
constexpr uint8_t RST_PIN = 0;         
constexpr uint8_t SS_PIN = 5;  
void inserData();
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}


void readRFID() {


  if ( ! rfid.PICC_IsNewCardPresent()) {
    return;
  }
   
  if ( ! rfid.PICC_ReadCardSerial()){
    return;
  }

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  strRFID = "";
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
    strRFID += nuidPICC[i];
  }
  Serial.println(F("The NUID tag is:"));
  Serial.print(F("In hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  inserData();
  Serial.println();

  rfid.PICC_HaltA();


  rfid.PCD_StopCrypto1();


}

void inserData() {
  HTTPClient http;
  http.begin("http://192.168.254.102/flutter-rfid-attendance-system-backend/set/insert_attendance.php");
  http.addHeader("Content-Type", "application/json");


  String payload = "{\"rfid\" : \"" + strRFID + "\"}";
  Serial.print("\nSQL: ");
  Serial.println(payload);
  Serial.print("Result: ");
  Serial.println(http.POST(payload));
}


void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  SPI.begin();
  rfid.PCD_Init(SS_PIN, RST_PIN); 
  rfid.PCD_DumpVersionToSerial();




}

void loop() {
  readRFID();
  
}

