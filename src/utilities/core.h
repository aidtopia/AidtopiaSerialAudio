#ifndef AIDTOPIA_SERIALAUDIOCORE_H
#define AIDTOPIA_SERIALAUDIOCORE_H

#include "utilities/message.h"
#include "utilities/messagebuffer.h"

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
};

}

#endif
