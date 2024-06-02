#ifndef AIDTOPIA_SERIALAUDIOCORE_H
#define AIDTOPIA_SERIALAUDIOCORE_H

#include <utilities/message.h>
#include <utilities/messagebuffer.h>
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

                // For spying or debugging
                virtual void onMessageReceived(const Message &msg);
        };
};

// SerialAudioCore handles message passing over a serial line.
class SerialAudioCore {
    public:
        template <typename SerialType>
        void begin(SerialType &serial, int baudrate = 9600) {
            serial.begin(baudrate);
            m_stream = &serial;
        }

        // If a new message is available, update copies it to msg and returns
        // true.  Otherwise returns false.
        bool update(Message *msg = nullptr);

        void send(Message const &msg, Feedback feedback);

    private:
        // Returns true if a complete and valid message has been received.
        bool checkForIncomingMessage();

        Stream        *m_stream;
        MessageBuffer  m_in;
        MessageBuffer  m_out;
};

}

#endif
