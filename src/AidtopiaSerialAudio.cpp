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

bool SerialAudio::update(Hooks *hooks) {
    Message msg;
    if (m_core.update(&msg)) onEvent(msg, hooks);
    if (m_timeout.expired()) {
        auto const timeout =
            Message{Message::ID::ERROR, static_cast<uint16_t>(Error::TIMEDOUT)};
        onEvent(timeout, hooks);
    }
    dispatch();
    return !m_queue.full();
}

SerialAudio::Hooks::~Hooks() {}
void SerialAudio::Hooks::onError(Error) {}
void SerialAudio::Hooks::onQueryResponse(Parameter, uint16_t) {}
void SerialAudio::Hooks::onDeviceChange(Device, DeviceChange) {}
void SerialAudio::Hooks::onFinishedFile(Device, uint16_t) {}
void SerialAudio::Hooks::onInitComplete(uint8_t) {}

void SerialAudio::reset() {
    m_queue.clear();
    m_lastNotification.clear();
    dispatch(Message::ID::RESET, State::EXPECT_ACK | State::UNINITIALIZED);
    m_timeout.set(3000);
}

void SerialAudio::queryFileCount(Device device) {
    // TODO: We need to ensure these get a longer timeout.
    Message::ID msgid;
    switch (device) {
        case Device::USB:    msgid = Message::ID::USBFILECOUNT;    break;
        case Device::SDCARD: msgid = Message::ID::SDFILECOUNT;     break;
        case Device::FLASH:  msgid = Message::ID::FLASHFILECOUNT;  break;
        default:  return;
    }
    enqueue(msgid, State::EXPECT_RESPONSE);
}

void SerialAudio::queryFirmwareVersion() {
    enqueue(Message::ID::FIRMWAREVERSION, State::EXPECT_RESPONSE);
}

void SerialAudio::selectSource(Device source) {
    auto const paramLo = static_cast<uint8_t>(source);
    enqueue(Message::ID::SELECTSOURCE, State::EXPECT_ACK | State::DELAY,
            combine(0, paramLo));
}

void SerialAudio::queryStatus() {
    // TODO:  Might need a longer timeout.
    enqueue(Message::ID::STATUS, State::EXPECT_RESPONSE);
}

void SerialAudio::setVolume(uint8_t volume) {
    volume = min(volume, 30);
    enqueue(Message::ID::SETVOLUME, State::EXPECT_ACK, volume);
}

void SerialAudio::increaseVolume() {
    enqueue(Message::ID::VOLUMEUP, State::EXPECT_ACK);
}

void SerialAudio::decreaseVolume() {
    enqueue(Message::ID::VOLUMEDOWN, State::EXPECT_ACK);
}

void SerialAudio::queryVolume() {
    enqueue(Message::ID::VOLUME, State::EXPECT_RESPONSE);
}

void SerialAudio::setEqProfile(EqProfile eq) {
    enqueue(Message::ID::SETEQPROFILE, State::EXPECT_ACK,
            static_cast<uint16_t>(eq));
}

void SerialAudio::queryEqProfile() {
    enqueue(Message::ID::EQPROFILE, State::EXPECT_RESPONSE);
}

void SerialAudio::playFile(uint16_t index) {
    enqueue(Message::ID::PLAYFILE, State::EXPECT_ACK, index);
}

void SerialAudio::playNextFile() {
    enqueue(Message::ID::PLAYNEXT, State::EXPECT_ACK);
}

void SerialAudio::playPreviousFile() {
    enqueue(Message::ID::PLAYPREVIOUS, State::EXPECT_ACK);
}

void SerialAudio::loopFile(uint16_t index) {
    enqueue(Message::ID::LOOPFILE, State::EXPECT_ACK, index);
}

void SerialAudio::loopAllFiles() {
    enqueue(Message::ID::LOOPALL, State::EXPECT_ACK);
}

void SerialAudio::playFilesInRandomOrder() {
    enqueue(Message::ID::RANDOMPLAY, State::EXPECT_ACK);
}

void SerialAudio::queryFolderCount() {
    enqueue(Message::ID::FOLDERCOUNT, State::EXPECT_RESPONSE);
}

void SerialAudio::playTrack(uint16_t track) {
    enqueue(Message::ID::PLAYFROMMP3, State::EXPECT_ACK, track);
}

void SerialAudio::playTrack(uint16_t folder, uint16_t track) {
    if (track < 256) {
        auto const param = combine(
            static_cast<uint8_t>(folder),
            static_cast<uint8_t>(track)
        );
        enqueue(Message::ID::PLAYFROMFOLDER, State::EXPECT_ACK, param);
    } else if (folder < 16) {
        auto const param = ((folder & 0x0F) << 12) | (track & 0x0FFF);
        enqueue(Message::ID::PLAYFROMBIGFOLDER, State::EXPECT_ACK, param);
    }
}

void SerialAudio::loopCurrentTrack() {
    enqueue(Message::ID::LOOPCURRENTTRACK, State::EXPECT_ACK, 0);
}

void SerialAudio::stopLoopingCurrentTrack() {
    enqueue(Message::ID::LOOPCURRENTTRACK, State::EXPECT_ACK, 1);
}

void SerialAudio::loopFolder(uint16_t folder) {
    // Note that this command ACKs twice when successful.  I think one is for
    // the command to put it into loop folder mode and the second is when it
    // actually begins playing.
    enqueue(Message::ID::LOOPFOLDER, State::EXPECT_ACK | State::EXPECT_ACK2,
            folder);
}

void SerialAudio::queryCurrentFile(Device device) {
    auto msgid = Message::ID::NONE;
    switch (device) {
        case Device::USB:    msgid = Message::ID::CURRENTUSBFILE;   break;
        case Device::SDCARD: msgid = Message::ID::CURRENTSDFILE;    break;
        case Device::FLASH:  msgid = Message::ID::CURRENTFLASHFILE; break;
        default: return;
    }
    enqueue(msgid, State::EXPECT_RESPONSE);
}

void SerialAudio::queryPlaybackSequence() {
    enqueue(Message::ID::PLAYBACKSEQUENCE, State::EXPECT_RESPONSE);
}


void SerialAudio::stop() {
    enqueue(Message::ID::STOP, State::EXPECT_ACK);
}

void SerialAudio::pause() {
    enqueue(Message::ID::PAUSE, State::EXPECT_ACK);
}

void SerialAudio::unpause() {
    enqueue(Message::ID::UNPAUSE, State::EXPECT_ACK);
}

void SerialAudio::insertAdvert(uint16_t track) {
    enqueue(Message::ID::INSERTADVERT, State::EXPECT_ACK, track);
}

void SerialAudio::insertAdvert(uint8_t folder, uint8_t track) {
    if (folder == 0) return insertAdvert(track);
    enqueue(Message::ID::INSERTADVERTN, State::EXPECT_ACK,
            combine(folder, track));
}

void SerialAudio::stopAdvert() {
    enqueue(Message::ID::STOPADVERT, State::EXPECT_ACK);
}

void SerialAudio::dispatch() {
    if (!m_state.ready()) return;
    if (m_queue.empty()) return;
    Serial.println(F("Dispatching from queue"));
    dispatch(m_queue.peekFront());
    m_queue.popFront();
}

void SerialAudio::dispatch(Message::ID msgid, State::Flag flags, uint16_t data) {
    dispatch(Command{State{msgid, flags}, data});
}

void SerialAudio::dispatch(Command const &cmd) {
    auto const feedback =
        cmd.state.has(State::EXPECT_ACK) ? Feedback::FEEDBACK :
                                            Feedback::NO_FEEDBACK;
    m_core.send(Message{cmd.state.sent(), cmd.param}, feedback);
    m_state = cmd.state;
    unsigned const duration =
        m_state.has(State::EXPECT_ACK)      ? 30  :
        m_state.has(State::EXPECT_RESPONSE) ? 100 : 0;
    m_timeout.set(duration);
}

bool SerialAudio::enqueue(Message::ID msgid, State::Flag flags, uint16_t data) {
    auto const cmd = Command{State{msgid, flags}, data};
    if (!m_queue.pushBack(cmd)) {
        Serial.println(F("*** Failed to enqueue command"));
        return false;
    }
    dispatch();
    return true;
}

void SerialAudio::onEvent(Message const &msg, Hooks *hooks) {
    Serial.print(F("onEvent ("));
    Serial.print(static_cast<uint8_t>(msg.getID()), HEX);
    Serial.print(F(", "));
    Serial.print(msg.getParam(), HEX);
    Serial.print(F(") State="));
    Serial.print(static_cast<uint8_t>(m_state.sent()), HEX);
    if (m_state.has(State::EXPECT_ACK))       Serial.print(F(" | EXPECT_ACK"));
    if (m_state.has(State::EXPECT_ACK2))      Serial.print(F(" | EXPECT_ACK2"));
    if (m_state.has(State::EXPECT_RESPONSE))  Serial.print(F(" | EXPECT_RESPONSE"));
    if (m_state.has(State::DELAY))            Serial.print(F(" | DELAY"));
    if (m_state.has(State::UNINITIALIZED))    Serial.print(F(" | UNINITIALIZED"));
    Serial.println();
    
    handleEvent(msg, hooks);
    
    // We might be ready to dispatch a queued command now.
    dispatch();
}

void SerialAudio::handleEvent(Message const &msg, Hooks *hooks) {
    using ID = Message::ID;

    if (isAsyncNotification(msg)) {
        // TODO:  Consider what should happen if a device is inserted while
        // we're in an uninitialized or a no-sources state.

        if (hooks != nullptr) {
            if (msg == m_lastNotification) {
                // Some notifications arrive twice in a row, but we don't want
                // to confuse the client by calling back twice.
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
    
    if (isAck(msg)) {
        if (m_state.testAndClear(State::EXPECT_ACK)) {
            m_timeout.cancel();
            if (m_state.hasAny(State::EXPECT_ACK2 | State::DELAY)) {
                m_timeout.set(300);
            }
            return;
        }
        if (m_state.testAndClear(State::EXPECT_ACK2)) {
            m_timeout.cancel();
            return;
        }
        Serial.println(F("Unexpected ACK!"));
        return;
    }

    if (isInitComplete(msg)) {
        if (m_state.poweringUp()) {
            Serial.println(F("Got INITCOMPLETE on power up"));
            m_state.clear(State::UNINITIALIZED);
            m_timeout.cancel();
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        if (m_state.sent() == ID::RESET) {
            Serial.println(F("Reset completed"));
            m_state.clear(State::UNINITIALIZED);
            m_timeout.cancel();
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        if (m_state.sent() == ID::INITCOMPLETE) {
            Serial.println(F("OMG! INITCOMPLETE worked as a query!"));
            m_state.clear(State::EXPECT_RESPONSE);
            m_timeout.cancel();
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        Serial.println(F("Audio module unexpectedly reset!"));
        m_state = State{Message:ID::NONE};
        m_timeout.cancel();
        m_queue.clear();
        m_lastNotification.clear();
        if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
        return;
    }
    
    if (isQueryResponse(msg)) {
        if (m_state.has(State::EXPECT_RESPONSE)) {
            if (msg.getID() == m_state.sent()) {
                Serial.println(F("Received expected query response"));
                m_timeout.cancel();
                m_state.clear(State::EXPECT_RESPONSE);
                if (msg.getID() == ID::STATUS && m_state.has(State::UNINITIALIZED)) {
                    // A valid response to a status query means we've detected a
                    // live audio module after powerup.
                    m_state.clear(State::UNINITIALIZED);
                    if (hooks != nullptr) {
                        // The device given in the status message is a reasonble
                        // proxy for the set of storage devices attached.
                        auto const devices = MSB(msg.getParam());
                        hooks->onInitComplete(devices);
                        return;
                    }
                }
                if (hooks != nullptr) {
                    hooks->onQueryResponse(static_cast<Parameter>(msg.getID()),
                                           msg.getParam());
                }
                return;
            }
            Serial.println(F("Got different query response than expected"));
            return;
        }
        Serial.println(F("Got unexpected query response!"));
        return;
    }
    
    if (isTimeout(msg)) {
        if (m_state.testAndClear(State::DELAY)) {
            Serial.println(F("Delay completed"));
            m_timeout.cancel();
            return;
        }
        if (m_state.poweringUp()) {
            Serial.println(F("Didn't receive INITCOMPLETE on powerup."));
            // Let's try querying the module to see whether it's already running.
            dispatch(ID::STATUS, State::EXPECT_RESPONSE | State::UNINITIALIZED);
            return;
        }
        // Fall through to the general error handler.
    }
    
    if (isError(msg)) {
        m_timeout.cancel();
        m_state.clear(State::ALL_FLAGS);
        if (hooks != nullptr) {
            hooks->onError(static_cast<SerialAudio::Error>(msg.getParam()));
        }
        return;
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
    m_lastNotification.clear();
    m_state = State();
    m_timeout.set(3000);
    Serial.println(F("onPowerUp: Waiting for INITCOMPLETE."));
}

}
