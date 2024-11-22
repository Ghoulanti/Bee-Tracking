// Konyvtarak
#include "SparkFun_UHF_RFID_Reader.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>

// RFID class meghivasa
RFID rfidModule;

// RX, TX, kommunikacio
SoftwareSerial softSerial(2, 3);
#define rfidSerial softSerial
#define rfidBaud 38400
#define moduleType ThingMagic_M6E_NANO

const int TAG_SIZE = 12;
int eepromAdress = 0;
const int MAX_TAGS = 10; // HA TOBB RFID TAGOT HASZNALUNK EZT A SZAMOT NOVELJUK!!!

// Ezt olvashatosag miatt adom hozza + egyszerubb igy (meg talan nem foglal igy annyi helyet az EEPROM-ban a sok ID)
class ID {
  public:
    byte name;
    time_t time;
    char id[TAG_SIZE];

    ID(byte _name = 0, time_t _time = 0, const char* _id = "") {
      name = _name;
      time = _time;
    }
};

bool setupRfidModule(long baudRate);
int findNextAddress();
void processTag();
const unsigned long TAG_TIMEOUT = 5000; // EZT IS LEHET VALTOZTATNI (lehet nem is kell, megoldastol fugg)
ID bees[MAX_TAGS];
ID tags[MAX_TAGS];

void setup() {
  // Serial portok aktivalasa
  Serial.begin(115200);
  while (!Serial);

  if (setupRfidModule(rfidBaud) == false) {
    Serial.println(F("A RedBoard be van dugva a gepbe, tehat az RFID reader nem aktiv. A d betuvel uritheto az EEPROM"));
  }

  // ITT KELL BEALLITANI AZ IDOT!!!  (ora, perc, masodperc, nap, honap, ev)
  setTime(0, 0, 0, 1, 1, 1970);

  // RFID reader aktivalas
  rfidModule.setRegion(REGION_EUROPE);
  rfidModule.setReadPower(2500);
  rfidModule.startReading();
  retrieveDataFromEEPROM();

  // Minden tagnak csinalunk egy objectet
  for (int i = 0; i < MAX_TAGS; i++) {
    bees[i].name = (byte)i;
    bees[i].time = 0;
    memset(bees[i].id, 0, TAG_SIZE);
  }
}

void loop() {
  // Ha a gepbe van bedugva megjeleniti az adatokat (DELETE LATER)
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'r') {
      retrieveDataFromEEPROM();
    }
  }

  // Ha a gepbe van bedugva a RedBoard ezzel kiuriti az EEPROM-ot
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'd') {
      clearEEPROM();
    }
  }

  // RFID tagok feldolgozasa
  if (rfidModule.check() == true) {
    byte responseType = rfidModule.parseResponse();
    if (responseType == RESPONSE_IS_TAGFOUND) {
      processTag();
    }
  }
}

// EEPROM kiuritese
void clearEEPROM() {
  for (int i =0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0xFF);
  }
  Serial.println("EEPROM kiuritve");
}

// Adatok megszerzese az EEPROM-bol
void retrieveDataFromEEPROM() {
  Serial.println(F("Adatok keresese..."));
  
  int address = 0;
  while (address < EEPROM.length()) {
    ID tempBee;
    EEPROM.get(address, tempBee);
    address += sizeof(ID);

    if (tempBee.time != 0) {
      Serial.print(F("Meh: "));
      Serial.print(tempBee.name);
      Serial.print(F(" Beolvasas ideje: "));
      printTime(tempBee.time);
    }
  }
}

void printTime(time_t date) {
  Serial.print(year(date));
  Serial.print("/");
  Serial.print(month(date));
  Serial.print("/");
  Serial.print(day(date));
  Serial.print(" ");
  Serial.print(hour(date));
  Serial.print(":");
  Serial.print(minute(date));
  Serial.print(":");
  Serial.println(second(date));
}

// Adatok el,ementese
void storeBeeDataInEEPROM(const ID & bee) {
  int address = findNextAddress();
  EEPROM.put(address, bee);
}


int findNextAddress() {
  int address = 0;
  ID tempBee;
  while (address < EEPROM.length()) {
    EEPROM.get(address, tempBee);
      if (tempBee.time == 0) {
        return address;
      }
    address += sizeof(ID);
  }
  return address;
}

// RFID modul setupolasa
bool setupRfidModule(long baudRate) {
  rfidModule.begin(rfidSerial, moduleType);
  rfidSerial.begin(baudRate);

  rfidModule.getVersion();
  if (rfidModule.msg[0] != ALL_GOOD) {
    return false;
  }

  rfidModule.setTagProtocol();
  rfidModule.setAntennaPort();
  return true;
}

// Beolvasott ID-k kezelese
void processTag() {
  byte tagEPCBytes = rfidModule.getTagEPCBytes();
  char tagID[TAG_SIZE] = {0};
  int index = 0;

  for (byte x = 0; x < tagEPCBytes; x++) {
    if (rfidModule.msg[31 + x] < 0x10 && index < TAG_SIZE - 2) {
      tagID[index++] = '0';
    }
    sprintf(tagID + index, "%02X", rfidModule.msg[31 + x]);
    index += 2;
  }
  int address = findNextAddress();
  EEPROM.put(address, tagEPCBytes);
  
  
}

