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
            Serial.print(F("< "));
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


void SerialAudioManager::update() {
    Message msg;
    if (m_core.update(&msg)) {
        Serial.print(F("received: "));
        Serial.print(static_cast<int>(msg.getID()), HEX);
        Serial.print(' ');
        Serial.println(msg.getParam(), HEX);
        if (!isAsyncNotification(msg.getID())) {
            m_timeout.cancel();
        }
        onEvent(msg);
    }
    if (m_timeout.expired()) {
        onEvent(Message{Message::Error::TIMEDOUT});
    }
}

void SerialAudioManager::playFile(uint16_t index) {
    enqueue(Message{Message::ID::PLAYFILE, index});
}

void SerialAudioManager::selectSource(Device source) {
    auto const paramLo = static_cast<uint8_t>(source);
    enqueue(Message{Message::ID::SELECTSOURCE, 0, paramLo});
}

void SerialAudioManager::setVolume(uint8_t volume) {
    volume = min(volume, 30);
    enqueue(Message{Message::ID::SETVOLUME, volume});
}

void SerialAudioManager::reset() {
    clearQueue();
    enqueue(Message{Message::ID::RESET});
}

void SerialAudioManager::sendMessage(Message const &msg) {
    auto const msgid = msg.getID();
    if (msgid == Message::ID::RESET) {
        m_core.send(msg, Feedback::FEEDBACK);
        m_timeout.set(3000);
        m_expected = Message::ID::INITCOMPLETE;
        m_state = State::INITPENDING;
    } else if (isQuery(msgid)) {
        m_core.send(msg, Feedback::NO_FEEDBACK);
        m_timeout.set(100);
        m_expected = msgid;
        m_state = State::RESPONSEPENDING;
    } else {
        m_core.send(msg, Feedback::FEEDBACK);
        m_timeout.set(33);
        m_expected = Message::ID::ACK;
        m_state = State::RESPONSEPENDING;
    }
}

void SerialAudioManager::enqueue(Message const &msg) {
    m_queue[m_tail++] = msg;
    if (m_tail == m_head) Serial.println(F("Queue overflowed!"));
    if (m_state == State::READY) dispatch();
}

void SerialAudioManager::clearQueue() {
    m_head = m_tail = 0;
}

void SerialAudioManager::dispatch() {
    if (m_head == m_tail) return;

    Serial.println(F("Dispatching next queued message."));
    sendMessage(m_queue[m_head++]);
}


void SerialAudioManager::onEvent(Message const &msg) {
    using ID = Message::ID;

    if (isAsyncNotification(msg.getID())) {
        Serial.println(F("Asynchronous notification received."));
        return;
    }
    
    if (msg.getID() == ID::INITCOMPLETE && m_state != State::INITPENDING) {
        Serial.println(F("Audio device unexpectedly reset!"));
    }

    switch (m_state) {
        case State::INITPENDING:
            Serial.print(F("INITPENDING: "));
            if (msg.getID() == ID::ACK) {
                Serial.println(F("reset command ack'ed"));
            } else if (msg.getID() == ID::INITCOMPLETE) {
                Serial.println(F("completed"));
                ready();
            } else if (isTimeout(msg)) {
                Serial.println(F("Timed out"));
                // Maybe the module is initialized and just quietly waiting for
                // us.  We'll check its status.
                sendMessage(Message{ID::STATUS});
            } else if (isError(msg.getID())) {
                Serial.println(F("error"));
                m_state = State::STUCK;
            } else {
                Serial.println(F("unexpected event"));
            }
            break;
        case State::RECOVERING:
            break;
        case State::RESPONSEPENDING:
            Serial.print(F("RESPONSEPENDING: "));
            if (msg.getID() == m_expected) {
                Serial.println(F("response received"));
                ready();
            } else if (isError(msg.getID())) {
                Serial.println(F("error"));
                m_timeout.cancel();
                m_state = State::STUCK;
            } else {
                Serial.println(F("unexpected event"));
            }
            break;
        case State::READY:
            Serial.println(F("READY: "));
            Serial.println(F("unexpected event"));
            break;
        case State::STUCK:
            Serial.println(F("STUCK: "));
            Serial.println(F("unexpected event"));
            break;
    }
}

void SerialAudioManager::ready() {
    m_timeout.cancel();
    m_expected = Message::ID::NONE;
    m_state = State::READY;
    dispatch();
}

}
