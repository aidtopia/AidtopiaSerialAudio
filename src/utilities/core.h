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

        // Returns true if a complete and valid message has been received.
        bool checkForIncomingMessage();

        void    send(Message const &msg, Feedback feedback);
        Message receive() const;

    private:
        Stream        *m_stream;
        MessageBuffer  m_in;
        MessageBuffer  m_out;
};

// SerialAudioTransaction uses SerialAudioCore to provide reliable exchange of
// commands, queries, and responses.
class SerialAudioTransaction {
    public:
        template <typename SerialType>
        void begin(SerialType &stream) {
            m_core.begin(stream, 9600);
        }

        void update();

        void sendCommand(
            Message::ID msgid,
            uint16_t    param = 0,
            Feedback    feedback = Feedback::FEEDBACK
        );
        void sendQuery(Message::ID msgid, uint16_t param = 0);

    private:
        void checkForTimeout();

        SerialAudioCore m_core;
        Timeout<MillisClock> m_timeout;
};

}

#endif
