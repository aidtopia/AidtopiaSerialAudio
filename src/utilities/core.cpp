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


bool SerialAudioCore::update(Message *msg) {
    if (checkForIncomingMessage()) {
        if (msg != nullptr) {
            *msg = Message{static_cast<Message::ID>(m_in.getID()), m_in.getData()};
        }
        return true;
    }
    return false;
}


void SerialAudioManager::update() {
    Message msg;
    if (m_core.update(&msg)) {
        Serial.print("received: ");
        Serial.print(static_cast<int>(msg.getID()), HEX);
        Serial.print(' ');
        Serial.println(msg.getParam(), HEX);
        if (!isAsyncEvent(msg.getID())) {
            m_timeout.cancel();
        }
        onEvent(msg);
    }
    if (m_timeout.expired()) {
        Serial.println("timeout expired");
        onEvent(Message{Message::Error::TIMEDOUT});
    }
}

void SerialAudioManager::sendMessage(Message const &msg, Feedback feedback) {
    m_core.send(msg, feedback);
}

void SerialAudioManager::sendCommand(
    Message::ID msgid,
    uint16_t param
) {
    sendMessage(Message(msgid, param), Feedback::FEEDBACK);
    m_timeout.set(10);
}

void SerialAudioManager::sendQuery(Message::ID msgid, uint16_t param) {
    // Since queries naturally have a response, we won't ask for feedback.
    sendMessage(Message{msgid, param}, Feedback::NO_FEEDBACK);
    m_timeout.set(20);
}

void SerialAudioManager::onEvent(Message const &msg) {
    using ID = Message::ID;

    switch (m_state) {
        case State::WAITFORINIT:
            // Initialization time varies depending on how many devices are
            // attached and how many files and folders they contain.  We're
            // not supposed to send messages to the device during initializaton.
            // So we'll wait a while for any sign of life from the module.
            if (msg.getID() == ID::INITCOMPLETE) {
                Serial.println(F("Initialization completed!"));
                m_timeout.cancel();
                m_state = State::READY;
            } else if (isTimeout(msg)) {
                Serial.println(F("Timed out waiting for initialization."));
                // Maybe the module is initialized and just quietly
                // waiting for us.  We'll check its status.
                sendQuery(ID::STATUS);
                m_state = State::WAITFORSTATUS;
            } else if (isError(msg.getID())) {
                Serial.println(F("Error occurred while waiting for initialization."));
                m_state = State::STUCK;
            } else if (isAsyncEvent(msg.getID())) {
                Serial.println(F("Async notification while waiting for init."));
                // An asynchronous notification suggests the module is
                // already alive and kicking.
                m_timeout.cancel();
                m_state = State::READY;
            }
            break;
        case State::WAITFORSTATUS:
            if (msg.getID() == ID::STATUS) {
                Serial.println(F("Response from status query."));
                m_timeout.cancel();
                m_state = State::READY;
            } else if (isTimeout(msg)) {
                Serial.println(F("Timed out waiting for response to status query."));
                m_timeout.cancel();
                m_state = State::STUCK;
            } else if (isError(msg.getID())) {
                Serial.println(F("Error occured while waiting for status."));
                m_timeout.cancel();
                m_state = State::STUCK;
            } else if (isAsyncEvent(msg.getID())) {
                Serial.println(F("Async notificiation while waiting for status."));
            }
            break;
        case State::READY: break;
        case State::STUCK: break;
    }
}

}
