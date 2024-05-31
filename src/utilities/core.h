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
            
            // begin is usually called right after power-up, so we'll assume the
            // audio module also just powered up.
            m_state = State::WAITFORINIT;
            m_timeout.set(5000);
        }
        
        void update();

    // These will become private.
        void sendCommand(Message::ID msgid, uint16_t param = 0);
        void sendQuery(Message::ID msgid, uint16_t param = 0);

    private:
        void onEvent(Message const &msg);

        SerialAudioCore         m_core;
        Timeout<MillisClock>    m_timeout;
        enum class State {
            WAITFORINIT,
            WAITFORSTATUS,
            READY,
            STUCK
        }                       m_state = State::WAITFORINIT;
};

}

#endif
