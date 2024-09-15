#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <MFRC522.h>
#include <WiFiManager.h> 
#include "credentials.h"
#include <SPIFFS.h>
#include <FS.h>
MFRC522 rfid; 
MFRC522::MIFARE_Key key; 
byte nuidPICC[4] = {0x0, 0x0, 0x0, 0x0};
String strRFID = "";
String serverIP;
constexpr uint8_t RST_PIN = 0;         
constexpr uint8_t SS_PIN = 5;  
void inserData();


bool readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if(!file || file.isDirectory()){
      Serial.println("- failed to open file for reading");
      return false;
  }
  
  serverIP = file.readString();
  Serial.println("Server IP set to: " + serverIP);
  file.close();
  return true;
}
void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

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
    nuidPICC[3 - i] = rfid.uid.uidByte[i];
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
  String url = 
  "http://" + 
  serverIP + 
  "/flutter-rfid-attendance-system-backend/set/insert_attendance.php";  
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  uint32_t* rfid = (uint32_t*)nuidPICC;

  String payload = "{\"rfid\" : \"" + String(*rfid) + "\"}";
  Serial.print("\nSQL: ");
  Serial.println(payload);
  Serial.print("Result: ");
  Serial.println(http.POST(payload));
}


void setup() {
  Serial.begin(115200);

  WiFiManager wm;
 
  WiFiManagerParameter param("sip", "server ip", "", 32);
  wm.addParameter(&param);
  
  wm.autoConnect("RFID", "admin123");
  serverIP = String(param.getValue());
  Serial.println(serverIP);
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  if(!serverIP.isEmpty()){
    Serial.println("Reset IP");
    writeFile(SPIFFS, "/server.cfg", serverIP.c_str());
  } else {
    if(!readFile(SPIFFS, "/server.cfg")) {
      writeFile(SPIFFS, "/server.cfg", serverIP.c_str());
    }
  }
 
  SPI.begin();

  rfid.PCD_Init(SS_PIN, RST_PIN); 
  
  byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
  if(v != 0xFF || v!= 0x00){
    Serial.println("RFID Ok!");
  }

 

}

void loop() {
  readRFID();
  
}

