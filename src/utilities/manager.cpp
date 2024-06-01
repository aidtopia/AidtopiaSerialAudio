#include <Arduino.h>
#include <utilities/manager.h>

namespace aidtopia {

void SerialAudioManager::update(Hooks *hooks) {
    Message msg;
    if (m_core.update(&msg)) {
        if (hooks) hooks->onMessageReceived(msg);
        onEvent(msg, hooks);
    }
    if (m_timeout.expired()) {
        onEvent(Message{Message::Error::TIMEDOUT}, hooks);
    }
}

void SerialAudioManager::reset() {
    clearQueue();
    enqueue(Message{Message::ID::RESET});
}

void SerialAudioManager::queryFileCount(Device device) {
    switch (device) {
        case Device::USB:    enqueue(Message{Message::ID::USBFILECOUNT});   break;
        case Device::SDCARD: enqueue(Message{Message::ID::SDFILECOUNT});    break;
        case Device::FLASH:  enqueue(Message{Message::ID::FLASHFILECOUNT}); break;
        default: break;
    }
}

void SerialAudioManager::selectSource(Device source) {
    auto const paramLo = static_cast<uint8_t>(source);
    enqueue(Message{Message::ID::SELECTSOURCE, 0, paramLo});
}

void SerialAudioManager::queryStatus() {
    enqueue(Message{Message::ID::STATUS});
}

void SerialAudioManager::queryFirmwareVersion() {
    enqueue(Message{Message::ID::FIRMWAREVERSION});
}

void SerialAudioManager::setVolume(uint8_t volume) {
    volume = min(volume, 30);
    enqueue(Message{Message::ID::SETVOLUME, volume});
}

void SerialAudioManager::increaseVolume() {
    enqueue(Message{Message::ID::VOLUMEUP});
}

void SerialAudioManager::decreaseVolume() {
    enqueue(Message{Message::ID::VOLUMEDOWN});
}

void SerialAudioManager::queryVolume() {
    enqueue(Message{Message::ID::VOLUME});
}

void SerialAudioManager::setEqProfile(EqProfile eq) {
    enqueue(Message{Message::ID::SETEQPROFILE, static_cast<uint16_t>(eq)});
}

void SerialAudioManager::queryEqProfile() {
    enqueue(Message{Message::ID::EQPROFILE});
}

void SerialAudioManager::playFile(uint16_t index) {
    enqueue(Message{Message::ID::PLAYFILE, index});
}

void SerialAudioManager::playNextFile() {
    enqueue(Message{Message::ID::PLAYNEXT});
}

void SerialAudioManager::playPreviousFile() {
    enqueue(Message{Message::ID::PLAYPREVIOUS});
}

void SerialAudioManager::loopFile(uint16_t index) {
    enqueue(Message{Message::ID::LOOPFILE, index});
}

void SerialAudioManager::loopAllFiles() {
    enqueue(Message{Message::ID::LOOPALL});
}

void SerialAudioManager::playFilesInRandomOrder() {
    enqueue(Message{Message::ID::RANDOMPLAY});
}

void SerialAudioManager::queryFolderCount() {
    enqueue(Message{Message::ID::FOLDERCOUNT});
}

void SerialAudioManager::playTrack(uint16_t track) {
    enqueue(Message{Message::ID::PLAYFROMMP3, track});
}

void SerialAudioManager::playTrack(uint16_t folder, uint16_t track) {
    if (track < 256) {
        auto const param = combine(
            static_cast<uint8_t>(folder),
            static_cast<uint8_t>(track)
        );
        enqueue(Message{Message::ID::PLAYFROMFOLDER, param});
    } else if (folder < 16) {
        auto const param = ((folder & 0x0F) << 12) | (track & 0x0FFF);
        enqueue(Message{Message::ID::PLAYFROMBIGFOLDER, param});
    }
}

void SerialAudioManager::queryCurrentFile(Device device) {
    switch (device) {
        case Device::USB:    enqueue(Message{Message::ID::CURRENTUSBFILE});   break;
        case Device::SDCARD: enqueue(Message{Message::ID::CURRENTSDFILE});    break;
        case Device::FLASH:  enqueue(Message{Message::ID::CURRENTFLASHFILE}); break;
        default: break;
    }
}

void SerialAudioManager::queryPlaybackSequence() {
    enqueue(Message{Message::ID::PLAYBACKSEQUENCE});
}


void SerialAudioManager::stop() {
    enqueue(Message{Message::ID::STOP});
}

void SerialAudioManager::pause() {
    enqueue(Message{Message::ID::PAUSE});
}

void SerialAudioManager::unpause() {
    enqueue(Message{Message::ID::UNPAUSE});
}




void SerialAudioManager::insertAdvert(uint16_t track) {
    enqueue(Message{Message::ID::INSERTADVERT, track});
}

void SerialAudioManager::insertAdvert(uint8_t folder, uint8_t track) {
    if (folder == 0) return insertAdvert(track);
    enqueue(Message{Message::ID::INSERTADVERTN, combine(folder, track)});
}

void SerialAudioManager::stopAdvert() {
    enqueue(Message{Message::ID::STOPADVERT});
}

void SerialAudioManager::sendMessage(Message const &msg) {
    auto const msgid = msg.getID();
    m_lastRequest = msgid;
    if (msgid == Message::ID::RESET) {
        m_core.send(msg, Feedback::NO_FEEDBACK);
        m_timeout.set(3000);
        m_state = State::INITPENDING;
    } else if (msgid == Message::ID::SELECTSOURCE) {
        m_core.send(msg, Feedback::FEEDBACK);
        m_timeout.set(33);
        m_state = State::SOURCEPENDINGACK;
    } else if (isQuery(msgid)) {
        m_core.send(msg, Feedback::NO_FEEDBACK);
        m_timeout.set(100);
        m_state = State::RESPONSEPENDING;
    } else {
        m_core.send(msg, Feedback::FEEDBACK);
        m_timeout.set(33);
        m_state = State::ACKPENDING;
    }
}

void SerialAudioManager::enqueue(Message const &msg) {
    m_queue[m_tail] = msg;
    m_tail = (m_tail + 1) % 8;
    if (m_tail == m_head) Serial.println(F("Queue overflowed!"));
    if (m_state == State::READY) dispatch();
}

void SerialAudioManager::clearQueue() {
    m_head = m_tail = 0;
}

void SerialAudioManager::dispatch() {
    if (m_head == m_tail) return;

    Serial.print(F("Dispatching m_queue["));
    Serial.print(m_head);
    Serial.println(']');
    sendMessage(m_queue[m_head]);
    m_head = (m_head + 1) % 8;
}


void SerialAudioManager::onEvent(Message const &msg, Hooks *hooks) {
    using ID = Message::ID;

    if (isAsyncNotification(msg.getID())) {
        Serial.println(F("Asynchronous notification received."));
        if (hooks != nullptr) {
            switch (msg.getID()) {
                case ID::DEVICEINSERTED:
                    hooks->onDeviceChange(static_cast<Device>(msg.getParam()),
                                          DeviceChange::INSERTED);
                    break;
                case ID::DEVICEREMOVED:
                    hooks->onDeviceChange(static_cast<Device>(msg.getParam()),
                                          DeviceChange::REMOVED);
                    break;
                case ID::FINISHEDUSBFILE:
                    hooks->onFinishedFile(Device::USB, msg.getParam());
                    break;
                case ID::FINISHEDSDFILE:
                    hooks->onFinishedFile(Device::SDCARD, msg.getParam());
                    break;
                case ID::FINISHEDFLASHFILE:
                    hooks->onFinishedFile(Device::FLASH, msg.getParam());
                    break;
                default:
                    break;
            }
        }
        return;
    }
    
    if (msg.getID() == ID::INITCOMPLETE && m_state != State::INITPENDING) {
        Serial.println(F("Audio device unexpectedly reset!"));
    }

    switch (m_state) {
        case State::READY:
            Serial.println(F("READY: "));
            if (isError(msg.getID())) {
                Serial.println(F("Oops, lastRequest actually got an error."));
            } else {
                Serial.println(F("unexpected event"));
            }
            break;
        case State::ACKPENDING:
            Serial.print(F("ACKPENDING: "));
            if (msg.getID() == Message::ID::ACK) {
                Serial.println(F("Ack'ed"));
                ready();
            } else if (isError(msg.getID())) {
                Serial.println(F("error"));
                ready();
            } else {
                Serial.println(F("unexpected event"));
                ready();
            }
            break;
        case State::RESPONSEPENDING:
            Serial.print(F("RESPONSEPENDING: "));
            if (msg.getID() == m_lastRequest) {
                Serial.println(F("response received"));
                if (hooks != nullptr) {
                    hooks->onQueryResponse(static_cast<Parameter>(m_lastRequest), msg.getParam());
                }
                ready();
            } else if (isError(msg.getID())) {
                Serial.println(F("error"));
                ready();
            } else {
                Serial.println(F("unexpected event"));
                ready();
            }
            break;
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
        case State::SOURCEPENDINGACK:
            Serial.print(F("SOURCEPENDINGACK: "));
            if (msg.getID() == ID::ACK) {
                Serial.println(F("ACK received."));
                m_timeout.set(200);  // Wait 200 ms after selecting a source.
                m_state = State::SOURCEPENDINGDELAY;
            } else if (isError(msg.getID())) {
                Serial.println(F("error"));
                ready();
            } else {
                Serial.println(F("unexpected event"));
                ready();
            }
            break;
        case State::SOURCEPENDINGDELAY:
            Serial.print(F("SOURCEPENDINGDELAY: "));
            if (isTimeout(msg)) {
                Serial.println(F("delay completed"));
                ready();
            } else {
                Serial.println(F("unexpected event"));
            }
            break;
        case State::STUCK:
            Serial.println(F("STUCK: "));
            Serial.println(F("unexpected event"));
            break;
    }
}

void SerialAudioManager::ready() {
    m_timeout.cancel();
    m_state = State::READY;
    dispatch();
}

}
