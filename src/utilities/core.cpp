#include <Arduino.h>
#include <utilities/core.h>

namespace aidtopia {

static void dump(uint8_t const *buf, uint8_t len) {
    if (len == 0) return;
    Serial.print(buf[0], HEX);
    for (int i = 1; i < len; ++i) {
        Serial.print(' ');
        Serial.print(buf[i], HEX);
    }
    Serial.println();
}

bool SerialAudioCore::checkForIncomingMessage() {
    while (m_stream->available() > 0) {
        if (m_in.receive(m_stream->read())) {
            Serial.print(F("< ")); dump(m_in.getBytes(), m_in.getLength());
            if (m_in.isValid()) return true;
        }
    }
    return false;
}

void SerialAudioCore::send(Message const &msg, Feedback feedback) {
    auto const out =
        MessageBuffer(static_cast<uint8_t>(msg.getID()), msg.getParam(),
                      feedback == Feedback::FEEDBACK);
    const auto buf = out.getBytes();
    const auto len = out.getLength();
    m_stream->write(buf, len);
    Serial.print(F("> ")); dump(buf, len);
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
