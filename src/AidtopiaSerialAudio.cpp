// AidtopiaSerialAudio
// Adrian McCarthy 2018-

#include <Arduino.h>
#include <AidtopiaSerialAudio.h>
#include <utilities/message.h>

namespace aidtopia {

static constexpr uint16_t combine(uint8_t hi, uint8_t lo) {
  return (static_cast<uint16_t>(hi) << 8) | lo;
}

static constexpr uint8_t MSB(uint16_t word) {
    return static_cast<uint8_t>(word >> 8);
}

static constexpr uint8_t LSB(uint16_t word) {
    return static_cast<uint8_t>(word & 0x00FF);
}

static bool isTimeout(Message const &msg) {
    return isError(msg) &&
           msg.getParam() == static_cast<uint16_t>(SerialAudio::Error::TIMEDOUT);
}


void SerialAudio::update(Hooks *hooks) {
    Message msg;
    if (m_core.update(&msg)) onEvent(msg, hooks);
    if (m_timeout.expired()) {
        auto const timeout =
            Message{Message::ID::ERROR, static_cast<uint16_t>(Error::TIMEDOUT)};
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
    m_queue.clear();
    enqueue(Message::ID::RESET, Feedback::FEEDBACK, State::RESETACKPENDING, 30);
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
    enqueue(Message::ID::FIRMWAREVERSION, Feedback::NO_FEEDBACK,
            State::RESPONSEPENDING, 100);
}

void SerialAudio::selectSource(Device source) {
    auto const paramLo = static_cast<uint8_t>(source);
    enqueue(Message::ID::SELECTSOURCE, combine(0, paramLo), Feedback::FEEDBACK,
            State::SOURCEACKPENDING, 30);
}

void SerialAudio::queryStatus() {
    enqueue(Message::ID::STATUS, Feedback::NO_FEEDBACK,
            State::RESPONSEPENDING, 100);
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
    enqueue(Message::ID::VOLUME, Feedback::NO_FEEDBACK,
            State::RESPONSEPENDING, 100);
}

void SerialAudio::setEqProfile(EqProfile eq) {
    enqueue(Message::ID::SETEQPROFILE, static_cast<uint16_t>(eq));
}

void SerialAudio::queryEqProfile() {
    enqueue(Message::ID::EQPROFILE, Feedback::NO_FEEDBACK,
            State::RESPONSEPENDING, 100);
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
    enqueue(Message::ID::FOLDERCOUNT, Feedback::NO_FEEDBACK,
            State::RESPONSEPENDING, 100);
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
    // Note the longer timeout because the modules take a bit longer.  I think
    // the module checks the filesystem for the folder first.
    // Also note this command ACKs twice when successful.  I think one is for
    // the command to put it into loop folder mode and the second is when it
    // actually begins playing.
    enqueue(Message::ID::LOOPFOLDER, folder, Feedback::FEEDBACK,
            State::EXTRAACKPENDING, 100);
}

void SerialAudio::queryCurrentFile(Device device) {
    auto msgid = Message::ID::NONE;
    switch (device) {
        case Device::USB:    msgid = Message::ID::CURRENTUSBFILE;   break;
        case Device::SDCARD: msgid = Message::ID::CURRENTSDFILE;    break;
        case Device::FLASH:  msgid = Message::ID::CURRENTFLASHFILE; break;
        default: return;
    }
    enqueue(msgid, Feedback::NO_FEEDBACK, State::RESPONSEPENDING, 100);
}

void SerialAudio::queryPlaybackSequence() {
    enqueue(Message::ID::PLAYBACKSEQUENCE, Feedback::NO_FEEDBACK,
            State::RESPONSEPENDING, 100);
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

void SerialAudio::enqueue(Command const &cmd) {
    m_queue.pushBack(cmd);
    if (m_state == State::READY) dispatch();
}

void SerialAudio::enqueue(
    Message::ID msgid, uint16_t data,
    Feedback feedback,
    State state,
    unsigned timeout
) {
    enqueue(Command{Message{msgid, data}, feedback, state, timeout});
}

void SerialAudio::enqueue(
    Message::ID msgid,
    Feedback feedback,
    State state,
    unsigned timeout
) {
    enqueue(msgid, 0, feedback, state, timeout);
}

void SerialAudio::enqueue(Message const &msg) {
    Command cmd = {msg, Feedback::FEEDBACK, State::ACKPENDING, 33};
    enqueue(cmd);
}

void SerialAudio::enqueue(Message::ID msgid, uint16_t data) {
    enqueue(Message{msgid, data});
}

void SerialAudio::dispatch() {
    if (m_queue.empty()) return;
    auto const &cmd = m_queue.peekFront();
    sendMessage(cmd.msg, cmd.feedback);
    m_state = cmd.state;
    m_timeout.set(cmd.timeout);
    m_queue.popFront();
}

void SerialAudio::onEvent(Message const &msg, Hooks *hooks) {
    using ID = Message::ID;

    if (isAsyncNotification(msg.getID())) {
        if (hooks != nullptr) {
            if (msg == m_lastNotification) {
                // Some notifications arrive twice in a row, but we don't want
                // to confuse the client by calling back.
                m_lastNotification.clear();
                Serial.println(F("Duplicate notification suppressed."));
                return;
            }
            m_lastNotification = msg;
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
    
    if (msg.getID() == ID::INITCOMPLETE &&
        m_state != State::POWERUPINITPENDING &&
        m_state != State::RESETINITPENDING
    ) {
        Serial.println(F("Audio module unexpectedly reset!"));
        m_queue.clear();
        if (hooks != nullptr) {
            hooks->onInitComplete(LSB(msg.getParam()));
        }
        ready();
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

        case State::EXTRAACKPENDING:
            Serial.print(F("EXTRAACKPENDING: "));
            if (msg.getID() == Message::ID::ACK) {
                Serial.println(F("ACK'ed"));
                m_timeout.set(100);
                m_state = State::ACKPENDING;
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

        case State::POWERUPINITPENDING:
            Serial.print(F("POWERUPINITPENDING: "));
            if (msg.getID() == ID::INITCOMPLETE) {
                Serial.println(F("completed"));
                if (hooks != nullptr) {
                    hooks->onInitComplete(LSB(msg.getParam()));
                }
                ready();
            } else if (isTimeout(msg)) {
                Serial.println(F("Timed out"));
                // Maybe the module is initialized and just quietly waiting for
                // us.  We'll check its status.
                sendMessage(Message{ID::STATUS}, Feedback::NO_FEEDBACK);
                m_state = State::POWERUPSTATUSPENDING;
                m_timeout.set(300);
            } else if (isError(msg)) {
                if (hooks != nullptr) {
                    hooks->onError(static_cast<SerialAudio::Error>(msg.getParam()));
                }
                m_state = State::STUCK;
            } else {
                Serial.println(F("unexpected event"));
            }
            break;

        case State::POWERUPSTATUSPENDING:
            Serial.print(F("POWERUPSTATUSPENDING: "));
            if (msg.getID() == ID::STATUS) {
                Serial.println(F("got status response"));
                // TODO m_state = State::DISCOVERDEVICES
                if (hooks != nullptr) {
                    hooks->onInitComplete(MSB(msg.getParam()));
                }
                ready();
            } else if (isTimeout(msg)) {
                Serial.println(F("Timed out"));
                m_state = State::STUCK;
            } else {
                Serial.println(F("unexpected event"));
            }
            break;

        case State::RESETACKPENDING:
            Serial.print(F("RESETACKPENDING: "));
            if (msg.getID() == Message::ID::ACK) {
                Serial.println(F("ACK"));
                m_timeout.set(3000);
                m_state = State::RESETINITPENDING;
            } else if (isError(msg)) {
                Serial.println(F("error"));
                if (hooks != nullptr) {
                    hooks->onError(static_cast<Error>(msg.getParam()));
                }
                ready();
            } else {
                Serial.println(F("unexpected event"));
            }
            break;

        case State::RESETINITPENDING:
            Serial.print(F("RESETINITPENDING: "));
            if (msg.getID() == ID::INITCOMPLETE) {
                Serial.println(F("completed"));
                if (hooks != nullptr) {
                    hooks->onInitComplete(LSB(msg.getParam()));
                }
                ready();
            } else if (isTimeout(msg)) {
                Serial.println(F("Timed out"));
                m_state = State::STUCK;
            } else if (isError(msg)) {
                if (hooks != nullptr) {
                    hooks->onError(static_cast<SerialAudio::Error>(msg.getParam()));
                }
                m_state = State::STUCK;
            } else {
                Serial.println(F("unexpected event"));
            }
            break;

        case State::SOURCEACKPENDING:
            Serial.print(F("SOURCEACKPENDING: "));
            if (msg.getID() == ID::ACK) {
                Serial.println(F("ACK received."));
                m_timeout.set(200);  // Wait 200 ms after selecting a source.
                m_state = State::SOURCEDELAYPENDING;
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

        case State::SOURCEDELAYPENDING:
            Serial.print(F("SOURCEDELAYPENDING: "));
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
            }
            break;

        case State::STUCK:
            Serial.print(F("STUCK: "));
            Serial.println(F("unexpected event"));
            if (isError(msg) && hooks != nullptr) {
                hooks->onError(static_cast<Error>(msg.getParam()));
            }
            break;
    }
}

void SerialAudio::onPowerUp() {
    // I used to just hit the module with a reset command on powerup, but
    // documentation and experience suggests that the device might not tolerate
    // a reset while already resetting.  So we'll assume the device is powering
    // up and wait for an initialization complete (0x3F) notification.  If one
    // doesn't come, the state machine will fall back to figuring out whether
    // the module is already online and to discover what devices are attached.
    m_queue.clear();
    m_lastRequest = Message::ID::NONE;
    m_lastNotification.clear();
    m_state = State::POWERUPINITPENDING;
    m_timeout.set(3000);
    Serial.println(F("onPowerUp: Waiting for INITCOMPLETE."));
}

void SerialAudio::ready() {
    m_timeout.cancel();
    m_state = State::READY;
    dispatch();
}

void SerialAudio::sendMessage(Message const &msg, Feedback feedback) {
    m_core.send(msg, feedback);
    m_lastRequest = msg.getID();
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


