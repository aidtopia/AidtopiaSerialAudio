#include <AidtopiaSerialAudio.h>

// This example demonstrates the use of callback hooks to implement a
// playlist with AidtopiaSerialAudio.
//
// I recommend you read through the FireAndForget example first.

AidtopiaSerialAudio audio;

// Let's make a playlist.
int const playlist[] = {2, 1, 3};
auto constexpr playlistSize = sizeof(playlist) / sizeof(playlist[0]);
int playlistPosition = 0;  // current position in the list

// To work through the playlist, we'll need to know when to start
// playing the first track, and when to play the next track.  For
// that, we'll use callback hooks.

// Derive a class from AidtopiaSerialAudio::Hooks that overrides the
// callback methods we care about.
class PlaylistHooks : public AidtopiaSerialAudio::Hooks {
  public:

    // This method will be called once the module is initialized.
    void onInitComplete(Devices devices) override {
      // `devices` is the set of storage devices that are connected
      // to the audio module.  Let's say we want to use the SD card.
      if (!devices.has(Device::SDCARD)) {
        Serial.println("There's no SD card in the audio player.");
        return;
      }
    
      // Select the SD card as the storage device we want to play
      // the tracks from.  It's possible that the SD card is already
      // the selected storage device.  Heck, it might even be the
      // only device.  But there's no harm in selecting it
      // explicitly.
      audio.selectSource(Device::SDCARD);
      
      if (playlistSize == 0) {
        Serial.println("There's nothing in the playlist.");
        return;
      }

      // Also queue up a command to play the first track in our
      // playlist.
      playlistPosition = 0;
      audio.playTrack(playlist[playlistPosition]);

      // Update playlistPosition to the next position.
      // The `% playlistSize` makes it wrap around, so once the last
      // track has played, it'll start over with the first one.
      playlistPosition = (playlistPosition + 1) % playlistSize;
    }

    // This method will be called when a file has finished playing.
    void onFinishedFile(Device /*device*/, uint16_t /*index*/) override {
      // `device` is the device that the file was playing from.
      // `index` is the filesystem index of the file that finished
      // playing.  We don't care about either of these, so I've
      // commented out the names from the function signature.

      if (playlistSize == 0) return;

      // Queue up a command to play the next track in our playlist.
      audio.playTrack(playlist[playlistPosition]);
      
      // Update our position in the playlist.
      playlistPosition = (playlistPosition + 1) % playlistSize;
    }

    // There are a few more callbacks you can override, but they
    // aren't necessary for this example.
};

// We'll need an instance of our PlaylistHooks.
PlaylistHooks myHooks;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Playlist example for AidtopiaSerialAudio."));

  audio.begin(Serial1);

  // We're not queuing up any commands yet.  Instead, we'll wait
  // until we get the callback that tells us the audio module is
  // ready.
}

void loop() {
  // Remember, it's important to regularly call update().
  // Notice that, in this example, we pass in our hooks.
  audio.update(myHooks);
}
