#include <AidtopiaSerialAudio.h>

AidtopiaSerialAudio audio;

void setup() {
  Serial.begin(115200);
  Serial.println(F("FireAndForget example for AidtopiaSerialAudio."));

  // Initialize `audio` to use the module wired to Serial1.  You
  // can use any available HardwareSerial or SoftwareSerial port.
  // Note that it's not necessary to call the begin() method for
  // the serial port yourself.
  audio.begin(Serial1);

  // Most of the methods, like playTrack, put a command on a
  // queue.  The first queued command will be sent during an
  // update() call once the library detects that the module is
  // initialized.  See the loop() function in this example for
  // more details.
  audio.playTrack(3);

  // You can have only a small number of commands on the queue
  // at a time.  Currently, it's 4.
  
  // You cannot use the queue to set up a playlist because it
  // doesn't wait for a track to finish before sending the
  // command to play the next track.  To play a sequence of
  // tracks, you can use one of the looping functions, like
  // loopFolder(), or you can use callback hooks.
}

void loop() {
  // It's important to regularly call update().  During an update:
  //   * A queued command or query may be sent to the module.
  //   * A response from the module may be handled.
  //   * A callback hook may be called (if you provide hooks).
  audio.update();
}
