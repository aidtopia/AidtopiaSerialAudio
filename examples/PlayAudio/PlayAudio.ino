#include <AidtopiaSerialAudio.h>

using SerialAudio = AidtopiaSerialAudio;
static SerialAudio audio;

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
  Serial.println(F("\nAidtopia Serial Audio Test\n"));

  // Initialize the audio module connected to Serial1.  If you don't have
  // a hardware serial port available, you can use SoftwareSerial.  You do
  // not have to call the serial device's begin method first.  This will
  // set the baud rate.
  audio.begin(Serial1);
  audio.selectSource(SerialAudio::Device::SDCARD);
  audio.queryFirmwareVersion();
  audio.queryEqProfile();
  audio.setVolume(20);
  audio.playTrack(3);
}

// We'll let the user enter raw command numbers and parameters to explore
// what's possible.
static uint8_t cmd = 0x00;
static uint16_t param = 0x0000;
static int state = 0;

void sendIt(uint8_t msgid, uint16_t param) {
  using aidtopia::Message;
  Serial.println(F("--"));
  audio.enqueue(Message{static_cast<Message::ID>(msgid), param});
}

class SpyHooks : public SerialAudio::Hooks {
  public:
    using EqProfile = SerialAudio::EqProfile;

    void onDeviceChange(Device device, DeviceChange change) override {
      printDeviceName(device);
      Serial.print(' ');
      Serial.println(change == DeviceChange::INSERTED ? F("INSERTED") : F("REMOVED"));
    }

    void onFinishedFile(Device device, uint16_t index) override {
      printDeviceName(device);
      Serial.print(F(" index "));
      Serial.print(index);
      Serial.println(F(" finished playing."));
      audio.decreaseVolume();
      audio.playNextFile();
    }

    void onQueryResponse(Parameter param, uint16_t value) override {
      switch (param) {
        case Parameter::EQPROFILE:
          Serial.print(F("Equalizer profile: "));
          printEqProfileName(static_cast<EqProfile>(value));
          Serial.println();
          break;
        case Parameter::FIRMWAREVERSION:
          Serial.print(F("Firmware version: "));
          Serial.println(value);
          break;
        case Parameter::VOLUME:
          Serial.print(F("Volume: "));
          Serial.println(value);
          break;
        default:
          Serial.println(F("Ignoring query response."));
          break;
      }
    }

  private:
    void printDeviceName(Device device) {
      switch (device) {
        case Device::USB:     Serial.print(F("USB"));     break;
        case Device::SDCARD:  Serial.print(F("SDCARD"));  break;
        case Device::FLASH:   Serial.print(F("FLASH"));   break;
        case Device::AUX:     Serial.print(F("AUX"));     break;
        case Device::SLEEP:   Serial.print(F("SLEEP"));   break;
        default:              Serial.print(F("Unknown")); break;
      }
    }

    void printEqProfileName(EqProfile eq) {
      switch (eq) {
        case EqProfile::NORMAL:    Serial.print(F("Normal"));    break;
        case EqProfile::POP:       Serial.print(F("Pop"));       break;
        case EqProfile::ROCK:      Serial.print(F("Rock"));      break;
        case EqProfile::JAZZ:      Serial.print(F("Jazz"));      break;
        case EqProfile::CLASSICAL: Serial.print(F("Classical")); break;
        case EqProfile::BASS:      Serial.print(F("Bass"));      break;
        default:                   Serial.print(F("Unknown"));   break;
    }
}

} hooks;


void loop() {
  audio.update(&hooks);

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
