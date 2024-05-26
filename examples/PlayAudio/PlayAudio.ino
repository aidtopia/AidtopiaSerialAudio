#include <AidtopiaSerialAudio.h>

// static Aidtopia_SerialAudio audio;  // prints nothing to Serial
static Aidtopia_SerialAudioWithLogging audio;  // logs to Serial

auto constexpr stop_pin = 4;
auto constexpr play_pin = 5;

void setup() {
  Serial.begin(115200);
  Serial.println("\nAidtopia Serial Audio Test\n");

  // Initialize the audio module connected to Serial1.  If you don't have
  // a hardware serial port available, you can use SoftwareSerial.
  audio.logDetail();
  audio.begin(Serial1);

  pinMode(stop_pin, INPUT_PULLUP);
  pinMode(play_pin, INPUT_PULLUP);
}

void loop() {
  audio.update();
  if (digitalRead(stop_pin) == LOW) {
    audio.stop();
    while (digitalRead(stop_pin) == LOW) {}
  }
  if (digitalRead(play_pin) == LOW) {
    audio.playTrack(4);
    while (digitalRead(play_pin) == LOW) {}
  }
}
