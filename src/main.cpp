#include <Arduino.h>
#include <EEPROM.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SPI.h>

namespace {

constexpr uint8_t RFID_SS_PIN = 10;
constexpr uint8_t RFID_RST_PIN = 9;
constexpr uint8_t DOOR_SERVO_PIN = 6;
constexpr uint8_t LOW_SOURCE_PIN = 4;
constexpr uint8_t WRITE_MODE_SENSE_PIN = 2;
constexpr uint8_t DELETE_MODE_SENSE_PIN = 3;

constexpr uint8_t SERVO_CLOSED_ANGLE = 0;
constexpr uint8_t SERVO_OPEN_ANGLE = 180;
constexpr unsigned long DOOR_OPEN_DURATION_MS = 4000;

constexpr uint16_t EEPROM_MAGIC = 0x5243;
constexpr uint8_t EEPROM_VERSION = 1;
constexpr uint8_t MAX_STORED_CARDS = 20;
constexpr uint8_t MAX_UID_BYTES = 10;

struct CardRecord {
  uint8_t size;
  uint8_t uid[MAX_UID_BYTES];
};

struct CardStorageHeader {
  uint16_t magic;
  uint8_t version;
  uint8_t count;
};

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);
Servo doorServo;

CardStorageHeader cardStorageHeader{};
CardRecord storedCardRecords[MAX_STORED_CARDS]{};

bool isDoorOpen = false;
unsigned long doorOpenedAtMs = 0;

void printUid(const MFRC522::Uid &uid) {
  for (byte i = 0; i < uid.size; ++i) {
    if (uid.uidByte[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(uid.uidByte[i], HEX);
    if (i + 1 < uid.size) {
      Serial.print(':');
    }
  }
}

bool sameUid(const MFRC522::Uid &uid, const CardRecord &card) {
  if (uid.size != card.size) {
    return false;
  }
  for (byte i = 0; i < uid.size; ++i) {
    if (uid.uidByte[i] != card.uid[i]) {
      return false;
    }
  }
  return true;
}

void loadStorage() {
  EEPROM.get(0, cardStorageHeader);

  if (cardStorageHeader.magic != EEPROM_MAGIC || cardStorageHeader.version != EEPROM_VERSION || cardStorageHeader.count > MAX_STORED_CARDS) {
    cardStorageHeader.magic = EEPROM_MAGIC;
    cardStorageHeader.version = EEPROM_VERSION;
    cardStorageHeader.count = 0;
    EEPROM.put(0, cardStorageHeader);

    for (uint8_t i = 0; i < MAX_STORED_CARDS; ++i) {
      storedCardRecords[i] = {};
    }
    return;
  }

  const int baseAddress = static_cast<int>(sizeof(cardStorageHeader));
  for (uint8_t i = 0; i < cardStorageHeader.count; ++i) {
    EEPROM.get(baseAddress + static_cast<int>(i) * static_cast<int>(sizeof(CardRecord)), storedCardRecords[i]);
  }

  for (uint8_t i = cardStorageHeader.count; i < MAX_STORED_CARDS; ++i) {
    storedCardRecords[i] = {};
  }
}

void saveStorage() {
  EEPROM.put(0, cardStorageHeader);

  const int baseAddress = static_cast<int>(sizeof(cardStorageHeader));
  for (uint8_t i = 0; i < cardStorageHeader.count; ++i) {
    EEPROM.put(baseAddress + static_cast<int>(i) * static_cast<int>(sizeof(CardRecord)), storedCardRecords[i]);
  }
}

int findCardIndex(const MFRC522::Uid &uid) {
  for (uint8_t i = 0; i < cardStorageHeader.count; ++i) {
    if (sameUid(uid, storedCardRecords[i])) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool addCard(const MFRC522::Uid &uid) {
  if (uid.size == 0 || uid.size > MAX_UID_BYTES) {
    return false;
  }

  if (findCardIndex(uid) >= 0) {
    return true;
  }

  if (cardStorageHeader.count >= MAX_STORED_CARDS) {
    return false;
  }

  CardRecord &slot = storedCardRecords[cardStorageHeader.count];
  slot = {};
  slot.size = uid.size;
  for (byte i = 0; i < uid.size; ++i) {
    slot.uid[i] = uid.uidByte[i];
  }

  cardStorageHeader.count++;
  saveStorage();
  return true;
}

bool deleteCard(const MFRC522::Uid &uid) {
  const int index = findCardIndex(uid);
  if (index < 0) {
    return false;
  }

  for (int i = index; i < static_cast<int>(cardStorageHeader.count) - 1; ++i) {
    storedCardRecords[i] = storedCardRecords[i + 1];
  }

  storedCardRecords[cardStorageHeader.count - 1] = {};
  cardStorageHeader.count--;
  saveStorage();
  return true;
}

bool isWriteModeEnabled() {
  return digitalRead(WRITE_MODE_SENSE_PIN) == LOW;
}

bool isDeleteModeEnabled() {
  return digitalRead(DELETE_MODE_SENSE_PIN) == LOW;
}

void smoothMoveServo(uint8_t target) {
  const uint8_t current = doorServo.read();
  const int8_t step = (target > current) ? 2 : -2;

  for (int angle = current; angle != target; angle += step) {
    if ((step > 0 && angle >= static_cast<int>(target)) ||
        (step < 0 && angle <= static_cast<int>(target))) {
      break;
    }
    doorServo.write(static_cast<uint8_t>(angle));
    delay(10);
  }
  doorServo.write(target);
}

void lockDoor() {
  smoothMoveServo(SERVO_CLOSED_ANGLE);
  isDoorOpen = false;
  doorOpenedAtMs = 0;
}

void unlockDoor() {
  smoothMoveServo(SERVO_OPEN_ANGLE);
  isDoorOpen = true;
  doorOpenedAtMs = millis();
}

void pulseServoReaction() {
  doorServo.write(SERVO_OPEN_ANGLE);
  delay(200);
  doorServo.write(SERVO_CLOSED_ANGLE);
  isDoorOpen = false;
  doorOpenedAtMs = 0;
}

void handleDoorTimer() {
  if (isDoorOpen && millis() - doorOpenedAtMs >= DOOR_OPEN_DURATION_MS) {
    lockDoor();
    Serial.println(F("Door locked"));
  }
}

void processCard() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print(F("Card UID: "));
  printUid(mfrc522.uid);
  Serial.println();

  if (isWriteModeEnabled()) {
    if (addCard(mfrc522.uid)) {
      Serial.println(F("Card stored in EEPROM"));
      pulseServoReaction();
    } else {
      Serial.println(F("Failed to store card (memory full or invalid UID)"));
    }
  } else if (isDeleteModeEnabled()) {
    if (deleteCard(mfrc522.uid)) {
      Serial.println(F("Card deleted from EEPROM"));
      pulseServoReaction();
    } else {
      Serial.println(F("Card not found"));
    }
  } else {
    const int index = findCardIndex(mfrc522.uid);
    if (index >= 0) {
      Serial.print(F("Access granted, slot #"));
      Serial.println(index + 1);
      unlockDoor();
    } else {
      Serial.println(F("Access denied"));
    }
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(300);
}

}  // namespace

void setup() {
  Serial.begin(115200);

  pinMode(LOW_SOURCE_PIN, OUTPUT);
  digitalWrite(LOW_SOURCE_PIN, LOW);
  pinMode(WRITE_MODE_SENSE_PIN, INPUT_PULLUP);
  pinMode(DELETE_MODE_SENSE_PIN, INPUT_PULLUP);

  doorServo.attach(DOOR_SERVO_PIN);
  lockDoor();

  SPI.begin();
  mfrc522.PCD_Init();

  loadStorage();

  Serial.println(F("NFC access controller ready"));
  Serial.print(F("Stored cards: "));
  Serial.println(cardStorageHeader.count);
  Serial.println(F("Short PIN 4 and PIN 2 to enter write mode"));
  Serial.println(F("Short PIN 4 and PIN 3 to enter delete mode"));
}

void loop() {
  handleDoorTimer();
  processCard();
}