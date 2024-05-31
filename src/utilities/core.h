#ifndef AIDTOPIA_SERIALAUDIOCORE_H
#define AIDTOPIA_SERIALAUDIOCORE_H

#include <utilities/message.h>
#include <utilities/messagebuffer.h>
#include <utilities/timeout.h>

namespace aidtopia {

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

class SerialAudioManager {
    public:
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
        
        void update();

        // Note that the devices can be OR'd to create a set.  Some players
        // support just SDCARD. Others also support USB.
        enum class Device : uint8_t {
            NONE        = 0x00,
            USB         = 0x01,  // a USB drive
            SDCARD      = 0x02,  // Micro SD card, aka, a TF (True Flash) card
            FLASH       = 0x04   // onboard Flash memory (via SPI interface)
        };
        void selectSource(Device source);

        void playFile(uint16_t index);
        void setVolume(uint8_t volume);
        void reset();

    // These will become private.
        void sendMessage(Message const &msg);
        void enqueue(Message const &msg);

    private:
        void clearQueue();
        void dispatch();
        void onEvent(Message const &msg);
        void ready();

        SerialAudioCore         m_core;
        Timeout<MillisClock>    m_timeout;
        enum class State {
            INITPENDING,
            RECOVERING,
            READY,
            RESPONSEPENDING,
            STUCK
        }                       m_state = State::INITPENDING;
        Message::ID             m_expected = Message::ID::NONE;
        Message                 m_queue[8];
        uint8_t                 m_head : 4;
        uint8_t                 m_tail : 4;
};

}

#endif
