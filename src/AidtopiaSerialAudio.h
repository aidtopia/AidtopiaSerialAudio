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
        // arbitrary constants, but don't change any explicitly listed values.
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

        class Hooks {
            public:
                using Device = SerialAudio::Device;
                using DeviceChange = SerialAudio::DeviceChange;
                using EqProfile = SerialAudio::EqProfile;
                using Parameter = SerialAudio::Parameter;

                virtual ~Hooks();

                virtual void onError(Message::Error code);

                // Asynchronous Notifications
                virtual void onDeviceChange(Device src, DeviceChange change);
                virtual void onFinishedFile(Device device, uint16_t file_index);
                virtual void onInitComplete(uint8_t devices);

                // Query Responses
#if 1
                virtual void onQueryResponse(Parameter param, uint16_t value);
#else                
                // Breaking each of these out makes for a nicer callback API,
                // but it also makes for a much larger v-table in RAM.
                virtual void onCurrentFile(Device device, uint16_t file_index);
                virtual void onDeviceFileCount(Device device, uint16_t count);
                virtual void onEqualizer(EqProfile eq);
                virtual void onFirmwareVersion(uint16_t version);
                virtual void onFolderCount(uint16_t count);
                virtual void onFolderTrackCount(uint16_t count);
                virtual void onPlaybackSequence(Sequence seq);
                virtual void onStatus(Device device, ModuleState state);
                virtual void onVolume(uint8_t volume);
#endif
        };
};

class SerialAudioManager : public SerialAudio {
    public:
        using SerialAudio::Device;
        using SerialAudio::ModuleState;
        using SerialAudio::EqProfile;
        using SerialAudio::Sequence;
        using SerialAudio::Hooks;
    
        template <typename SerialType>
        void begin(SerialType &stream) {
            m_core.begin(stream);

            // Arduino IDE compiler isn't up-to-date enough to allow us to put
            // initializers for bitfield members in the class definition.  So
            // we'll init the queue pointers here.
            clearQueue();

            // begin is usually called right after power-up, so we'll assume the
            // audio module also just powered up.  If it's already powered up,
            // then, once the timeout expires, the state machine will do what it
            // needs to do.
            m_state = State::INITPENDING;
            m_timeout.set(3000);
        }
        
        void update(Hooks *hooks = nullptr);

        void reset();
        void queryFileCount(Device device);
        void selectSource(Device source);
        void queryStatus();
        void queryFirmwareVersion();

        void setVolume(uint8_t volume);
        void increaseVolume();
        void decreaseVolume();
        void queryVolume();

        void setEqProfile(EqProfile eq);
        void queryEqProfile();

        // Commands with "File" access sounds by their file system indexes.
        void playFile(uint16_t index);
        void playNextFile();
        void playPreviousFile();
        void loopFile(uint16_t index);
        void loopAllFiles();
        void playFilesInRandomOrder();

        // Commands with "Track" access sounds by the file name's prefix.
        void queryFolderCount();
        void playTrack(uint16_t track);  // from "MP3" folder
        void playTrack(uint16_t folder, uint16_t track);

        void queryCurrentFile(Device device);
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

#if 0
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

#endif
