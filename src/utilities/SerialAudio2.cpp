#include <Arduino.h>
#include <utilities/SerialAudio2.h>
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
           msg.getParam() == static_cast<uint16_t>(SerialAudio2::Error::TIMEDOUT);
}

void SerialAudio2::update(Hooks *hooks) {
    Message msg;
    if (m_core.update(&msg)) onEvent(msg, hooks);
    if (m_timeout.expired()) {
        auto const timeout =
            Message{Message::ID::ERROR, static_cast<uint16_t>(Error::TIMEDOUT)};
        onEvent(timeout, hooks);
    }
    if (m_state.ready()) dispatch();
}

SerialAudio2::Hooks::~Hooks() {}
void SerialAudio2::Hooks::onError(Error) {}
void SerialAudio2::Hooks::onQueryResponse(Parameter, uint16_t) {}
void SerialAudio2::Hooks::onDeviceChange(Device, DeviceChange) {}
void SerialAudio2::Hooks::onFinishedFile(Device, uint16_t) {}
void SerialAudio2::Hooks::onInitComplete(uint8_t) {}

void SerialAudio2::reset() {
    m_queue.clear();
    //enqueue(Message::ID::RESET, Feedback::FEEDBACK, State::RESETACKPENDING, 30);
}

void SerialAudio2::queryFileCount(Device device) {
    // TODO: We need to ensure these get a longer timeout.
    Message::ID msgid;
    switch (device) {
        case Device::USB:    msgid = Message::ID::USBFILECOUNT;    break;
        case Device::SDCARD: msgid = Message::ID::SDFILECOUNT;     break;
        case Device::FLASH:  msgid = Message::ID::FLASHFILECOUNT;  break;
        default:  return;
    }
    enqueue(msgid, State2::EXPECT_RESPONSE);
}

void SerialAudio2::queryFirmwareVersion() {
    enqueue(Message::ID::FIRMWAREVERSION, State2::EXPECT_RESPONSE);
}

void SerialAudio2::selectSource(Device source) {
    auto const paramLo = static_cast<uint8_t>(source);
    enqueue(Message::ID::SELECTSOURCE, State2::EXPECT_ACK | State2::DELAY,
            combine(0, paramLo));
}

void SerialAudio2::queryStatus() {
    // TODO:  Might need a longer timeout.
    enqueue(Message::ID::STATUS, State2::EXPECT_RESPONSE);
}

void SerialAudio2::setVolume(uint8_t volume) {
    volume = min(volume, 30);
    enqueue(Message::ID::SETVOLUME, State2::EXPECT_ACK, volume);
}

void SerialAudio2::increaseVolume() {
    enqueue(Message::ID::VOLUMEUP, State2::EXPECT_ACK);
}

void SerialAudio2::decreaseVolume() {
    enqueue(Message::ID::VOLUMEDOWN, State2::EXPECT_ACK);
}

void SerialAudio2::queryVolume() {
    enqueue(Message::ID::VOLUME, State2::EXPECT_RESPONSE);
}

void SerialAudio2::setEqProfile(EqProfile /*eq*/) {
//    enqueue(Message::ID::SETEQPROFILE, static_cast<uint16_t>(eq));
}

void SerialAudio2::queryEqProfile() {
    enqueue(Message::ID::EQPROFILE, State2::EXPECT_RESPONSE);
}

void SerialAudio2::playFile(uint16_t /*index*/) {
//    enqueue(Message::ID::PLAYFILE, index);
}

void SerialAudio2::playNextFile() {
//    enqueue(Message::ID::PLAYNEXT);
}

void SerialAudio2::playPreviousFile() {
//    enqueue(Message::ID::PLAYPREVIOUS);
}

void SerialAudio2::loopFile(uint16_t /*index*/) {
//    enqueue(Message::ID::LOOPFILE, index);
}

void SerialAudio2::loopAllFiles() {
//    enqueue(Message::ID::LOOPALL);
}

void SerialAudio2::playFilesInRandomOrder() {
//    enqueue(Message::ID::RANDOMPLAY);
}

void SerialAudio2::queryFolderCount() {
    enqueue(Message::ID::FOLDERCOUNT, State2::EXPECT_RESPONSE);
}

void SerialAudio2::playTrack(uint16_t track) {
    enqueue(Message::ID::PLAYFROMMP3, State2::EXPECT_ACK, track);
}

void SerialAudio2::playTrack(uint16_t folder, uint16_t track) {
    if (track < 256) {
        auto const param = combine(
            static_cast<uint8_t>(folder),
            static_cast<uint8_t>(track)
        );
        enqueue(Message::ID::PLAYFROMFOLDER, State2::EXPECT_ACK, param);
    } else if (folder < 16) {
        auto const param = ((folder & 0x0F) << 12) | (track & 0x0FFF);
        enqueue(Message::ID::PLAYFROMBIGFOLDER, State2::EXPECT_ACK, param);
    }
}

void SerialAudio2::loopFolder(uint16_t folder) {
    // Note that this command ACKs twice when successful.  I think one is for
    // the command to put it into loop folder mode and the second is when it
    // actually begins playing.
    enqueue(Message::ID::LOOPFOLDER, State2::EXPECT_ACK | State2::EXPECT_ACK2,
            folder);
}

void SerialAudio2::queryCurrentFile(Device /*device*/) {
//    auto msgid = Message::ID::NONE;
//    switch (device) {
//        case Device::USB:    msgid = Message::ID::CURRENTUSBFILE;   break;
//        case Device::SDCARD: msgid = Message::ID::CURRENTSDFILE;    break;
//        case Device::FLASH:  msgid = Message::ID::CURRENTFLASHFILE; break;
//        default: return;
//    }
//    enqueue(msgid, Feedback::NO_FEEDBACK, State::RESPONSEPENDING, 100);
}

void SerialAudio2::queryPlaybackSequence() {
    enqueue(Message::ID::PLAYBACKSEQUENCE, State2::EXPECT_RESPONSE);
}


void SerialAudio2::stop() {
    enqueue(Message::ID::STOP, State2::EXPECT_ACK);
}

void SerialAudio2::pause() {
    enqueue(Message::ID::PAUSE, State2::EXPECT_ACK);
}

void SerialAudio2::unpause() {
    enqueue(Message::ID::UNPAUSE, State2::EXPECT_ACK);
}

void SerialAudio2::insertAdvert(uint16_t /*track*/) {
//    enqueue(Message::ID::INSERTADVERT, track);
}

void SerialAudio2::insertAdvert(uint8_t /*folder*/, uint8_t /*track*/) {
//    if (folder == 0) return insertAdvert(track);
//    enqueue(Message::ID::INSERTADVERTN, combine(folder, track));
}

void SerialAudio2::stopAdvert() {
    enqueue(Message::ID::STOPADVERT, State2::EXPECT_ACK);
}

void SerialAudio2::dispatch() {
    if (!m_state.ready()) return;
    if (m_queue.empty()) return;
    dispatch(m_queue.peekFront());
    m_queue.popFront();
}

void SerialAudio2::dispatch(Message::ID msgid, State2::Flag flags, uint16_t data) {
    dispatch(Command2{State2{msgid, flags}, data});
}

void SerialAudio2::dispatch(Command2 const &cmd) {
    auto const feedback =
        cmd.state.has(State2::EXPECT_ACK) ? Feedback::FEEDBACK :
                                            Feedback::NO_FEEDBACK;
    m_core.send(Message{cmd.state.sent(), cmd.param}, feedback);
    m_state = cmd.state;
    unsigned const duration =
        m_state.has(State2::EXPECT_ACK)      ? 30  :
        m_state.has(State2::EXPECT_RESPONSE) ? 100 : 0;
    m_timeout.set(duration);
}

void SerialAudio2::enqueue(Message::ID msgid, State2::Flag flags, uint16_t data) {
    m_queue.pushBack(Command2{State2{msgid, flags}, data});
    if (m_state.ready()) dispatch();
}

void SerialAudio2::onEvent(Message const &msg, Hooks *hooks) {
    Serial.print(F("onEvent ("));
    Serial.print(static_cast<uint8_t>(msg.getID()), HEX);
    Serial.print(F(", "));
    Serial.print(msg.getParam(), HEX);
    Serial.print(F(") State="));
    Serial.print(static_cast<uint8_t>(m_state.sent()), HEX);
    if (m_state.has(State2::EXPECT_ACK))       Serial.print(F(" | EXPECT_ACK"));
    if (m_state.has(State2::EXPECT_ACK2))      Serial.print(F(" | EXPECT_ACK2"));
    if (m_state.has(State2::EXPECT_RESPONSE))  Serial.print(F(" | EXPECT_RESPONSE"));
    if (m_state.has(State2::DELAY))            Serial.print(F(" | DELAY"));
    if (m_state.has(State2::UNINITIALIZED))    Serial.print(F(" | UNINITIALIZED"));
    Serial.println();
    
    handleEvent(msg, hooks);
    
    // We might be ready to dispatch a queued command now.
    dispatch();
}

void SerialAudio2::handleEvent(Message const &msg, Hooks *hooks) {
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
        if (m_state.testAndClear(State2::EXPECT_ACK)) {
            m_timeout.cancel();
            if (m_state.hasAny(State2::EXPECT_ACK2 | State2::DELAY)) {
                m_timeout.set(300);
            }
            return;
        }
        if (m_state.testAndClear(State2::EXPECT_ACK2)) {
            m_timeout.cancel();
            return;
        }
        Serial.println(F("Unexpected ACK!"));
        return;
    }

    if (isInitComplete(msg)) {
        if (m_state.poweringUp()) {
            Serial.println(F("Got INITCOMPLETE on power up"));
            m_state.clear(State2::UNINITIALIZED);
            m_timeout.cancel();
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        if (m_state.sent() == ID::RESET) {
            Serial.println(F("Reset completed"));
            m_state.clear(State2::UNINITIALIZED);
            m_timeout.cancel();
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        if (m_state.sent() == ID::INITCOMPLETE) {
            Serial.println(F("OMG! INITCOMPLETE worked as a query!"));
            m_state.clear(State2::EXPECT_RESPONSE);
            m_timeout.cancel();
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        Serial.println(F("Audio module unexpectedly reset!"));
        m_state = State2{Message:ID::NONE};
        m_timeout.cancel();
        m_queue.clear();
        m_lastNotification.clear();
        if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
        return;
    }
    
    if (isQueryResponse(msg)) {
        if (m_state.has(State2::EXPECT_RESPONSE)) {
            if (msg.getID() == m_state.sent()) {
                Serial.println(F("Received expected query response"));
                m_timeout.cancel();
                m_state.clear(State2::EXPECT_RESPONSE);
                if (msg.getID() == ID::STATUS && m_state.has(State2::UNINITIALIZED)) {
                    // A valid response to a status query means we've detected a
                    // live audio module after powerup.
                    m_state.clear(State2::UNINITIALIZED);
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
        if (m_state.testAndClear(State2::DELAY)) {
            Serial.println(F("Delay completed"));
            m_timeout.cancel();
            return;
        }
        if (m_state.poweringUp()) {
            Serial.println(F("Didn't receive INITCOMPLETE on powerup."));
            // Let's try querying the module to see whether it's already running.
            dispatch(ID::STATUS, State2::EXPECT_RESPONSE | State2::UNINITIALIZED);
            return;
        }
        // Fall through to the general error handler.
    }
    
    if (isError(msg)) {
        m_timeout.cancel();
        m_state.clear(State2::ALL_FLAGS);
        if (hooks != nullptr) {
            hooks->onError(static_cast<SerialAudio2::Error>(msg.getParam()));
        }
        return;
    }
}

void SerialAudio2::onPowerUp() {
    // I used to just hit the module with a reset command on powerup, but
    // documentation and experience suggests that the device might not tolerate
    // a reset while already resetting.  So we'll assume the device is powering
    // up and wait for an initialization complete (0x3F) notification.  If one
    // doesn't come, the state machine will fall back to figuring out whether
    // the module is already online and to discover what devices are attached.
    m_queue.clear();
    m_lastNotification.clear();
    m_state = State2();
    m_timeout.set(3000);
    Serial.println(F("onPowerUp: Waiting for INITCOMPLETE."));
}

}
