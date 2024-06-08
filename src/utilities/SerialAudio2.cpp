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
    switch (device) {
        case Device::USB:    /*enqueue(Message::ID::USBFILECOUNT);   */ break;
        case Device::SDCARD: /*enqueue(Message::ID::SDFILECOUNT);    */ break;
        case Device::FLASH:  /*enqueue(Message::ID::FLASHFILECOUNT); */ break;
        default: break;
    }
}

void SerialAudio2::queryFirmwareVersion() {
//    enqueue(Message::ID::FIRMWAREVERSION, Feedback::NO_FEEDBACK,
//            State::RESPONSEPENDING, 100);
}

void SerialAudio2::selectSource(Device /*source*/) {
//    auto const paramLo = static_cast<uint8_t>(source);
//    enqueue(Message::ID::SELECTSOURCE, combine(0, paramLo), Feedback::FEEDBACK,
//            State::SOURCEACKPENDING, 30);
}

void SerialAudio2::queryStatus() {
//    enqueue(Message::ID::STATUS, Feedback::NO_FEEDBACK,
//            State::RESPONSEPENDING, 100);
}

void SerialAudio2::setVolume(uint8_t /*volume*/) {
//    volume = min(volume, 30);
//    enqueue(Message::ID::SETVOLUME, volume);
}

void SerialAudio2::increaseVolume() {
//    enqueue(Message::ID::VOLUMEUP);
}

void SerialAudio2::decreaseVolume() {
//    enqueue(Message::ID::VOLUMEDOWN);
}

void SerialAudio2::queryVolume() {
//    enqueue(Message::ID::VOLUME, Feedback::NO_FEEDBACK,
//            State::RESPONSEPENDING, 100);
}

void SerialAudio2::setEqProfile(EqProfile /*eq*/) {
//    enqueue(Message::ID::SETEQPROFILE, static_cast<uint16_t>(eq));
}

void SerialAudio2::queryEqProfile() {
//    enqueue(Message::ID::EQPROFILE, Feedback::NO_FEEDBACK,
//            State::RESPONSEPENDING, 100);
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
//    enqueue(Message::ID::FOLDERCOUNT, Feedback::NO_FEEDBACK,
//            State::RESPONSEPENDING, 100);
}

void SerialAudio2::playTrack(uint16_t /*track*/) {
//    enqueue(Message::ID::PLAYFROMMP3, track);
}

void SerialAudio2::playTrack(uint16_t /*folder*/, uint16_t /*track*/) {
//    if (track < 256) {
//        auto const param = combine(
//            static_cast<uint8_t>(folder),
//            static_cast<uint8_t>(track)
//        );
//        enqueue(Message::ID::PLAYFROMFOLDER, param);
//    } else if (folder < 16) {
//        auto const param = ((folder & 0x0F) << 12) | (track & 0x0FFF);
//        enqueue(Message::ID::PLAYFROMBIGFOLDER, param);
//    }
}

void SerialAudio2::loopFolder(uint16_t /*folder*/) {
    // Note the longer timeout because the modules take a bit longer.  I think
    // the module checks the filesystem for the folder first.
    // Also note this command ACKs twice when successful.  I think one is for
    // the command to put it into loop folder mode and the second is when it
    // actually begins playing.
//    enqueue(Message::ID::LOOPFOLDER, folder, Feedback::FEEDBACK,
//            State::EXTRAACKPENDING, 100);
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
//    enqueue(Message::ID::PLAYBACKSEQUENCE, Feedback::NO_FEEDBACK,
//            State::RESPONSEPENDING, 100);
}


void SerialAudio2::stop() {
//    enqueue(Message::ID::STOP);
}

void SerialAudio2::pause() {
//    enqueue(Message::ID::PAUSE);
}

void SerialAudio2::unpause() {
//    enqueue(Message::ID::UNPAUSE);
}


void SerialAudio2::insertAdvert(uint16_t /*track*/) {
//    enqueue(Message::ID::INSERTADVERT, track);
}

void SerialAudio2::insertAdvert(uint8_t /*folder*/, uint8_t /*track*/) {
//    if (folder == 0) return insertAdvert(track);
//    enqueue(Message::ID::INSERTADVERTN, combine(folder, track));
}

void SerialAudio2::stopAdvert() {
//    enqueue(Message::ID::STOPADVERT);
}

void SerialAudio2::dispatch() {
    if (m_queue.empty()) return;
    dispatch(m_queue.peekFront());
    m_queue.popFront();
}

void SerialAudio2::dispatch(Command2 const &cmd) {
    m_state = cmd.state;
    auto const msgid = static_cast<Message::ID>(m_state & MSGID_MASK);
    sendMessage(Message{msgid, cmd.param}, (m_state & EXPECT_ACK) ? Feedback::FEEDBACK : Feedback::NO_FEEDBACK);
    m_timeout.set(100);
}

void SerialAudio2::onEvent(Message const &msg, Hooks *hooks) {
    using ID = Message::ID;

    Serial.print(F("onEvent ("));
    Serial.print(static_cast<uint8_t>(msg.getID()), HEX);
    Serial.print(F(", "));
    Serial.print(msg.getParam(), HEX);
    Serial.print(F(") State="));
    Serial.print(m_state & MSGID_MASK, HEX);
    if (m_state & EXPECT_ACK)       Serial.print(F(" | EXPECT_ACK"));
    if (m_state & EXPECT_ACK2)      Serial.print(F(" | EXPECT_ACK2"));
    if (m_state & EXPECT_RESPONSE)  Serial.print(F(" | EXPECT_RESPONSE"));
    if (m_state & DELAY)            Serial.print(F(" | DELAY"));
    Serial.println();

    if (isAsyncNotification(msg)) {
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
    
    if (isAck(msg)) {
        if (m_state & EXPECT_ACK2) {
            m_state &= ~EXPECT_ACK2;
            m_timeout.set(100);  // set longer timeout for second ACK
            return;
        }
        if (m_state & EXPECT_ACK) {
            m_state &= ~EXPECT_ACK;
            m_timeout.cancel();
            return;
        }
    }

    if (isInitComplete(msg)) {
        if (m_state == POWERING_UP) {
            Serial.println(F("Got INITCOMPLETE on power up"));
            m_timeout.cancel();
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        if (lastSent() == ID::RESET) {
            Serial.println(F("Reset completed"));
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        if (lastSent() == ID::INITCOMPLETE) {
            Serial.println(F("OMG! INITCOMPLETE worked as a query!"));
            m_state &= ~EXPECT_RESPONSE;
            if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));
            return;
        }
        Serial.println(F("Audio module unexpectedly reset!"));
        m_queue.clear();
        if (hooks != nullptr) hooks->onInitComplete(LSB(msg.getParam()));

        return;
    }
    
    if (isTimeout(msg)) {
        m_timeout.cancel();
        if (m_state & DELAY) {
            Serial.println(F("Delay completed"));
            m_state &= ~DELAY;
            return;
        }
        if (m_state == POWERING_UP) {
            Serial.println(F("Didn't receive INITCOMPLETE on powerup."));
            dispatch(Command2{static_cast<uint16_t>(ID::STATUS) | EXPECT_RESPONSE, 0});
            return;
        }
    }
    
    if (isQueryResponse(msg)) {
        if (m_state & EXPECT_RESPONSE) {
            if (static_cast<uint8_t>(msg.getID()) == (m_state & MSGID_MASK)) {
                Serial.println(F("Received expected query response"));
                m_timeout.cancel();
                m_state &= ~EXPECT_RESPONSE;
                if (hooks != nullptr) {
                    hooks->onQueryResponse(static_cast<Parameter>(msg.getID()), msg.getParam());
                }
                return;
            }
            Serial.println(F("Got different query response than expected"));
            return;
        }
        Serial.println(F("Got unexpected query response!"));
        return;
    }
    
    if (isError(msg)) {
        m_timeout.cancel();
        m_state &= MSGID_MASK;
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
    m_state = POWERING_UP;
    m_timeout.set(3000);
    Serial.println(F("onPowerUp: Waiting for INITCOMPLETE."));
}

void SerialAudio2::sendMessage(Message const &msg, Feedback feedback) {
    m_core.send(msg, feedback);
    m_state = (m_state & FLAGS_MASK) | static_cast<uint8_t>(msg.getID());
}

}
