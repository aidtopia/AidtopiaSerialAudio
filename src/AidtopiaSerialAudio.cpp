#include <Arduino.h>
#include "AidtopiaSerialAudio.h"
#include "utilities/message.h"

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

SerialAudio::Devices::Devices() : m_bitmask(0) {}

SerialAudio::Devices::Devices(SerialAudio::Device device) :
    m_bitmask(static_cast<uint8_t>(device)) {}

SerialAudio::Devices::Devices(uint8_t bitmask) : m_bitmask(bitmask) {}

SerialAudio::Devices &SerialAudio::Devices::operator|=(SerialAudio::Devices const &rhs) {
    m_bitmask |= rhs.m_bitmask;
    return *this;
}

bool SerialAudio::Devices::empty() const {
    return m_bitmask == 0;
}

bool SerialAudio::Devices::has(Device device) const {
    auto const bit = static_cast<uint8_t>(device);
    return (m_bitmask & bit) == bit;
}

bool SerialAudio::Devices::hasAny(Devices devices) const {
    return (m_bitmask & devices.m_bitmask) != 0;
}

void SerialAudio::Devices::clear() {
    m_bitmask = 0;
}

void SerialAudio::Devices::insert(Device device) {
    auto const bit = static_cast<uint8_t>(device);
    m_bitmask |= bit;
}

void SerialAudio::Devices::remove(Device device) {
    auto const bit = static_cast<uint8_t>(device);
    m_bitmask &= ~bit;
}

SerialAudio::Devices operator|(SerialAudio::Device d1, SerialAudio::Device d2) {
    return SerialAudio::Devices{d1} |= SerialAudio::Devices{d2};
}

SerialAudio::Devices operator|(SerialAudio::Device d1, SerialAudio::Devices d2) {
    return SerialAudio::Devices{d1} |= d2;
}

SerialAudio::Devices operator|(SerialAudio::Devices d1, SerialAudio::Device d2) {
    return d1 |= SerialAudio::Devices{d2};
}

SerialAudio::Devices operator|(SerialAudio::Devices d1, SerialAudio::Devices d2) {
    return d1 |= d2;
}


bool SerialAudio::update()             { return update(nullptr); }
bool SerialAudio::update(Hooks &hooks) { return update(&hooks);  }

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

// The non-virtual interface is just a wrapper for the overridable hook methods.
void SerialAudio::Hooks::handleError(Error error) {
    onError(error);
}

void SerialAudio::Hooks::handleQueryResponse(Parameter param, uint16_t value) {
    onQueryResponse(param, value);
}

void SerialAudio::Hooks::handleDeviceChange(Device device, DeviceChange change) {
    onDeviceChange(device, change);
}

void SerialAudio::Hooks::handleFinishedFile(Device device, uint16_t index) {
    if (device == m_deviceLastFinished && index == m_indexLastFinished) {
        Serial.println(F("Duplicate notification suppressed."));
        m_deviceLastFinished = Device::NONE;
        m_indexLastFinished = 0;
        return;
    }
    onFinishedFile(device, index);
    m_deviceLastFinished = device;
    m_indexLastFinished = index;
}

void SerialAudio::Hooks::handleInitComplete(Devices devices) {
    onInitComplete(devices);
    m_deviceLastFinished = Device::NONE;
    m_indexLastFinished = 0;
}

// Unless a subclass provides overrides, the hooks do nothing.
void SerialAudio::Hooks::onError(Error) {}
void SerialAudio::Hooks::onQueryResponse(Parameter, uint16_t) {}
void SerialAudio::Hooks::onDeviceChange(Device, DeviceChange) {}
void SerialAudio::Hooks::onFinishedFile(Device, uint16_t) {}
void SerialAudio::Hooks::onInitComplete(Devices) {}

void SerialAudio::reset() {
    m_queue.clear();
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

void SerialAudio::queryFolderCount() {
    enqueue(Message::ID::FOLDERCOUNT, State::EXPECT_RESPONSE);
}

void SerialAudio::queryFolderFileCount(uint16_t folder) {
    enqueue(Message::ID::FOLDERFILECOUNT, State::EXPECT_RESPONSE, folder);
}

void SerialAudio::loopFolder(uint16_t folder) {
    // Note that this command ACKs twice when successful.  I think one is for
    // the command to put it into loop folder mode and the second is when it
    // actually begins playing.
    enqueue(Message::ID::LOOPFOLDER, State::EXPECT_ACK | State::EXPECT_ACK2,
            folder);
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
            switch (msg.getID()) {
                case ID::DEVICEINSERTED: {
                    auto const device = static_cast<Device>(msg.getParam());
                    m_tocheck.insert(device);
                    hooks->handleDeviceChange(device, DeviceChange::INSERTED);
                    break;
                }
                case ID::DEVICEREMOVED: {
                    auto const device = static_cast<Device>(msg.getParam());
                    m_available.remove(device);
                    m_tocheck.remove(device);
                    hooks->handleDeviceChange(device, DeviceChange::REMOVED);
                    break;
                }
                case ID::FINISHEDUSBFILE:
                    hooks->handleFinishedFile(Device::USB, msg.getParam());
                    break;
                case ID::FINISHEDSDFILE:
                    hooks->handleFinishedFile(Device::SDCARD, msg.getParam());
                    break;
                case ID::FINISHEDFLASHFILE:
                    hooks->handleFinishedFile(Device::FLASH, msg.getParam());
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
            if (hooks != nullptr) {
                hooks->handleInitComplete(Devices(LSB(msg.getParam())));
            }
            return;
        }
        if (m_state.sent() == ID::RESET) {
            Serial.println(F("Reset completed"));
            m_state.clear(State::UNINITIALIZED);
            m_timeout.cancel();
            if (hooks != nullptr) {
                hooks->handleInitComplete(Devices(LSB(msg.getParam())));
            }
            return;
        }
        if (m_state.sent() == ID::INITCOMPLETE) {
            Serial.println(F("OMG! INITCOMPLETE worked as a query!"));
            m_state.clear(State::EXPECT_RESPONSE);
            m_timeout.cancel();
            if (hooks != nullptr) {
                hooks->handleInitComplete(Devices(LSB(msg.getParam())));
            }
            return;
        }
        Serial.println(F("Audio module unexpectedly reset!"));
        m_state = State{Message:ID::NONE};
        m_timeout.cancel();
        m_queue.clear();
        if (hooks != nullptr) {
            hooks->handleInitComplete(Devices(LSB(msg.getParam())));
        }
        return;
    }
    
    if (isQueryResponse(msg)) {
        if (m_state.has(State::EXPECT_RESPONSE)) {
            if (msg.getID() == m_state.sent()) {
                Serial.println(F("Received expected query response"));
                m_timeout.cancel();
                m_state.clear(State::EXPECT_RESPONSE);
                if (m_state.has(State::UNINITIALIZED)) {
                    switch (msg.getID()) {
                        case ID::STATUS: {
                            // A response to a status query while we're
                            // uninitialized means we've detected a live audio
                            // module after powerup.
                            if (hooks != nullptr) {
                                // The device given in the status message is
                                // currently selected.  We'll assume that means
                                // it's available, and we'll try to discover any
                                // other available devices before reporting that
                                // the device is initialized.
                                Device const device = Device(MSB(msg.getParam()));
                                m_tocheck = Device::USB | Device::SDCARD | Device::FLASH;
                                m_tocheck.remove(device);
                                m_available = Devices(device);
                                if (discoveryContinues()) return;
                                hooks->handleInitComplete(m_available);
                            }
                            return;
                        }

                        // A file count while we're uninitialized means we are
                        // trying to discover which devices are available.

                        case ID::USBFILECOUNT: {
                            if (msg.getParam() > 0) m_available |= Device::USB;
                            if (discoveryContinues()) return;
                            if (hooks != nullptr) {
                                hooks->handleInitComplete(m_available);
                            }
                            return;
                        }
                        case ID::SDFILECOUNT: {
                            if (msg.getParam() > 0) m_available |= Device::SDCARD;
                            if (discoveryContinues()) return;
                            if (hooks != nullptr) {
                                hooks->handleInitComplete(m_available);
                            }
                            return;
                        }
                        case ID::FLASHFILECOUNT: {
                            if (msg.getParam() > 0) m_available |= Device::FLASH;
                            if (discoveryContinues()) return;
                            if (hooks != nullptr) {
                                hooks->handleInitComplete(m_available);
                            }
                            return;
                        }

                        default:  // Uh, TBD?
                            return;
                    }
                }
                if (hooks != nullptr) {
                    auto const param = static_cast<Parameter>(msg.getID());
                    hooks->handleQueryResponse(param, msg.getParam());
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
            hooks->handleError(static_cast<SerialAudio::Error>(msg.getParam()));
        }
        return;
    }
}

bool SerialAudio::discoveryContinues() {
    auto const discoveryState =
        State::EXPECT_RESPONSE | State::UNINITIALIZED;
    if (m_tocheck.has(Device::USB)) {
        Serial.println(F("Discovery tries USB."));
        m_tocheck.remove(Device::USB);
        dispatch(Message::ID::USBFILECOUNT, discoveryState);
        return true;
    }
    if (m_tocheck.has(Device::SDCARD)) {
        Serial.println(F("Discovery tries SD card."));
        m_tocheck.remove(Device::SDCARD);
        dispatch(Message::ID::SDFILECOUNT, discoveryState);
        return true;
    }
    if (m_tocheck.has(Device::FLASH)) {
        Serial.println(F("Discovery tries flash memory."));
        m_tocheck.remove(Device::FLASH);
        dispatch(Message::ID::FLASHFILECOUNT, discoveryState);
        return true;
    }
    m_state.clear(State::UNINITIALIZED);
    Serial.println(F("Discovery finished."));
    return false;
}

void SerialAudio::onPowerUp() {
    // I used to just hit the module with a reset command on powerup, but
    // documentation and experience suggests that the device might not tolerate
    // a reset while already resetting.  So we'll assume the device is powering
    // up and wait for an initialization complete (0x3F) notification.  If one
    // doesn't come, the state machine will fall back to figuring out whether
    // the module is already online and which devices are attached.
    m_queue.clear();
    m_state = State();
    m_timeout.set(3000);
    Serial.println(F("onPowerUp: Waiting for INITCOMPLETE."));
}

}
