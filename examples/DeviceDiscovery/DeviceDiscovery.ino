#include <AidtopiaSerialAudio.h>

// This example demonstrates how to query a serial audio player to
// learn more about its state and about the storage devices
// attached to it.
//
// The Queries example shows how to use callback hooks to receive
// answers to simple queries.
//
// The PlayList example shows how to sequence commands in response
// to callbacks.
//
// This example is a more complex demonstration of both of those
// techniques.

AidtopiaSerialAudio audio;

// As usual, we derive a class from AidtopiaSerialAudio::Hooks in order
// to receive responses and asynchronous notifications from the audio
// player.  But these overrides will decide which commands to send
// next based on what's learned.
class DeviceDiscoveryHooks : public AidtopiaSerialAudio::Hooks {
  public:
    void onInitComplete(Devices devices) override {
      Serial.println(F("Initialization completed."));
      Serial.print(F("Devices Available:"));
      if (devices.has(Device::USB))     Serial.print(F(" USB"));
      if (devices.has(Device::SDCARD))  Serial.print(F(" SD"));
      if (devices.has(Device::FLASH))   Serial.print(F(" FLASH"));
      Serial.println();
    }
};

DeviceDiscoveryHooks hooks;

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nDeviceDiscovery example for AidtopiaSerialAudio\n"));

  audio.begin(Serial1);
}

void loop() {
  audio.update(hooks);
}
