// AidtopiaSerialAudio
// Adrian McCarthy 2018-

#include <Arduino.h>
#include <AidtopiaSerialAudio.h>
#include <utilities/message.h>

namespace aidtopia {

static constexpr uint16_t combine(uint8_t hi, uint8_t lo) {
  return (static_cast<uint16_t>(hi) << 8) | lo;
}

void SerialAudio::update(Hooks *hooks) {
    Message msg;
    if (m_core.update(&msg)) onEvent(msg, hooks);
    if (m_timeout.expired()) {
        auto const timeout =
            Message{Message::ID::ERROR, static_cast<uint8_t>(Error::TIMEDOUT)};
        onEvent(timeout, hooks);
    }
}

SerialAudio::Hooks::~Hooks() {}
void SerialAudio::Hooks::onError(Error) {}
void SerialAudio::Hooks::onQueryResponse(Parameter, uint16_t) {}
void SerialAudio::Hooks::onDeviceChange(Device, DeviceChange) {}
void SerialAudio::Hooks::onFinishedFile(Device, uint16_t) {}
void SerialAudio::Hooks::onInitComplete(uint8_t) {}

void SerialAudio::reset() {
    clearQueue();
    enqueue(Message::ID::RESET);
}

void SerialAudio::queryFileCount(Device device) {
    switch (device) {
        case Device::USB:    enqueue(Message::ID::USBFILECOUNT);   break;
        case Device::SDCARD: enqueue(Message::ID::SDFILECOUNT);    break;
        case Device::FLASH:  enqueue(Message::ID::FLASHFILECOUNT); break;
        default: break;
    }
}

void SerialAudio::queryFirmwareVersion() {
    enqueue(Message::ID::FIRMWAREVERSION);
}

void SerialAudio::selectSource(Device source) {
    auto const paramLo = static_cast<uint8_t>(source);
    enqueue(Message::ID::SELECTSOURCE, combine(0, paramLo));
}

void SerialAudio::queryStatus() {
    enqueue(Message::ID::STATUS);
}

void SerialAudio::setVolume(uint8_t volume) {
    volume = min(volume, 30);
    enqueue(Message::ID::SETVOLUME, volume);
}

void SerialAudio::increaseVolume() {
    enqueue(Message::ID::VOLUMEUP);
}

void SerialAudio::decreaseVolume() {
    enqueue(Message::ID::VOLUMEDOWN);
}

void SerialAudio::queryVolume() {
    enqueue(Message::ID::VOLUME);
}

void SerialAudio::setEqProfile(EqProfile eq) {
    enqueue(Message::ID::SETEQPROFILE, static_cast<uint16_t>(eq));
}

void SerialAudio::queryEqProfile() {
    enqueue(Message::ID::EQPROFILE);
}

void SerialAudio::playFile(uint16_t index) {
    enqueue(Message::ID::PLAYFILE, index);
}

void SerialAudio::playNextFile() {
    enqueue(Message::ID::PLAYNEXT);
}

void SerialAudio::playPreviousFile() {
    enqueue(Message::ID::PLAYPREVIOUS);
}

void SerialAudio::loopFile(uint16_t index) {
    enqueue(Message::ID::LOOPFILE, index);
}

void SerialAudio::loopAllFiles() {
    enqueue(Message::ID::LOOPALL);
}

void SerialAudio::playFilesInRandomOrder() {
    enqueue(Message::ID::RANDOMPLAY);
}

void SerialAudio::queryFolderCount() {
    enqueue(Message::ID::FOLDERCOUNT);
}

void SerialAudio::playTrack(uint16_t track) {
    enqueue(Message::ID::PLAYFROMMP3, track);
}

void SerialAudio::playTrack(uint16_t folder, uint16_t track) {
    if (track < 256) {
        auto const param = combine(
            static_cast<uint8_t>(folder),
            static_cast<uint8_t>(track)
        );
        enqueue(Message::ID::PLAYFROMFOLDER, param);
    } else if (folder < 16) {
        auto const param = ((folder & 0x0F) << 12) | (track & 0x0FFF);
        enqueue(Message::ID::PLAYFROMBIGFOLDER, param);
    }
}

void SerialAudio::loopFolder(uint16_t folder) {
    enqueue(Message::ID::LOOPFOLDER, folder);
}

void SerialAudio::queryCurrentFile(Device device) {
    switch (device) {
        case Device::USB:    enqueue(Message::ID::CURRENTUSBFILE);   break;
        case Device::SDCARD: enqueue(Message::ID::CURRENTSDFILE);    break;
        case Device::FLASH:  enqueue(Message::ID::CURRENTFLASHFILE); break;
        default: break;
    }
}

void SerialAudio::queryPlaybackSequence() {
    enqueue(Message::ID::PLAYBACKSEQUENCE);
}


void SerialAudio::stop() {
    enqueue(Message::ID::STOP);
}

void SerialAudio::pause() {
    enqueue(Message::ID::PAUSE);
}

void SerialAudio::unpause() {
    enqueue(Message::ID::UNPAUSE);
}


void SerialAudio::insertAdvert(uint16_t track) {
    enqueue(Message::ID::INSERTADVERT, track);
}

void SerialAudio::insertAdvert(uint8_t folder, uint8_t track) {
    if (folder == 0) return insertAdvert(track);
    enqueue(Message::ID::INSERTADVERTN, combine(folder, track));
}

void SerialAudio::stopAdvert() {
    enqueue(Message::ID::STOPADVERT);
}

void SerialAudio::sendMessage(Message const &msg) {
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

void SerialAudio::enqueue(Message const &msg) {
    m_queue[m_tail] = msg;
    m_tail = (m_tail + 1) % 8;
    if (m_tail == m_head) Serial.println(F("Queue overflowed!"));
    if (m_state == State::READY) dispatch();
}

void SerialAudio::enqueue(Message::ID msgid, uint16_t data) {
    enqueue(msgid, data);
}

void SerialAudio::clearQueue() {
    m_head = m_tail = 0;
}

void SerialAudio::dispatch() {
    if (m_head == m_tail) return;

    Serial.print(F("Dispatching m_queue["));
    Serial.print(m_head);
    Serial.println(']');
    sendMessage(m_queue[m_head]);
    m_head = (m_head + 1) % 8;
}


static bool isTimeout(Message const &msg) {
    return isError(msg) && msg.getParam() == static_cast<uint16_t>(SerialAudio::Error::TIMEDOUT);
}

void SerialAudio::onEvent(Message const &msg, Hooks *hooks) {
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
            Serial.print(F("READY: "));
            if (isError(msg)) {
                Serial.println(F("Oops, lastRequest actually got an error."));
                if (hooks != nullptr) {
                    hooks->onError(static_cast<Error>(msg.getParam()));
                }
            } else {
                Serial.println(F("unexpected event"));
            }
            break;
        case State::ACKPENDING:
            Serial.print(F("ACKPENDING: "));
            if (msg.getID() == Message::ID::ACK) {
                Serial.println(F("Ack'ed"));
                ready();
            } else if (isError(msg)) {
                Serial.println(F("error"));
                if (hooks != nullptr) {
                    hooks->onError(static_cast<Error>(msg.getParam()));
                }
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
            } else if (isError(msg)) {
                if (hooks != nullptr) {
                    hooks->onError(static_cast<Error>(msg.getParam()));
                }
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
            } else if (isError(msg)) {
                if (hooks != nullptr) {
                    hooks->onError(static_cast<SerialAudio::Error>(msg.getParam()));
                }
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
            } else if (isError(msg)) {
                if (hooks != nullptr) {
                    hooks->onError(static_cast<Error>(msg.getParam()));
                }
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
            } else if (isError(msg)) {
                if (hooks != nullptr) {
                    hooks->onError(static_cast<Error>(msg.getParam()));
                }
                ready();
            } else {
                Serial.println(F("unexpected event"));
                ready();
            }
            break;
        case State::STUCK:
            Serial.println(F("STUCK: "));
            Serial.println(F("unexpected event"));
            if (isError(msg) && hooks != nullptr) {
                hooks->onError(static_cast<Error>(msg.getParam()));
            }
            break;
    }
}

void SerialAudio::ready() {
    m_timeout.cancel();
    m_state = State::READY;
    dispatch();
}

void SerialAudio::onPowerUp() {
    clearQueue();
    m_state = State::INITPENDING;
    m_timeout.set(3000);
}

#if 0
void printModuleStateName(ModuleState state) {
  switch (state) {
    case MS_STOPPED: Serial.print(F("Stopped")); break;
    case MS_PLAYING: Serial.print(F("Playing")); break;
    case MS_PAUSED:  Serial.print(F("Paused"));  break;
    case MS_ASLEEP:  Serial.print(F("Asleep"));  break;
    default:         Serial.print(F("???"));     break;
  }
}

void printSequenceName(Sequence seq) {
  switch (seq) {
    case SEQ_LOOPALL:    Serial.print(F("Loop All")); break;
    case SEQ_LOOPFOLDER: Serial.print(F("Loop Folder")); break;
    case SEQ_LOOPTRACK:  Serial.print(F("Loop Track")); break;
    case SEQ_RANDOM:     Serial.print(F("Random")); break;
    case SEQ_SINGLE:     Serial.print(F("Single")); break;
    default:             Serial.print(F("???")); break;
  }
}
#endif

}


