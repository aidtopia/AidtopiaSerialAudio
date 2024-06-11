#include <AidtopiaSerialAudio.h>

using SerialAudio = aidtopia::SerialAudio;
static SerialAudio audio;

class Button {
  public:
    void begin(uint8_t pin) {
      m_pin = pin;
      pinMode(m_pin, INPUT_PULLUP);
    }

    bool update() {
      if (m_state == State::UP && digitalRead(m_pin) == LOW) {
        m_state = State::DOWN;
        return true;
      } else if (m_state == State::DOWN && digitalRead(m_pin) == HIGH) {
        m_state = State::UP;
      }
      return false;
    }

  private:
    uint8_t m_pin = 0;
    enum class State : uint8_t { UP, DOWN } m_state = State::UP;
};

Button red_button;
Button green_button;
Button blue_button;
Button yellow_button;

static bool readyToSetUpAudio = false;
static bool readyForSongChange = false;

void setUpAudio() {
  // It's a good idea to explicitly select your input source.
  audio.selectSource(SerialAudio::Device::SDCARD);

  // Set some parameters.
  audio.setVolume(20);
  audio.setEqProfile(SerialAudio::EqProfile::BASS);

  // Start making some noise!
  audio.playTrack(3);

  readyToSetUpAudio = false;
}

void changeSong() {
  audio.decreaseVolume();
  audio.playNextFile();
  readyForSongChange = false;
}

class SpyHooks : public SerialAudio::Hooks {
  public:
    using EqProfile = SerialAudio::EqProfile;
    using ModuleState = SerialAudio::ModuleState;
    using Error = SerialAudio::Error;

    void onError(Error error_code) override {
      Serial.print(F("Error: "));
      Serial.print(static_cast<uint16_t>(error_code), HEX);
      Serial.print(' ');
      printErrorName(error_code);
      Serial.println();
    }

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
      readyForSongChange = true;
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
        case Parameter::STATUS:
          Serial.print(F("Status: device="));
          printDeviceName(static_cast<Device>((value >> 8) & 0xFF));
          Serial.print(F(" state="));
          printModuleStateName(static_cast<ModuleState>(value & 0xFF));
          Serial.println();
          break;
        default:
          break;
      }
    }

    void onInitComplete(uint8_t devices) override {
      static constexpr Device all_devices[] =
        {Device::USB, Device::SDCARD, Device::FLASH};
      Serial.print(F("Device(s) online:"));
      for (auto const &device : all_devices) {
        auto const mask = static_cast<uint8_t>(device);
        if ((devices & mask) == mask) {
          Serial.print(' ');
          printDeviceName(device);
        }
      }
      Serial.println();

      readyToSetUpAudio = true;
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

    void printModuleStateName(ModuleState state) {
      switch (state) {
        case ModuleState::STOPPED:      [[fallthrough]];
        case ModuleState::ALT_STOPPED:  Serial.print("Stopped");  break;
        case ModuleState::PLAYING:      Serial.print("Playing");  break;
        case ModuleState::PAUSED:       Serial.print("Paused");   break;
        case ModuleState::ASLEEP:       Serial.print("Asleep");   break;
        default:                        Serial.print("unknown state"); break;
      }
    }

    void printErrorName(Error error) {
      switch (error) {
        case Error::UNSUPPORTED:    Serial.print(F("UNSUPPORTED"));   break;
        case Error::NOSOURCES:      Serial.print(F("NOSOURCES"));     break;
        case Error::SLEEPING:       Serial.print(F("SLEEPING"));      break;
        case Error::SERIALERROR:    Serial.print(F("SERIALERROR"));   break;
        case Error::BADCHECKSUM:    Serial.print(F("BADCHECKSUM"));   break;
        case Error::FILEOUTOFRANGE: Serial.print(F("FILEOUTOFRANGE")); break;
        case Error::TRACKNOTFOUND:  Serial.print(F("TRACKNOTFOUND")); break;
        case Error::INSERTIONERROR: Serial.print(F("INSERTIONERROR")); break;
        case Error::SDCARDERROR:    Serial.print(F("SDCARDERROR"));   break;
        case Error::ENTEREDSLEEP:   Serial.print(F("ENTEREDSLEEP"));  break;
        case Error::TIMEDOUT:       Serial.print(F("TIMEDOUT"));      break;
        default:                    Serial.print(F("Unknown"));       break;
      }
    }
} hooks;

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nAidtopia Serial Audio Test\n"));

  Serial.print(F("sizeof(audio) = "));
  Serial.println(sizeof(audio));

  // Initialize the audio module connected to Serial1.  If you don't have
  // a hardware serial port available, you can use SoftwareSerial.  You do
  // not have to call the serial device's begin method first.  This will
  // set the baud rate.
  audio.begin(Serial1);

  // You can issue audio commands now, and they'll queue up until the device
  // is ready.  Instead, we'll wait for the OnInitComplete callback.

  red_button.begin(4);
  green_button.begin(5);
  blue_button.begin(6);
  yellow_button.begin(7);
}

void loop() {
  audio.update(&hooks);
  if (readyToSetUpAudio) { setUpAudio(); }
  if (readyForSongChange) { changeSong(); }

  if (red_button.update())      audio.stop();
  if (green_button.update())    audio.loopFolder(1);
  if (blue_button.update())     audio.loopFolder(47);  // no such folder.
  if (yellow_button.update())   audio.reset();
}
