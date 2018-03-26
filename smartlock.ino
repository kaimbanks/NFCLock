// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_PN532.h"

const int SCK_PIN = A3;
const int MOSI_PIN = A5;
const int SS_PIN = A2;
const int MISO_PIN = A4;

const int IRQ_PIN = D2;
const int RST_PIN = D3;

const int LOCK_BUTTON = A0;

String lockStatus;

bool authorized = false;

Servo motor;

int servoPos = 0;

#if PN532_MODE == PN532_SPI_MODE
  Adafruit_PN532 nfc(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
#elif PN532_MODE == PN532_I2C_MODE
  Adafruit_PN532 nfc(IRQ_PIN, RST_PIN);
#endif

int testCredentials(String input) {
    if (input == "secret_key") {
        authorized = true;
        return 0;
    } else {
        authorized = false;
        return -1;
    }
}

void setup() {
    Particle.function("toggleLock", toggleLock);
    Particle.variable("lockStatus", lockStatus);
    RGB.color(255, 255, 255);
    motor.attach(WKP);

    motor.write(180);
    lockStatus = "unlocked";
    Particle.function("testCreds", testCredentials);
    pinMode(D7, OUTPUT);
    pinMode(LOCK_BUTTON, INPUT_PULLUP);
    Serial.begin(115200);

    setUpNFC();
}

bool isButtonPressed() {
    return (digitalRead(LOCK_BUTTON) == 0);
}

void setUpNFC() {
    nfc.begin();

    uint32_t versiondata;

    do {
        versiondata = nfc.getFirmwareVersion();
        if (!versiondata) {
            Serial.println("no board");
            Serial.println(SCK_PIN);
            delay(1000);

        }
    }
    while (!versiondata);

    // Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    // Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
    // Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

    // configure board to read RFID tags
    nfc.SAMConfig();

    Serial.println("Waiting for an ISO14443A Card ...");
}

void loop() {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
        // Display some basic information about the card
        Serial.println("Found an ISO14443A card");
        Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);

        if (uidLength == 4)
        {
            // We probably have a Mifare Classic card ...
            uint32_t cardid = uid[0];
            cardid <<= 8;
            cardid |= uid[1];
            cardid <<= 8;
            cardid |= uid[2];
            cardid <<= 8;
            cardid |= uid[3];
            Serial.print("Mifare Classic card #");
            Serial.println(cardid);
            openLock();

        }

        Serial.println("");
    }
    else {
        // motor.write(0);
    }

    if (isButtonPressed()) {
        closeLock();
    }
}

int toggleLock (String input) {
    if (lockStatus == "unlocked") {
        closeLock();
    } else {
        openLock();
    }
    return 0;
}

void closeLock() {
    digitalWrite(D7, LOW);
    motor.write(5);
    Serial.println("locking");
    lockStatus = "locked";
}

void openLock() {
    digitalWrite(D7, HIGH);
    motor.write(180);
    Serial.println("unlocking");
    lockStatus = "unlocked";
}
