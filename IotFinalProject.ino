#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

// ======================================================
// PIN DEFINITIONS
// ======================================================
// RFID MFRC522
#define RFID_SS_PIN   3
#define RFID_RST_PIN  2

// SD Card Module
#define SD_CS_PIN     10

// RTC DS1302
#define RTC_IO_PIN    5
#define RTC_SCLK_PIN  6
#define RTC_CE_PIN    9

// Output devices
#define LED_PIN       7
#define BUZZER_PIN    8

// ======================================================
// OBJECTS
// ======================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

ThreeWire myWire(RTC_IO_PIN, RTC_SCLK_PIN, RTC_CE_PIN);
RtcDS1302<ThreeWire> Rtc(myWire);

// ======================================================
// STUDENT UID LIST
// Replace these sample UIDs with your real card UIDs
// ======================================================
struct Student {
  unsigned long uidDec;
  int id;
  const char* name;
};

const Student students[] = {
  {369715717, 1, "Nurbol Makhmudulla"},
  {2386988548, 2, "Tom Crusie"},
  {3546464770, 3, "Bekzat Tokbolat"},
  {68361902, 4, "Abdinur Otegen"},
};

const int studentCount = sizeof(students) / sizeof(students[0]);

// ======================================================
// GLOBAL VARIABLES
// ======================================================
unsigned long lastScanTime = 0;
byte lastUID[4] = {0, 0, 0, 0};
bool hasLastUID = false;
const unsigned long scanCooldown = 3000;  // 3 seconds

// ======================================================
// FUNCTION DECLARATIONS
// ======================================================
void showWelcome();
void beepScan();
void blinkLed();
void printDateTimeToLCD(const RtcDateTime& now);
void printDateTimeToFile(File &file, const RtcDateTime& now);
unsigned long getUIDDecimal();
const char* getStudentName(unsigned long uidDec);
int getStudentID(unsigned long uidDec);
void logAttendance(const char* name, unsigned long uidDec);
bool isSameUID(const byte* a, const byte* b);
void setRTCIfNeeded();
void printUIDToSerial(const byte* uid, byte uidSize);

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

 pinMode(10, OUTPUT);        // Important for Arduino Uno SPI
pinMode(RFID_SS_PIN, OUTPUT);
pinMode(SD_CS_PIN, OUTPUT);

digitalWrite(10, HIGH);
digitalWrite(RFID_SS_PIN, HIGH);
digitalWrite(SD_CS_PIN, HIGH);

  lcd.init();
  lcd.backlight();

  SPI.begin();
  mfrc522.PCD_Init();

  // RTC
  setRTCIfNeeded();

  // SD Card
  if (!SD.begin(SD_CS_PIN)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SD Card Failed");
    lcd.setCursor(0, 1);
    lcd.print("Check wiring");
    Serial.println("SD Card initialization failed!");
    while (true);
  }

if (!SD.exists("ATTEND.CSV")) {
  File file = SD.open("ATTEND.CSV", FILE_WRITE);
  if (file) {
    file.println("Date ; Time ; UID Card ; Student Name; Attendance Taken");
    file.close();
  }
}

  showWelcome();
}

// ======================================================
// MAIN LOOP
// ======================================================
void loop() {
  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Read the card serial
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }


  // Prevent repeated scans of the same card too quickly
  if (hasLastUID && isSameUID(mfrc522.uid.uidByte, lastUID) && (millis() - lastScanTime < scanCooldown)) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  // Save current UID as last scanned
  memcpy(lastUID, mfrc522.uid.uidByte, 4);
  hasLastUID = true;
  lastScanTime = millis();

unsigned long uidDec = getUIDDecimal();

Serial.print("UID Decimal: ");
Serial.println(uidDec);

beepScan();

const char* studentName = getStudentName(uidDec);
int studentID = getStudentID(uidDec);

logAttendance(studentName, uidDec);

if (studentID == 2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Access Denied");
  lcd.setCursor(0, 1);
  lcd.print("Unknown Card");
} else {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome!!");

  lcd.setCursor(0, 1);
  lcd.print("User ID: ");
  lcd.print(studentID);

  blinkLed();
}

  delay(1500);
  showWelcome();

  // Stop communication with the card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// ======================================================
// FUNCTIONS
// ======================================================
void showWelcome() {
  RtcDateTime now = Rtc.GetDateTime();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID Attendance");

  lcd.setCursor(0, 1);
  // Show date in short format: DD/MM HH:MM
  if (now.Day() < 10) lcd.print("0");
  lcd.print(now.Day());
  lcd.print("/");
  if (now.Month() < 10) lcd.print("0");
  lcd.print(now.Month());
  lcd.print(" ");
  if (now.Hour() < 10) lcd.print("0");
  lcd.print(now.Hour());
  lcd.print(":");
  if (now.Minute() < 10) lcd.print("0");
  lcd.print(now.Minute());
}

void beepScan() {
  tone(BUZZER_PIN, 2000);
  delay(200);
  noTone(BUZZER_PIN);
}

void blinkLed() {
  digitalWrite(LED_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
}

void printDateTimeToLCD(const RtcDateTime& now) {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (now.Day() < 10) lcd.print("0");
  lcd.print(now.Day());
  lcd.print("/");
  if (now.Month() < 10) lcd.print("0");
  lcd.print(now.Month());
  lcd.print("/");
  lcd.print(now.Year());

  lcd.setCursor(0, 1);
  if (now.Hour() < 10) lcd.print("0");
  lcd.print(now.Hour());
  lcd.print(":");
  if (now.Minute() < 10) lcd.print("0");
  lcd.print(now.Minute());
  lcd.print(":");
  if (now.Second() < 10) lcd.print("0");
  lcd.print(now.Second());
}

void printDateTimeToFile(File &file, const RtcDateTime& now) {
  if (now.Day() < 10) file.print("0");
  file.print(now.Day());
  file.print("/");

  if (now.Month() < 10) file.print("0");
  file.print(now.Month());
  file.print("/");

  file.print(now.Year());
  file.print(",");

  if (now.Hour() < 10) file.print("0");
  file.print(now.Hour());
  file.print(":");

  if (now.Minute() < 10) file.print("0");
  file.print(now.Minute());
  file.print(":");

  if (now.Second() < 10) file.print("0");
  file.print(now.Second());
}
unsigned long getUIDDecimal() {
  unsigned long uidDec = 0;

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidDec = uidDec * 256 + mfrc522.uid.uidByte[i];
  }

  return uidDec;
}
const char* getStudentName(unsigned long uidDec) {
  for (int i = 0; i < studentCount; i++) {
    if (students[i].uidDec == uidDec) {
      return students[i].name;
    }
  }

  return "Unknown";
}
int getStudentID(unsigned long uidDec) {
  for (int i = 0; i < studentCount; i++) {
    if (students[i].uidDec == uidDec) {
      return students[i].id;
    }
  }

  return -1;
}

void logAttendance(const char* name, unsigned long uidDec) {
  RtcDateTime now = Rtc.GetDateTime();
  int studentID = getStudentID(uidDec);

  File file = SD.open("ATTEND.CSV", FILE_WRITE);

  if (!file) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SD Write Error");
    lcd.setCursor(0, 1);
    lcd.print("Cannot Save");
    return;
  }

  // Date
  if (now.Day() < 10) file.print("0");
  file.print(now.Day());
  file.print("/");

  if (now.Month() < 10) file.print("0");
  file.print(now.Month());
  file.print("/");

  file.print(now.Year());
  file.print(";");

  // Time
  if (now.Hour() < 10) file.print("0");
  file.print(now.Hour());
  file.print(":");

  if (now.Minute() < 10) file.print("0");
  file.print(now.Minute());
  file.print(":");

  if (now.Second() < 10) file.print("0");
  file.print(now.Second());
  file.print(";");

  // UID Decimal
  file.print(uidDec);
  file.print(";");

  // Student name
  file.print(name);
  file.print(";");

  // Attendance status
  if (studentID == 2) {
    file.println("NO");
  } else {
    file.println("YES");
  }

  file.close();
}

bool isSameUID(const byte* a, const byte* b) {
  for (int i = 0; i < 4; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void setRTCIfNeeded() {
  Rtc.Begin();
  Rtc.SetIsWriteProtected(false);

  if (!Rtc.IsDateTimeValid()) {
    // Sets RTC to the compile time of this sketch
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    Rtc.SetDateTime(compiled);
  }

  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true);
  }
}

void printUIDToSerial(const byte* uid, byte uidSize) {
  Serial.print("UID: ");
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    if (i < uidSize - 1) Serial.print(" ");
  }
  Serial.println();
}