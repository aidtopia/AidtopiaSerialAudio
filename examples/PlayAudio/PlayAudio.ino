#include <AidtopiaSerialAudio.h>

static Aidtopia_SerialAudio audio;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, World!\n");
  audio.begin(Serial1);
}

void loop() {
  audio.update();
}
