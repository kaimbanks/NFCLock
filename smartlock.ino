// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_PN532.h"

// pin variables & class scope variables
const int SS_PIN = A2;
const int SCK_PIN = A3;
const int MISO_PIN = A4;
const int MOSI_PIN = A5;

const int IRQ_PIN = D2;
const int RST_PIN = D3;

const int LOCK_BUTTON = A0;

const int SERVO_PIN = D1;

const int lockedPosition = 105;
const int unlockedPosition = 15;

int lockStatus; // 1 is locked; 0 unlocked; -1 error

Servo motor;

// set to SPI or I2C depending on how the header file is configured
#if PN532_MODE == PN532_SPI_MODE
  Adafruit_PN532 nfc(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
#elif PN532_MODE == PN532_I2C_MODE
  Adafruit_PN532 nfc(IRQ_PIN, RST_PIN);
#endif

// runs once at the start of each boot-up
// 1. get serial port up and running
// 2. make sure the NFC board is recognized
// 3. expose variables and functions to the Particle cloud
// 4. get the servo set up
// 5. get the unlock button set up
// 6. set the status of the lock according to the servo's position
void setup() {
    // 1
    Serial.begin(9600);

    // 2
    nfc.begin();

    uint32_t versiondata;

    do {
        versiondata = nfc.getFirmwareVersion();
        if (!versiondata) {
            Serial.println("no board");
            delay(1000);
        }
    }
    while (!versiondata);

    Serial.print("Found chip PN5");
    Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. ");
    Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata>>8) & 0xFF, DEC);

    // configure board to read RFID tags
    nfc.SAMConfig();

    Serial.println("Ready for an ISO14443A Card.");

    // 3
    Particle.function("toggleLock", toggleLock);
    Particle.function("getStatus", getStatus);
    Particle.variable("lockStatus", lockStatus);
    Particle.function("moveServo", moveServo);

    // 4
    motor.attach(SERVO_PIN);
    motor.write(unlockedPosition);

    // 5
    pinMode(LOCK_BUTTON, INPUT_PULLUP);

    // 6
    lockStatus = 0;
}

int moveServo(String input) {
    motor.write(input.toInt());
    return motor.read();
}

// check whether the hardware button is pressed
bool isButtonPressed() {
    return (digitalRead(LOCK_BUTTON) == 0);
}

// runs every 200ms to check for presence of NFC card
void loop() {
    // delay(500);
    // Serial.print("lock status is: ");
    // Serial.println(lockStatus);

    if (lockStatus == 1) {
        Serial.print("Checking for card: ");
        uint8_t success = 0;
        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
        uint8_t uidLength = 0;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
        Serial.println(success);
        if (success) {
            // Display some basic information about the card
            Serial.println("Found an ISO14443A card");

            if (uidLength == 4) { // found a legit card; print info and unlock
                uint32_t cardid = uid[0];
                cardid <<= 8;
                cardid |= uid[1];
                cardid <<= 8;
                cardid |= uid[2];
                cardid <<= 8;
                cardid |= uid[3];
                Serial.print("Mifare Classic card #");
                Serial.println(cardid);
                unlock();
            }

            Serial.println("");
        }
    }
    else {
        if (isButtonPressed()) { // door is unlocked and the button was pressed; lock it
            lock();
        }
    }
}

// cloud function to get current status of lock.
// checks the servo directly for more accurate status.
// returns the status
int getStatus(String input) {
    int current = motor.read();
    Serial.print("motor read is ");
    Serial.println(motor.read());
    if (current > lockedPosition - 5) {
        lockStatus = 1;
    } else if (current < unlockedPosition + 5) {
        lockStatus = 0;
    } else {
        lockStatus = 2;
    }
    Serial.print("result is: ");
    Serial.println(lockStatus);
    return lockStatus;
}

// cloud function to toggle the state of the lock
// returns the status
int toggleLock(String input) {
    if (lockStatus == 0) {
        lock();
        return 1;
    } else {
        unlock();
        return 0;
    }
    // return getStatus("lock");
}

// local function to lock the lock
void lock() {
    motor.write(lockedPosition);
    Serial.println("locking");
    lockStatus = 1;
    nfc.begin();
    nfc.SAMConfig();
    // delay(500);
    // getStatus("lock");
    // nfc.begin();
    // nfc.SAMConfig();
}

// local function to lock the lock
void unlock() {
    motor.write(unlockedPosition);
    // nfc.SAMConfig();
    Serial.println("unlocking");
    lockStatus = 0;
    // delay(500);
    // getStatus("lock");
}
