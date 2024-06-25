#include <AidtopiaSerialAudio.h>

// This example demonstrates the use of callback hooks to get the
// results of queries sent to the audio module.  Watch the output
// in the Arduino IDE Serial Monitor window.
//
// For a very simple example of using the AidtopiaSerialAudio
// library see the FireAndForget example.
//
// For another example of using callback hooks, see the Playlist
// example.

AidtopiaSerialAudio audio;

// Derive a class from AidtopiaSerialAudio::Hooks that overrides the
// callback methods we care about.
class QueryResponseHooks : public AidtopiaSerialAudio::Hooks {
  public:

    // This method will be called when the module responds to a
    // query that was initiated by us.  (The AidtopiaSerialModule
    // library sometimes sends its own queries and handles the
    // responses itself without passing them to the hooks.)
    void onQueryResponse(Parameter parameter, uint16_t value) override {
      // `parameter` tells us which value is being reported and
      // `value` is the actual value.
      //
      // Unfortunately, how you interpret the value depends on which
      // parameter is being reported.  For some, like volume, it's
      // straightforward.  Others require converting and/or breaking
      // apart the value.
      //
      // [Originally, the library did these conversions and called
      // parameter-specific callback methods.  But that led to more
      // RAM usage for the v-tables, which was deemed unreasonable
      // for most embedded projects.]

      switch (parameter) {
        case Parameter::EQPROFILE: {
          Serial.print(F("Equalizer Profile: "));
          // The value should correspond directly to one of the names
          // in the EqProfile enumeration.
          auto const profile = static_cast<EqProfile>(value);
          switch (profile) {
            case EqProfile::NORMAL:     Serial.println(F("Normal"));    break;
            case EqProfile::POP:        Serial.println(F("Pop"));       break;
            case EqProfile::ROCK:       Serial.println(F("Rock"));      break;
            case EqProfile::JAZZ:       Serial.println(F("Jazz"));      break;
            case EqProfile::CLASSICAL:  Serial.println(F("Classical")); break;
            case EqProfile::BASS:       Serial.println(F("Bass"));      break;
            default:                    Serial.println(F("Unknown"));   break;
          }
          break;
        }

        case Parameter::FIRMWAREVERSION: {
          Serial.print(F("Firmware version: "));
          // Not all modules respond to this query.  For those that
          // do, the value is just a number.
          auto const version = value;
          Serial.println(version);
          break;
        }

        case Parameter::STATUS: {
          Serial.print(F("Status: "));
          // The response to a status query needs to be separated into
          // a high byte that indicates which device is currently
          // selected, and a low byte which gives the module state.
          auto const device = static_cast<Device>(     (value >> 8) & 0xFF);
          auto const state  = static_cast<ModuleState>((value     ) & 0xFF);

          // When the module is in a low-power state called sleep, it
          // might response with this special device value.  In that case,
          // the state is ignored.
          if (device == Device::SLEEP) {
            Serial.println(F("Asleep"));
            return;
          }

          switch (state) {
            case ModuleState::ALT_STOPPED:  [[fallthrough]];
            case ModuleState::STOPPED:  Serial.print(F("Stopped")); break;
            case ModuleState::PLAYING:  Serial.print(F("Playing")); break;
            case ModuleState::PAUSED:   Serial.print(F("Paused"));  break;
            default:              Serial.print(F("unknown state")); break;
          }

          Serial.print(' ');

          switch (device) {
            case Device::USB:     Serial.println(F("USB drive"));     break;
            case Device::SDCARD:  Serial.println(F("SD card"));       break;
            case Device::FLASH:   Serial.println(F("Flash memory"));  break;

            // A special case for when a computer is connected over USB.
            case Device::AUX:     Serial.println(F("External USB"));  break;

            default:              Serial.println(F("unknown device")); break;
          }
          break;
        }

        case Parameter::VOLUME: {
          Serial.print(F("Volume: "));
          // The volume is just a number from 0 to 30.
          auto const volume = value;
          Serial.println(volume);
          break;
        }

        default:
          Serial.print(F("Received a query response for a parameter this "));
          Serial.println(F("example doesn't track."));
          break;
      }
    }

    // There are more callbacks you can override, but they aren't
    // necessary for this example.
};

// We'll need an instance of our QueryResponseHooks.
QueryResponseHooks myHooks;

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nQueries example for AidtopiaSerialAudio\n"));

  audio.begin(Serial1);

  // To make just a few queries, you can simply queue them up.
  audio.queryFirmwareVersion();
  audio.queryVolume();
  audio.queryEqProfile();
  audio.queryStatus();

  // CAUTION:  That's four commands in the queue, which is the limit.
  // If you want to make more queries or send additional commands,
  // you'll have to issue the additional ones from a callback hook.
}

void loop() {
  // Remember, it's important to regularly call update().
  // Notice that, in this example, we pass in our hooks.
  audio.update(myHooks);
}
