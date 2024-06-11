#include <AidtopiaSerialAudio.h>

using SerialAudio = aidtopia::SerialAudio;
SerialAudio audio;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Hello, World!"));
  audio.begin(Serial1);
  audio.playTrack(3);
  audio.loopCurrentTrack();
}

void loop() {
  audio.update();
}
