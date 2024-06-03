// AidtopiaSerialAudio
// Adrian McCarthy 2018-

// A library that works with various serial audio modules,
// like DFPlayer Mini, Catalex, etc.

#ifndef AIDTOPIASERIALAUDIO_H
#define AIDTOPIASERIALAUDIO_H

#include <utilities/core.h>
#include <utilities/timeout.h>

namespace aidtopia {

class SerialAudio {
    public:
        // From the client's point of view, these enumerations are just
        // arbitrary constants, but do not change any explicitly listed values.
        // They correspond to ones used in the message protocol for the
        // convenience of the implementation.

        enum class Device : uint8_t {
            NONE        = 0x00,
            USB         = 0x01,  // a USB drive
            SDCARD      = 0x02,  // Micro SD card, aka, a TF (True Flash) card
            FLASH       = 0x04,  // onboard Flash memory
            AUX         = 0x08,  // connection to a PC over USB
            SLEEP       = 0x10   // not selectable, but might be status results
        };
        
        enum class ModuleState : uint8_t {
            STOPPED     = 0x00,
            PLAYING     = 0x01,
            PAUSED      = 0x02,

            ALT_STOPPED = 0x03,  // Some modules supposedly use 3 instead of 0.
                                 // Will be mapped back to STOPPED for Hooks.

            ASLEEP      = 0xFF   // Inferred from other INITCOMPLETE params
        };
        
        enum class Sequence : uint8_t {
            LOOPALL     = 0x00,
            LOOPFOLDER  = 0x01,
            LOOPTRACK   = 0x02,
            RANDOM      = 0x03,
            SINGLE      = 0x04
        };
        
        enum class EqProfile : uint8_t {
            NORMAL      = 0x00,
            POP         = 0x01,
            ROCK        = 0x02,
            JAZZ        = 0x03,
            CLASSICAL   = 0x04,
            BASS        = 0x05
        };

        enum class DeviceChange { REMOVED, INSERTED };
        
        // These are the module parameters that can be queried.
        enum class Parameter {
            CURRENTFILE,        // I'm not sure how to implement this right now
            DEVICEFILECOUNT,    // I'm not sure how to implement this right now
            EQPROFILE           = static_cast<uint8_t>(Message::ID::EQPROFILE),
            FIRMWAREVERSION     = static_cast<uint8_t>(Message::ID::FIRMWAREVERSION),
            FOLDERCOUNT,        // I'm not sure how to implement this right now
            FOLDERTRACKCOUNT,   // I'm not sure how to implement this right now
            PLAYBACKSEQUENCE    = static_cast<uint8_t>(Message::ID::PLAYBACKSEQUENCE),
            STATUS              = static_cast<uint8_t>(Message::ID::STATUS),
            VOLUME              = static_cast<uint8_t>(Message::ID::VOLUME)
        };

        enum class Error : uint16_t {
          UNSUPPORTED        = 0x00,  // MsgID used is not supported
          NOSOURCES          = 0x01,  // module busy or no sources installed
          SLEEPING           = 0x02,  // module is sleeping
          SERIALERROR        = 0x03,  // serial communication error
          BADCHECKSUM        = 0x04,  // module received bad checksum
          FILEOUTOFRANGE     = 0x05,  // this is the file index
          TRACKNOTFOUND      = 0x06,  // couldn't find track by numeric prefix
          INSERTIONERROR     = 0x07,  // couldn't start ADVERT track
          SDCARDERROR        = 0x08,  // unformatted card??
          ENTEREDSLEEP       = 0x0A,  // entered sleep mode??

          // And reserving one for our state machine
          TIMEDOUT           = 0x0100
        };

        // Client must call `begin` before anything else, typically in `setup`.
        // `SerialType` should be an instance of `HardwareSerial` or
        // `SoftwareSerial`.
        template <typename SerialType>
        void begin(SerialType &stream) {
            m_core.begin(stream);
            clearQueue();

            // `begin` is usually called right after power-up, so we'll assume
            // the audio module has just powered up as well.
            m_state = State::INITPENDING;
            m_timeout.set(3000);
        }
        
        // Client should call `update` frequently, typically each pass through
        // the `loop` function.
        //
        // To receive a callback for a query response, an asynchronous
        // notification, or an error, provide a pointer to a class derived from
        // `SerialAudio::Hooks` the overrides the callback method(s) of
        // interest.
        class Hooks;
        void update(Hooks *hooks = nullptr);

        class Hooks {
            public:
                using Device = SerialAudio::Device;
                using DeviceChange = SerialAudio::DeviceChange;
                using Parameter = SerialAudio::Parameter;
                using Error = SerialAudio::Error;

                virtual ~Hooks();

                virtual void onError(Error code);
                virtual void onQueryResponse(Parameter param, uint16_t value);

                // Asynchronous Notifications
                virtual void onDeviceChange(Device src, DeviceChange change);
                virtual void onFinishedFile(Device device, uint16_t index);
                virtual void onInitComplete(uint8_t devices);
        };

        // These are the commands and queries the client can use to control the
        // audio module.
        //
        // All commands and queries are placed into a queue to be executed in
        // the order the client requested.  (The one exception is `reset`, which
        // first clears the queue.)
        //
        // The command at the head of the queue is executed as soon as the
        // module is ready, so the queue cannot be used as a playlist.  If two
        // play commands are in the queue, the second will be executed
        // immediately after the first.  To play a list of sounds, play the
        // first, and provide an onFileFinished hook that plays the next song
        // in the list.  Another option is to place the sounds in a numbered
        // folder and use the `loopFolder` command.
        void reset();
        void queryFirmwareVersion();
        void queryFileCount(Device device);
        void selectSource(Device source);
        void queryStatus();

        // Volume ranges from 0 to 30.
        void setVolume(uint8_t volume);
        void increaseVolume();
        void decreaseVolume();
        void queryVolume();

        void setEqProfile(EqProfile eq);
        void queryEqProfile();

        // Methods with "File" refer to sound files by their file system index.
        void playFile(uint16_t index);
        void playNextFile();
        void playPreviousFile();
        void loopFile(uint16_t index);
        void loopAllFiles();
        void playFilesInRandomOrder();
        void queryCurrentFile(Device device);

        // Methods with "Track" refer to sounds files by the file name's prefix.
        void queryFolderCount();
        void playTrack(uint16_t track);  // from "MP3" folder
        void playTrack(uint16_t folder, uint16_t track);
        void loopFolder(uint16_t folder);

        void stop();
        void pause();
        void unpause();

        void queryPlaybackSequence();

        // You can interrupt a track with an "advertisement" track from a folder
        // named "ADVERT" or "ADVERTn" for n in [1..9].  When the advertisement
        // track completes, the interrupted track will resume.  The module does
        // not indicate when the advertisement track completes (except for a
        // brief blip on the BUSY line if the module has one).  Inserting an
        // advertisement will fail if nothing is currently being played (or even
        // if it's paused).  Using stop during an advertisement stops all
        // playback.  Using stopAdvert stops only the advertisement, allowing
        // the interrupted track to resume.
        void insertAdvert(uint16_t track);
        void insertAdvert(uint8_t folder, uint8_t track);
        void stopAdvert();

#if 0  // TBD
    void sleep();
    void wake();
    void disableDACs();
    void enableDACs();
#endif

    // These will become private.
        void sendMessage(Message const &msg);
        void enqueue(Message const &msg);

    private:
        void clearQueue();
        void dispatch();
        void onEvent(Message const &msg, Hooks *hooks);
        void ready();

        SerialAudioCore         m_core;
        Timeout<MillisClock>    m_timeout;
        enum class State {
            READY,
            ACKPENDING,
            RESPONSEPENDING,
            INITPENDING,
            RECOVERING,
            SOURCEPENDINGACK,
            SOURCEPENDINGDELAY,
            STUCK
        }                       m_state = State::INITPENDING;
        Message::ID             m_lastRequest = Message::ID::NONE;
        Message                 m_queue[8];
        uint8_t                 m_head : 3;
        uint8_t                 m_tail : 3;
        uint8_t                 m_reserved : 2;
};

}

using AidtopiaSerialAudio = aidtopia::SerialAudio;

#endif
