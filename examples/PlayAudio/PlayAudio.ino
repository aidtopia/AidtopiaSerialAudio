#include <AidtopiaSerialAudio.h>

using namespace aidtopia;

static SerialAudioManager audio;

static constexpr int hexValue(char ch) {
  return ('0' <= ch && ch <= '9') ? ch - '0' :
         ('A' <= ch && ch <= 'F') ? ch - 'A' + 0xA :
         ('a' <= ch && ch <= 'f') ? ch - 'a' + 0xA :
         -1;
}

static constexpr bool isHexDigit(char ch) {
  return hexValue(ch) >= 0;
}

template <typename T>
bool applyHexDigit(char ch, T &value) {
  if (!isHexDigit(ch)) return false;
  value <<= 4;
  value |= hexValue(ch);
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nAidtopia Serial Audio Test\n");

  // Initialize the audio module connected to Serial1.  If you don't have
  // a hardware serial port available, you can use SoftwareSerial.  You do
  // not have to call the serial device's begin method first.  This will
  // set the baud rate.
  audio.begin(Serial1);
  audio.selectSource(SerialAudioManager::Device::SDCARD);
  uint8_t volume;
  if (audio.queryVolume(volume)) {
    Serial.print(F("Queried volume before setting: "));
    Serial.println(volume);
  }
  audio.setVolume(20);
  audio.playFile(3);
  if (audio.queryVolume(volume)) {
    Serial.print(F("Queried volume after setting: "));
    Serial.println(volume);
  }
}

// We'll let the user enter raw command numbers and parameters to explore
// what's possible.
static uint8_t cmd = 0x00;
static uint16_t param = 0x0000;
static int state = 0;

void sendIt(uint8_t msgid, uint16_t param) {
  Serial.println(F("--"));
  audio.enqueue(Message{static_cast<Message::ID>(msgid), param});
}

void loop() {
  audio.update();

  while (Serial.available()) {
    char ch = Serial.read();
    switch (state) {
      case 0:
        cmd = 0; param = 0;
        if (applyHexDigit(ch, cmd))   { state = 1; break; }
        else                          { state = (ch == ' ') ? 0 : 999; break; }
        break;
      case 1:
        if (applyHexDigit(ch, cmd))   { state = 2; break; }
        [[fallthrough]];
      case 2:
        if (ch == '\n')               { sendIt(cmd, param); state = 0; break; }
        else                          { state = (ch == ' ') ? 3 : 999; break; }
        break;
      case 3:
        if (applyHexDigit(ch, param)) { break; }
        [[fallthrough]];
      case 4:
        if (ch == '\n')               { sendIt(cmd, param); state = 0; break; }
        else                          { state = (ch == ' ') ? 4 : 999; break; }
        break;
      default:
        if (ch == '\n')               { state = 0; break; }
        break;
    }
  }
}
