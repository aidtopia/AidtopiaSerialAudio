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
            dump(m_in);
            if (m_in.isValid()) return true;
        }
    }
    return false;
}

void SerialAudioCore::send(Message const &msg, Feedback feedback) {
    m_out.set(static_cast<uint8_t>(msg.getID()), msg.getParam(), feedback);
    const auto buf = m_out.getBytes();
    const auto len = m_out.getLength();
    m_stream->write(buf, len);
    dump(m_out);
}

Message SerialAudioCore::receive() const {
    return Message(static_cast<Message::ID>(m_in.getID()), m_in.getData());
}


void SerialAudioTransaction::update() {
    if (m_core.checkForIncomingMessage()) {
        auto const msg = m_core.receive();
        if (static_cast<uint8_t>(msg.getID()) >= static_cast<uint8_t>(Message::ID::INITCOMPLETE)) {
            m_timeout.cancel();
        }
        //callHooks(msg);
    }
    checkForTimeout();
}

void SerialAudioTransaction::sendCommand(
    Message::ID msgid,
    uint16_t param,
    Feedback feedback
) {
    m_core.send(Message(msgid, param), feedback);
    m_timeout.set(200);
}

void SerialAudioTransaction::sendQuery(Message::ID msgid, uint16_t param) {
    // Since queries naturally have a response, we won't ask for feedback.
    sendCommand(msgid, param, Feedback::NO_FEEDBACK);
}

void SerialAudioTransaction::checkForTimeout() {
    if (m_timeout.expired()) {
        m_timeout.cancel();
        // onError(EC_TIMEDOUT);
    }
}

}
