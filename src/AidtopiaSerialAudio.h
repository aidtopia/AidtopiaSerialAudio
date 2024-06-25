// AidtopiaSerialAudio
// Adrian McCarthy 2018-

// A library that works with various serial audio modules,
// like DFPlayer Mini, Catalex, etc.

#ifndef AIDTOPIASERIALAUDIO_H
#define AIDTOPIASERIALAUDIO_H

#include "utilities/core.h"
#include "utilities/queue.h"
#include "utilities/message.h"
#include "utilities/timeout.h"

namespace aidtopia {

class SerialAudio {
    public:
        // From the client's point of view, these enumerations are just
        // arbitrary constants.  Do not change any explicitly listed values.
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
        
        // Represents a set of Device.
        class Devices {
            public:
                Devices();
                Devices(Device device);  // implicit conversion
                explicit Devices(uint8_t bitmask);
                Devices(Devices const &rhs) = default;
                Devices &operator=(Devices const &rhs) = default;
                Devices &operator|=(Devices const &rhs);
                bool empty() const;
                bool has(Device device) const;
                bool hasAny(Devices devices) const;
                void clear();
                void insert(Device device);
                void remove(Device device);
            private:
                uint8_t m_bitmask;
        };
        
        enum class ModuleState : uint8_t {
            STOPPED     = 0x00,
            PLAYING     = 0x01,
            PAUSED      = 0x02,
            ALT_STOPPED = 0x03,  // Some modules supposedly use 3 instead of 0.
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
            USBFILECOUNT        = static_cast<uint8_t>(Message::ID::USBFILECOUNT),
            SDFILECOUNT         = static_cast<uint8_t>(Message::ID::SDFILECOUNT),
            FLASHFILECOUNT      = static_cast<uint8_t>(Message::ID::FLASHFILECOUNT),
            EQPROFILE           = static_cast<uint8_t>(Message::ID::EQPROFILE),
            FIRMWAREVERSION     = static_cast<uint8_t>(Message::ID::FIRMWAREVERSION),
            FOLDERCOUNT         = static_cast<uint8_t>(Message::ID::FOLDERCOUNT),
            FOLDERFILECOUNT,   // I'm not sure how to implement this right now
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
            // `begin` is usually called shortly after power-up, so we'll assume
            // the audio module has just powered up as well.
            onPowerUp();
        }
        
        // Client should call `update` frequently, typically each pass through
        // the `loop` function.
        //
        // Returns true if ready for another command or query.
        bool update();
        
        // To receive a callback for a query response, an asynchronous
        // notification, or an error, call `update` with an instance of a class
        // derived from Hooks that overrides the callback method(s) of interest.
        class Hooks {
            public:
                using Device = SerialAudio::Device;
                using Devices = SerialAudio::Devices;
                using DeviceChange = SerialAudio::DeviceChange;
                using EqProfile = SerialAudio::EqProfile;
                using Error = SerialAudio::Error;
                using ModuleState = SerialAudio::ModuleState;
                using Parameter = SerialAudio::Parameter;
                using Sequence = SerialAudio::Sequence;

                virtual ~Hooks();

                // NVI:  Non-virtual interface forwards events to the private
                // virtual methods.
                void handleError(Error code);
                void handleQueryResponse(Parameter param, uint16_t value);
                void handleDeviceChange(Device src, DeviceChange change);
                void handleFinishedFile(Device device, uint16_t index);
                void handleInitComplete(Devices devices);

            private:
                // Provide overrides for any or all of these methods.
                virtual void onError(Error code);
                virtual void onQueryResponse(Parameter param, uint16_t value);

                // Asynchronous Notifications
                virtual void onDeviceChange(Device src, DeviceChange change);
                virtual void onFinishedFile(Device device, uint16_t index);
                virtual void onInitComplete(Devices devices);

            private:
                // For filtering duplicate asynchronous notifications.
                Device m_deviceLastFinished;
                uint16_t m_indexLastFinished;
        };

        bool update(Hooks *hooks);

        // (Arduino guidelines discourage libraries from requiring pointers in
        // the public interface, so we'll let the client pass in hooks by
        // reference.
        bool update(Hooks &hooks);

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

        // Methods that use folder numbers refer to the folder name's numeric
        // prefix.
        void queryFolderCount();
        void queryFolderFileCount(uint16_t folder);
        void loopFolder(uint16_t folder);

        // Methods with "Track" refer to sounds files by the file name's prefix.
        void playTrack(uint16_t track);  // from "MP3" folder
        void playTrack(uint16_t folder, uint16_t track);

        // Control whether the currently playing track should loop.
        void loopCurrentTrack();
        void stopLoopingCurrentTrack();

        void queryPlaybackSequence();

        void stop();
        void pause();
        void unpause();

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

    private:
        // The state keeps track of the last message sent and a checklist of
        // events to expect.
        class State {
            public:
                using Flag = uint8_t;
                enum : uint8_t {
                    NONE            = 0x00,
                    EXPECT_ACK      = 0x01,
                    EXPECT_ACK2     = 0x02,
                    EXPECT_RESPONSE = 0x04,
                    DELAY           = 0x08,
                    RESERVED4       = 0x10,
                    RESERVED3       = 0x20,
                    RESERVED2       = 0x40,
                    UNINITIALIZED   = 0x80,
                    
                    ALL_FLAGS       = 0xFF
                };

                State() : m_sent(Message::ID::NONE), m_flags(UNINITIALIZED) {}
                explicit State(Message::ID msgid, Flag flags = NONE) :
                    m_sent(msgid), m_flags(flags) {}

                Message::ID sent() const { return m_sent; }
                bool has(Flag flag) const { return (m_flags & flag) == flag; }
                bool hasAny(Flag flags) const { return (m_flags & flags) != 0; }
                void set(Flag flags) { m_flags |= flags; }
                void clear(Flag flags) { m_flags &= ~flags; }
                bool testAndClear(Flag flag) {
                    if (has(flag)) { clear(flag); return true; }
                    return false;
                }

                bool ready() const { return m_flags == NONE; }
                bool poweringUp() const {
                    return m_sent == Message::ID::NONE &&
                           m_flags == UNINITIALIZED;
                }

            private:
                Message::ID m_sent;
                uint8_t     m_flags;
        };

        struct Command {
            State state;
            uint16_t param;
        };

        bool enqueue(Message::ID msgid, State::Flag flags, uint16_t data = 0);
        void onEvent(Message const &msg, Hooks *hooks);
        void handleEvent(Message const &msg, Hooks *hooks);
        bool discoveryContinues();
        void dispatch();
        void dispatch(Message::ID msgid, State::Flag flags, uint16_t data = 0);
        void dispatch(Command const &cmd);
        void onPowerUp();

        SerialAudioCore         m_core;
        Queue<Command, 4>       m_queue;
        State                   m_state;
        Timeout<MillisClock>    m_timeout;
        // These are used by the state machine discover available devices.
        Devices                 m_available;
        Devices                 m_tocheck;
};

SerialAudio::Devices operator|(SerialAudio::Device d1, SerialAudio::Device d2);
SerialAudio::Devices operator|(SerialAudio::Device d1, SerialAudio::Devices d2);
SerialAudio::Devices operator|(SerialAudio::Devices d1, SerialAudio::Device d2);
SerialAudio::Devices operator|(SerialAudio::Devices d1, SerialAudio::Devices d2);

}

// Make the class available in the global namespace.
using AidtopiaSerialAudio = aidtopia::SerialAudio;

#endif
