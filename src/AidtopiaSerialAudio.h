// AidtopiaSerialAudio
// Adrian McCarthy 2018-

// A library that works with various serial audio modules,
// like DFPlayer Mini, Catalex, etc.

#ifndef AIDTOPIASERIALAUDIO_H
#define AIDTOPIASERIALAUDIO_H

#include <utilities/timeout.h>

class Aidtopia_SerialAudio {
  public:
    explicit Aidtopia_SerialAudio();

    // Call `begin` in `setup` an instance of the HardwareSerial (e.g., Serial1)
    // or a SoftwareSerial object that the audio module is connected too.
    template <typename StreamType>
    void begin(StreamType &stream) {
      stream.begin(9600);
      m_stream = &stream;
      reset();
    }

    // Call `update` each time through `loop`.
    void update();

    enum Device {
      DEV_USB,      // a storage device connected via USB
      DEV_SDCARD,   // an SD card in the TF slot
      DEV_AUX,      // typically a connection to a PC
      DEV_SLEEP,    // a pseudo-device to indicate the module is sleeping
      DEV_FLASH,    // internal flash memory
      // Synonyms used in the datasheets:
      DEV_TF  = DEV_SDCARD, // The SD card slot is sometimes called TF (True Flash)
      DEV_PC  = DEV_AUX,    // The AUX input is typically a PC connection
      DEV_SPI = DEV_FLASH   // The internal flash memory is an SPI device
    };

    enum Equalizer {
      EQ_NORMAL,
      EQ_POP,
      EQ_ROCK,
      EQ_JAZZ,
      EQ_CLASSICAL,
      EQ_BASS
    };

    enum ModuleState {
      MS_STOPPED,
      MS_PLAYING,
      MS_PAUSED,
      MS_ASLEEP
    };

    enum Sequence {
      SEQ_LOOPALL,
      SEQ_LOOPFOLDER,
      SEQ_LOOPTRACK,
      SEQ_RANDOM,
      SEQ_SINGLE
    };

    // Reset and re-initialize the audio module.
    // 
    // Resetting causes an unavoidable click on the output.
    void reset();

    // Select a Device to be the current source.
    //
    // Many modules select `DEV_SDCARD` by default, which is usually
    // appropriate, but it's good practice to select it yourself to be
    // certain.
    void selectSource(Device device);
    
    // Play a file selected by its file system index.
    //
    // If you don't know the file index of the track you want, you should
    // probably use `playTrack` instead.
    //
    // This command can play a track from any folder on the selected source
    // device.  You can use `queryFileCount` to find out how many
    // files are available.
    //
    // Corresponds to playback sequence `SEQ_SINGLE`.
    void playFile(uint16_t file_index);

    // Play the next file based on the current file index.
    void playNextFile();

    // Play the previous file based on the current file index.
    void playPreviousFile();

    // Play a single file repeatedly.
    //
    // Corresponds to playback sequence `SEQ_LOOPTRACK`.
    void loopFile(uint16_t file_index);

    // Play all the files on the device, in file index order, repeatedly.
    //
    // Corresponds to playback sequence `SEQ_LOOPALL`.
    void loopAllFiles();

    // Play all the files on the current device in a random order.
    //
    // TBD: Does it repeat once it has played all of them?
    //
    // Corresponds to playback sequence `SEQ_RANDOM`.
    void playFilesInRandomOrder();
    
    // playTrack lets you specify a folder named with two decimal
    // digits, like "01" or "02", and a track whose name begins
    // with three decimal digits like "001.mp3" or
    // "014 Yankee Doodle.wav".
    void playTrack(uint16_t folder, uint16_t track);

    // This overload lets you select a track whose file name begins
    // with a three or four decimal digit number, like "001" or "2432".
    // The file must be in a top-level folder named "MP3".  It's
    // recommended that you have fewer than 3000 files in this folder
    // in order to avoid long startup delays as the module searches
    // for the right file.
    //
    // Even though the folder is named "MP3", it may contain .wav
    // files as well.
    void playTrack(uint16_t track);

    // Insert an "advertisement."
    //
    // This interrupts a track to play a track from a folder named
    // "ADVERT".  The track must have a file name as described in
    // the playTrack(uint16_t) command above.  When the advert track
    // has completed, the interruped audio track resumes from where
    // it was.
    //
    // This is typically used with the regular audio in the "MP3"
    // folder described above, but it can interrupt any track
    // regardless of how you started playing it.
    //
    // If no track is currently playing (e.g., if the device is
    // stopped or paused), this will result in an "insertion error."
    //
    // You cannot insert while an inserted track is alrady playing.
    void insertAdvert(uint16_t track);

    // Stops a track that was inserted with `insertAdvert`.  The
    // interrupted track will resume from where it was.
    void stopAdvert();

    // Stops any audio that's playing and resets the playback
    // sequence to `SEQ_SINGLE`.
    void stop();

    // Pauses the current playback.
    void pause();

    // Undoes a previous call to `pause`.
    //
    // Alternative use:  When a track finishes playing when the
    // playback sequence is `SEQ_SINGLE`, the next track (by file index)
    // is cued up and paused.  If you call this function about 100 ms
    // after an `onTrackFinished` notification, the cued track will
    // begin playing.
    void unpause();

    // Set the volume to a level in the range of 0 - 30.
    void setVolume(int volume);
    void increaseVolume();
    void decreaseVolume();

    // Selecting an equalizer interrupts the current playback, so it's
    // best to select the EQ before starting playback.  Alternatively,
    // you can also pause, select the new EQ, and then unpause.
    void selectEQ(Equalizer eq);

    // Sleeping doesn't seem useful.  To lower the current draw, use
    // `disableDAC`.
    void sleep();
    
    // Seems buggy.  Try reset() or selectSource().
    void wake();

    // Disabling the DACs when not in use saves a few milliamps.
    // Causes a click on the output.
    void disableDACs();

    // To re-enable the DACs after they've been disabled.
    // Causes a click on the output.
    void enableDACs();

    // Ask how many audio files (total) are on a source device, including
    // the root directory and any subfolders.  This is useful for knowing
    // the upper bound on a `playFile` call.  Hook `onDeviceFileCount`
    // for the result.
    void queryFileCount(Device device);

    void queryCurrentFile(Device device);

    // Ask how many folders there are under the root folder on the current
    // source device.
    void queryFolderCount();

    // Ask which device is currently selected as the source and whether
    // it's playing, paused, or stopped.  Can also indicate if the module
    // is asleep.  Hook `onStatus` for the result.  (Current device doesn't
    // seem to be reliable on DFPlayer Mini.)
    void queryStatus();

    // Query the current volume.
    //
    // Hook `onVolume` for the result.
    void queryVolume();

    // Query the current equalizer setting.
    //
    // Hook `onEqualizer` for the result.
    void queryEQ();

    // Query the current playback sequence.
    //
    // Hook `onPlaybackSequence` for the result.
    void queryPlaybackSequence();

    // Query the firmware version.
    //
    // Hook `onFirmwareVersion` for the result.  Catalex doesn't respond
    // to this query, so watch for a timeout error.
    void queryFirmwareVersion();

  protected:
  public:  // TEMPORARILY CHANGING VISIBILITY FOR RESEARCH
    // These are the message IDs for the serial protocol.  Different modules
    // support different subsets of them.
    enum MsgID : uint8_t {
      // First, the commands
      MID_PLAYNEXT          = 0x01,
      MID_PLAYPREVIOUS      = 0x02,
      MID_PLAYFILE          = 0x03,
      MID_VOLUMEUP          = 0x04,
      MID_VOLUMEDOWN        = 0x05,
      MID_SETVOLUME         = 0x06,
      MID_SELECTEQ          = 0x07,
      MID_LOOPFILE          = 0x08,
      MID_LOOPFLASHTRACK    = MID_LOOPFILE,  // Alternate msg not used
      MID_SELECTSOURCE      = 0x09,
      MID_SLEEP             = 0x0A,
      MID_WAKE              = 0x0B,  // If not supported, use MID_SELECTSOURCE
      MID_RESET             = 0x0C,
      MID_RESUME            = 0x0D,
      MID_UNPAUSE           = MID_RESUME,
      MID_PAUSE             = 0x0E,
      MID_PLAYFROMFOLDER    = 0x0F,
      MID_AMPLIFIER         = 0x10,  // Documented by Flyron, but not supported
      MID_LOOPALL           = 0x11,
      MID_PLAYFROMMP3       = 0x12,  // "MP3" here refers to name of folder
      MID_INSERTADVERT      = 0x13,
      MID_PLAYFROMBIGFOLDER = 0x14,
      MID_STOPADVERT        = 0x15,
      MID_STOP              = 0x16,
      MID_LOOPFOLDER        = 0x17,
      MID_RANDOMPLAY        = 0x18,
      MID_LOOPCURRENTFILE   = 0x19,
      MID_DISABLEDAC        = 0x1A,
      MID_PLAYLIST          = 0x1B,  // Might not work, unusual message length
      MID_PLAYWITHVOLUME    = 0x1C,  // seems redundant

      // Asynchronous messages from the module
      MID_DEVICEINSERTED    = 0x3A,
      MID_DEVICEREMOVED     = 0x3B,
      MID_FINISHEDUSBFILE   = 0x3C,
      MID_FINISHEDSDFILE    = 0x3D,
      MID_FINISHEDFLASHFILE = 0x3E,

      // Quasi-asynchronous
      // This arrives some time after MID_RESET (or power on), so I've been
      // viewing it as an asynchronous notification.  The Flyron documentation
      // says the user can send this to query which storage devices are online,
      // but that doesn't work for any of the modules I've tried.
      MID_INITCOMPLETE      = 0x3F,

      // Basic replies
      MID_ERROR             = 0x40,
      MID_ACK               = 0x41,

      // Queries and their responses
      MID_STATUS            = 0x42,
      MID_VOLUME            = 0x43,
      MID_EQ                = 0x44,
      MID_PLAYBACKSEQUENCE  = 0x45,
      MID_FIRMWAREVERSION   = 0x46,
      MID_USBFILECOUNT      = 0x47,
      MID_SDFILECOUNT       = 0x48,
      MID_FLASHFILECOUNT    = 0x49,
      // no 0x4A? MID_AUXFILECOUNT?
      MID_CURRENTUSBFILE    = 0x4B,
      MID_CURRENTSDFILE     = 0x4C,
      MID_CURRENTFLASHFILE  = 0x4D,
      MID_FOLDERTRACKCOUNT  = 0x4E,
      MID_FOLDERCOUNT       = 0x4F,

      // We're going to steal an ID for our state machine's use.
      MID_ENTERSTATE        = 0x00
    };
  protected:
    enum ErrorCode : uint16_t {
      EC_UNSUPPORTED        = 0x00,  // MsgID used is not supported
      EC_NOSOURCES          = 0x01,  // module busy or no sources installed
      EC_SLEEPING           = 0x02,  // module is sleeping
      EC_SERIALERROR        = 0x03,  // serial communication error
      EC_BADCHECKSUM        = 0x04,  // module received bad checksum
      EC_FILEOUTOFRANGE     = 0x05,  // this is the file index
      EC_TRACKNOTFOUND      = 0x06,  // couldn't find track by numeric prefix
      EC_INSERTIONERROR     = 0x07,  // couldn't start ADVERT track
      EC_SDCARDERROR        = 0x08,  // ??
      EC_ENTEREDSLEEP       = 0x0A,  // entered sleep mode??

      // And reserving one for our state machine
      EC_TIMEDOUT           = 0x0100
    };
  
    // Manages a buffered message with all the protocol details.
    class Message {
      public:
        Message();

        enum Feedback { NO_FEEDBACK = 0x00, FEEDBACK = 0x01 };
        void set(MsgID msgid, uint16_t param, Feedback feedback = NO_FEEDBACK);

        const uint8_t *getBuffer() const;
        int getLength() const;
        bool isValid() const;
        MsgID getMessageID() const;
        uint8_t getParamHi() const;
        uint8_t getParamLo() const;
        uint16_t getParam() const;

        // Returns true if the byte `b` completes a message.
        bool receive(uint8_t b);
        
      private:
        // Sums the bytes used to compute the Message's checksum.
        uint16_t sum() const;

        enum : uint8_t { START = 0x7E, VERSION = 0xFF, LENGTH = 6, END = 0xEF };
        uint8_t m_buf[10];
        int m_length;
    };

    // Hooks
    virtual void onAck();
    virtual void onCurrentTrack(Device device, uint16_t file_index);
    virtual void onDeviceFileCount(Device device, uint16_t count);
    virtual void onDeviceInserted(Device src);
    virtual void onDeviceRemoved(Device src);
    virtual void onEqualizer(Equalizer eq);
    virtual void onError(uint16_t code);
    virtual void onFinishedFile(Device device, uint16_t file_index);
    virtual void onFirmwareVersion(uint16_t version);
    virtual void onFolderCount(uint16_t count);
    virtual void onFolderTrackCount(uint16_t count);
    virtual void onInitComplete(uint8_t devices);
    virtual void onMessageInvalid();
    virtual void onMessageReceived(const Message &msg);
    virtual void onMessageSent(const uint8_t *buf, int len);
    virtual void onPlaybackSequence(Sequence seq);
    virtual void onStatus(Device device, ModuleState state);
    virtual void onVolume(uint8_t volume);

  private:
    void checkForIncomingMessage();
    void checkForTimeout();
    void receiveMessage(const Message &msg);
    void callHooks(const Message &msg);

  public:  // TEMPORARILY CHANGE VISIBILITY FOR RESEARCH
    void sendMessage(const Message &msg);
    void sendCommand(MsgID msgid, uint16_t param = 0, bool feedback = true);
    void sendQuery(MsgID msgid, uint16_t param = 0);

  private:
    // A State tells us how to handle messages received from the module.
    struct State {
        // The return value is a pointer to the new State.  You can return
        // `this` to remain in the same State, `nullptr` to indicate that
        // the operation is complete and the user can make a new request,
        // or a pointer to another State in order to chain a series of
        // operations together.
        virtual State *onEvent(
          Aidtopia_SerialAudio *module,
          MsgID msgid,
          uint8_t paramHi, uint8_t paramLo
        );
    };

    void setState(State *new_state, uint8_t arg1 = 0, uint8_t arg2 = 0);

    // The intent is to have one static instance of each concrete State in
    // order to minimize RAM and ROM usage.
    struct InitResettingHardware : public State {
      State *onEvent(
        Aidtopia_SerialAudio *module,
        MsgID msgid,
        uint8_t paramHi, uint8_t paramLo
      ) override;
    };
    static InitResettingHardware s_init_resetting_hardware;

    struct InitGettingVersion : public State {
      State *onEvent(
        Aidtopia_SerialAudio *module,
        MsgID msgid,
        uint8_t paramHi, uint8_t paramLo
      ) override;
    };
    static InitGettingVersion s_init_getting_version;

    struct InitCheckingUSBFileCount : public State {
      State *onEvent(
        Aidtopia_SerialAudio *module,
        MsgID msgid,
        uint8_t paramHi, uint8_t paramLo
      ) override;
    };
    static InitCheckingUSBFileCount s_init_checking_usb_file_count;
    
    struct InitCheckingSDFileCount : public State {
      State *onEvent(
        Aidtopia_SerialAudio *module,
        MsgID msgid,
        uint8_t paramHi, uint8_t paramLo
      ) override;
    };
    static InitCheckingSDFileCount s_init_checking_sd_file_count;

    struct InitSelectingUSB : public State {
      State *onEvent(
        Aidtopia_SerialAudio *module,
        MsgID msgid,
        uint8_t paramHi, uint8_t paramLo
      ) override;
    };
    static InitSelectingUSB s_init_selecting_usb;

    struct InitSelectingSD : public State {
      State *onEvent(
        Aidtopia_SerialAudio *module,
        MsgID msgid,
        uint8_t paramHi, uint8_t paramLo
      ) override;
    };
    static InitSelectingSD s_init_selecting_sd;

    struct InitCheckingFolderCount : public State {
      State *onEvent(
        Aidtopia_SerialAudio *module,
        MsgID msgid,
        uint8_t paramHi, uint8_t paramLo
      ) override;
    };
    static InitCheckingFolderCount s_init_checking_folder_count;

    struct InitStartPlaying : public State {
      State *onEvent(
        Aidtopia_SerialAudio *module,
        MsgID msgid,
        uint8_t paramHi, uint8_t paramLo
      ) override;
    };
    static InitStartPlaying s_init_start_playing;

    // TODO:  Guess the module type.
    enum Module { MOD_UNKNOWN, MOD_CATALEX, MOD_DFPLAYERMINI };

    Stream  *m_stream;
    Message  m_in;
    Message  m_out;
    State   *m_state;
    Timeout<MillisClock> m_timeout;
};

class Aidtopia_SerialAudioWithLogging : public Aidtopia_SerialAudio {
  public:
    // The regular logging level includes asynchronous events and responses to
    // queries.  The detailed level includes the bytes of all incoming and
    // outgoing messages as well as the "ACK"s from the module.
    void logDetail(bool detailed = true) { m_detailed = detailed; }

  protected:
    void onAck() override;
    void onCurrentTrack(Device device, uint16_t file_index) override;
    void onDeviceFileCount(Device device, uint16_t count) override;
    void onDeviceInserted(Device src) override;
    void onDeviceRemoved(Device src) override;
    void onEqualizer(Equalizer eq) override;
    void onError(uint16_t code) override;
    void onFinishedFile(Device device, uint16_t file_index) override;
    void onFirmwareVersion(uint16_t version) override;
    void onFolderCount(uint16_t count) override;
    void onFolderTrackCount(uint16_t count) override;
    void onInitComplete(uint8_t devices) override;
    void onMessageInvalid() override;
    void onMessageReceived(const Message &msg) override;
    void onMessageSent(const uint8_t *buf, int len) override;
    void onPlaybackSequence(Sequence seq) override;
    void onStatus(Device device, ModuleState state) override;
    void onVolume(uint8_t volume) override;

    void printDeviceName(Device src);
    void printEqualizerName(Equalizer eq);
    void printModuleStateName(ModuleState state);
    void printSequenceName(Sequence seq);

  private:
    bool m_detailed = false;
};

#endif
