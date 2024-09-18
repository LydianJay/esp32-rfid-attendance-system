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
bool error    = false;

constexpr uint8_t RST_PIN       = 0;         
constexpr uint8_t SS_PIN        = 5;  
constexpr uint8_t pinError      = 25;
constexpr uint8_t pinComError   = 32;
constexpr uint8_t pinReady      = 14;
constexpr uint8_t pinReset      = 15;
void inserData();


void deleteFile(fs::FS &fs, const char * path) {
  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return;
  }
  Serial.println("Deleting config file");
  file.close();
  fs.remove(path);
}


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
 
  Serial.println();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  for(int i = 0; i < 3; i++) {
    digitalWrite(pinReady, LOW);
    delay(100);
    digitalWrite(pinReady, HIGH);
    delay(100);
  }
  
  
  inserData();
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
  if(http.POST(payload) != 200) {
    Serial.println("Error Com");
    for(int i = 0; i < 5; i++){
      digitalWrite(pinReady, LOW);
      digitalWrite(pinComError, HIGH);
      delay(320);
      digitalWrite(pinReady, HIGH);
      digitalWrite(pinComError, LOW);
      delay(320);
    }
    
    
  }
  else {
    digitalWrite(pinComError, LOW);
    
  }
  digitalWrite(pinReady, LOW);
}


void setup() {
  Serial.begin(115200);
  pinMode(pinError,     OUTPUT);
  pinMode(pinComError,  OUTPUT);
  pinMode(pinReady,     OUTPUT);
  pinMode(pinReset,     INPUT_PULLDOWN);
  digitalWrite(pinError,    LOW);
  digitalWrite(pinComError, HIGH);
  digitalWrite(pinReady,    LOW);
  WiFiManager wm;
  const int resetRead = digitalRead(pinReset);
  if(resetRead == HIGH){
    wm.resetSettings();
    
    digitalWrite(pinError,    HIGH);
    digitalWrite(pinComError, HIGH);
    digitalWrite(pinReady,    LOW);
    Serial.println("Resetting WiFi...");
  }
  Serial.println(resetRead);

  WiFiManagerParameter param("sip", "server ip", "", 32);
  wm.addParameter(&param);
  
  wm.autoConnect("RFID", "admin123");
  serverIP = String(param.getValue());
  Serial.println(serverIP);
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    error = true;
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
  if(v != 0xFF && v != 0x00){
    Serial.println("RFID Ok!");

  }
  else {
    Serial.println("Version Reg: " + String(v));
    error = true;
  }
  

  WiFi.status() == WL_CONNECTED ? Serial.println("Wifi Connected\n") : error = true;

  // Stops device for functioning if error has occured in initialization
  while (error) {
    digitalWrite(pinError, HIGH);
    delay(350);
    digitalWrite(pinError, LOW);
    delay(350);
  }
  
 
  digitalWrite(pinComError,    LOW);
}

void loop() {
  readRFID();
  
}

