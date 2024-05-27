#include <AidtopiaSerialAudio.h>

static Aidtopia_SerialAudioWithLogging audio;  // logs to Serial

static constexpr int hexValue(char ch) {
  return ('0' <= ch && ch <= '9') ? ch - '0' :
         ('A' <= ch && ch <= 'F') ? ch - 'A' + 0xA :
         ('a' <= ch && ch <= 'f') ? ch - 'a' + 0xA :
         -1;
}

static constexpr bool isHexDigit(char ch) {
  return hexValue(ch) >= 0;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nAidtopia Serial Audio Test\n");

  // Select full logging detail before calling begin in order to see the
  // messages exchanged during initialization.
  audio.logDetail();

  // Initialize the audio module connected to Serial1.  If you don't have
  // a hardware serial port available, you can use SoftwareSerial.  You do
  // not have to call the serial device's begin method first.  This will
  // set the baud rate.
  audio.begin(Serial1);
}

// We'll let the user enter raw command numbers and parameters to explore
// what's possible.
static uint8_t cmd = 0x00;
static uint16_t param = 0x0000;
static bool feedback = true;
static int state = 0;

void loop() {
  audio.update();

  while (Serial.available()) {
    char ch = Serial.read();
    switch (state) {
      case 0:
        cmd = 0;
        param = 0;
        feedback = true;
        if (isHexDigit(ch)) {
          cmd = hexValue(ch);
          state = 1;
        } else {
          state = (ch == ' ') ? 0 : 999;
        }
        break;
      case 1:
        if (isHexDigit(ch)) {
          cmd <<= 4;
          cmd |= hexValue(ch);
          state = 2;
        } else if (ch == '\n') {
          audio.sendCommand(static_cast<Aidtopia_SerialAudio::MsgID>(cmd), param, feedback);
          state = 0;
        } else if (ch == 'x' || ch == 'X') {
          feedback = false;
          state = 5;
        } else {
          state = (ch == ' ') ? 3 : 999;
        }
        break;
      case 2:
        if (ch == '\n') {
          audio.sendCommand(static_cast<Aidtopia_SerialAudio::MsgID>(cmd), param, feedback);
          state = 0;
        } else if (ch == 'x' || ch == 'X') {
          feedback = false;
          state = 5;
        } else {
          state = (ch == ' ') ? 3 : 999;
        }
        break;
      case 3:
        if (isHexDigit(ch)) {
          param = hexValue(ch);
          state = 4;
        } else if (ch == '\n') {
          audio.sendCommand(static_cast<Aidtopia_SerialAudio::MsgID>(cmd), param, feedback);
          state = 0;
        } else if (ch == 'x' || ch == 'X') {
          feedback = false;
          state = 5;
        } else {
          state = (ch == ' ') ? 5 : 999;
        }
        break;
      case 4:
        if (isHexDigit(ch)) {
          param <<= 4;
          param |= hexValue(ch);
        } else if (ch == '\n') {
          audio.sendCommand(static_cast<Aidtopia_SerialAudio::MsgID>(cmd), param, feedback);
          state = 0;
        } else if (ch == 'x' || ch == 'X') {
          feedback = false;
          state = 5;
        } else {
          state = (ch == ' ') ? 5 : 999;
        }
        break;
      case 5:
        if (ch == 'x' || ch == 'X') {
          feedback = false;
        } else if (ch == '\n') {
          audio.sendCommand(static_cast<Aidtopia_SerialAudio::MsgID>(cmd), param, feedback);
          state = 0;
        } else {
          state = (ch == ' ') ? 5 : 999;
        }
        break;
      case 999:
        if (ch == '\n') state = 0;
        break;
    }
  }
}
