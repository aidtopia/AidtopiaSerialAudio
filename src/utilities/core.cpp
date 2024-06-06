#include <Arduino.h>
#include <utilities/core.h>

namespace aidtopia {

static void dump(MessageBuffer const &msgbuf) {
    auto const buf = msgbuf.getBytes();
    auto const len = msgbuf.getLength();
    for (int i = 0; i < len; ++i) {
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}

bool SerialAudioCore::checkForIncomingMessage() {
    while (m_stream->available() > 0) {
        if (m_in.receive(m_stream->read())) {
            Serial.print(F("< ")); dump(m_in);
            if (m_in.isValid()) return true;
        }
    }
    return false;
}

void SerialAudioCore::send(Message const &msg, Feedback feedback) {
    m_out.set(static_cast<uint8_t>(msg.getID()), msg.getParam(),
              feedback == Feedback::FEEDBACK);
    const auto buf = m_out.getBytes();
    const auto len = m_out.getLength();
    m_stream->write(buf, len);
    Serial.print(F("> ")); dump(m_out);
}

bool SerialAudioCore::update(Message *msg) {
    if (checkForIncomingMessage()) {
        if (msg != nullptr) {
            *msg = Message{static_cast<Message::ID>(m_in.getID()), m_in.getData()};
        }
        return true;
    }
    return false;
}

}
