// AidtopiaSerialAudio
// Adrian McCarthy 2018-

// A library that works with various serial audio modules,
// like DFPlayer Mini, Catalex, etc.

#include <Arduino.h>
#include <AidtopiaSerialAudio.h>

static constexpr uint16_t combine(uint8_t hi, uint8_t lo) {
  return static_cast<uint16_t>(hi << 8) | lo;
}
static constexpr uint8_t high(uint16_t x) { return x >> 8; }
static constexpr uint8_t low(uint16_t x)  { return x & 0xFF; }


Aidtopia_SerialAudio::Aidtopia_SerialAudio() :
  m_stream(nullptr), m_in(), m_out(), m_state(nullptr), m_timeout() {}

void Aidtopia_SerialAudio::update() {
  checkForIncomingMessage();
  checkForTimeout();
}

void Aidtopia_SerialAudio::reset() {
  setState(&s_init_resetting_hardware);
}

void Aidtopia_SerialAudio::selectSource(Device device) {
  switch (device) {
    case DEV_USB:    sendCommand(MID_SELECTSOURCE, 1); break;
    case DEV_SDCARD: sendCommand(MID_SELECTSOURCE, 2); break;
    case DEV_FLASH:  sendCommand(MID_SELECTSOURCE, 5); break;
    default: break;
  }
}

void Aidtopia_SerialAudio::playFile(uint16_t file_index) {
  sendCommand(MID_PLAYFILE, file_index);
}

void Aidtopia_SerialAudio::playNextFile() {
  sendCommand(MID_PLAYNEXT);
}

void Aidtopia_SerialAudio::playPreviousFile() {
  sendCommand(MID_PLAYPREVIOUS);
}

void Aidtopia_SerialAudio::loopFile(uint16_t file_index) {
  sendCommand(MID_LOOPFILE, file_index);
}

void Aidtopia_SerialAudio::loopAllFiles() {
  sendCommand(MID_LOOPALL, 1);
}

void Aidtopia_SerialAudio::playFilesInRandomOrder() {
  sendCommand(MID_RANDOMPLAY);
}

void Aidtopia_SerialAudio::playTrack(uint16_t folder, uint16_t track) {
  // Under the hood, there are a couple different command
  // messages to achieve this. We'll automatically select the
  // most appropriate one based on the values.
  if (track < 256) {
    const uint16_t param = (folder << 8) | track;
    sendCommand(MID_PLAYFROMFOLDER, param);
  } else if (folder < 16 && track <= 3000) {
    // For folders with more than 255 tracks, we have this
    // alternative command.
    const uint16_t param = (folder << 12) | track;
    sendCommand(MID_PLAYFROMBIGFOLDER, param);
  }
}

void Aidtopia_SerialAudio::playTrack(uint16_t track) {
  sendCommand(MID_PLAYFROMMP3, track);
}

void Aidtopia_SerialAudio::insertAdvert(uint16_t track) {
  sendCommand(MID_INSERTADVERT, track);
}

void Aidtopia_SerialAudio::stopAdvert() {
  sendCommand(MID_STOPADVERT);
}

void Aidtopia_SerialAudio::stop() {
  sendCommand(MID_STOP);
}

void Aidtopia_SerialAudio::pause() {
  sendCommand(MID_PAUSE);
}

void Aidtopia_SerialAudio::unpause() {
  sendCommand(MID_UNPAUSE);
}

void Aidtopia_SerialAudio::setVolume(int volume) {
  // Catalex effectively goes to 31, but it doesn't automatically
  // clamp values.  DF Player Mini goes to 30 and clamps there.
  // We'll make them behave the same way.
  if (volume < 0) volume = 0;
  if (30 < volume) volume = 30;
  sendCommand(MID_SETVOLUME, static_cast<uint16_t>(volume));
}

void Aidtopia_SerialAudio::selectEQ(Equalizer eq) {
  sendCommand(MID_SELECTEQ, eq);
}

void Aidtopia_SerialAudio::sleep() {
  sendCommand(MID_SLEEP);
}

void Aidtopia_SerialAudio::wake() {
  sendCommand(MID_WAKE);
}

void Aidtopia_SerialAudio::disableDACs() {
  sendCommand(MID_DISABLEDAC, 1);
}

void Aidtopia_SerialAudio::enableDACs() {
  sendCommand(MID_DISABLEDAC, 0);
}

void Aidtopia_SerialAudio::queryFileCount(Device device) {
  switch (device) {
    case DEV_USB:    sendQuery(MID_USBFILECOUNT);   break;
    case DEV_SDCARD: sendQuery(MID_SDFILECOUNT);    break;
    case DEV_FLASH:  sendQuery(MID_FLASHFILECOUNT); break;
    default: break;
  }
}

void Aidtopia_SerialAudio::queryCurrentFile(Device device) {
  switch (device) {
    case DEV_USB:    sendQuery(MID_CURRENTUSBFILE);   break;
    case DEV_SDCARD: sendQuery(MID_CURRENTSDFILE);    break;
    case DEV_FLASH:  sendQuery(MID_CURRENTFLASHFILE); break;
    default: break;
  }
}

void Aidtopia_SerialAudio::queryFolderCount() {
  sendQuery(MID_FOLDERCOUNT);
}

void Aidtopia_SerialAudio::queryStatus() {
  sendQuery(MID_STATUS);
}

void Aidtopia_SerialAudio::queryVolume() {
  sendQuery(MID_VOLUME);
}

void Aidtopia_SerialAudio::queryEQ() {
  sendQuery(MID_EQ);
}

void Aidtopia_SerialAudio::queryPlaybackSequence() {
  sendQuery(MID_PLAYBACKSEQUENCE);
}

void Aidtopia_SerialAudio::queryFirmwareVersion() {
  sendQuery(MID_FIRMWAREVERSION);
}



Aidtopia_SerialAudio::Message::Message() :
  m_buf{START, VERSION, LENGTH, 0, FEEDBACK, 0, 0, 0, 0, END},
  m_length(0) {}

void Aidtopia_SerialAudio::Message::set(
    MsgID msgid, uint16_t param, Feedback feedback
) {
  // Note that we're filling in just the bytes that change.  We rely
  // on the framing bytes set when the buffer was first initialized.
  m_buf[3] = msgid;
  m_buf[4] = feedback;
  m_buf[5] = (param >> 8) & 0xFF;
  m_buf[6] = (param     ) & 0xFF;
  const uint16_t checksum = ~sum() + 1;
  m_buf[7] = (checksum >> 8) & 0xFF;
  m_buf[8] = (checksum     ) & 0xFF;
  m_length = 10;
}

const uint8_t *Aidtopia_SerialAudio::Message::getBuffer() const {
  return m_buf;
}

int Aidtopia_SerialAudio::Message::getLength() const {
  return m_length;
}

bool Aidtopia_SerialAudio::Message::isValid() const {
  if (m_length == 8 && m_buf[7] == END) return true;
  if (m_length != 10) return false;
  const uint16_t checksum = combine(m_buf[7], m_buf[8]);
  return sum() + checksum == 0;
}

Aidtopia_SerialAudio::MsgID Aidtopia_SerialAudio::Message::getMessageID() const {
  return static_cast<MsgID>(m_buf[3]);
}

uint8_t Aidtopia_SerialAudio::Message::getParamHi() const { return m_buf[5]; }

uint8_t Aidtopia_SerialAudio::Message::getParamLo() const { return m_buf[6]; }

uint16_t Aidtopia_SerialAudio::Message::getParam() const {
  return combine(m_buf[5], m_buf[6]);
}

bool Aidtopia_SerialAudio::Message::receive(uint8_t b) {
  switch (m_length) {
    default:
      // `m_length` is out of bounds, so start fresh.
      m_length = 0;
      /* FALLTHROUGH */
    case 0: case 1: case 2: case 9:
      // These bytes must always match the template.
      if (b == m_buf[m_length]) { ++m_length; return m_length == 10; }
      // No match; try to resync.
      if (b == START) { m_length = 1; return false; }
      m_length = 0;
      return false;
    case 7:
      // If there's no checksum, the message may end here.
      if (b == END) { m_length = 8; return true; }
      /* FALLTHROUGH */
    case 3: case 4: case 5: case 6: case 8:
      // These are the payload bytes we care about.
      m_buf[m_length++] = b;
      return false;
  }
}

uint16_t Aidtopia_SerialAudio::Message::sum() const {
  uint16_t s = 0;
  for (int i = 1; i <= LENGTH; ++i) {
    s += m_buf[i];
  }
  return s;
}

// HOOKS
void Aidtopia_SerialAudio::onAck() {
  Serial.println(F("ACK"));
}

void Aidtopia_SerialAudio::onCurrentTrack(Device device, uint16_t file_index) {
  printDeviceName(device);
  Serial.print(F(" current file index: "));
  Serial.println(file_index);
}

void Aidtopia_SerialAudio::onDeviceInserted(Device src) {
  Serial.print(F("Device inserted: "));
  printDeviceName(src);
  Serial.println();
}

void Aidtopia_SerialAudio::onDeviceRemoved(Device src) {
  printDeviceName(src);
  Serial.println(F(" removed."));
}

void Aidtopia_SerialAudio::onEqualizer(Equalizer eq) {
  Serial.print(F("Equalizer: "));
  printEqualizerName(eq);
  Serial.println();
}

void Aidtopia_SerialAudio::onError(uint16_t code) {
  Serial.print(F("Error "));
  Serial.print(code);
  Serial.print(F(": "));
  switch (code) {
    case 0x00: Serial.println(F("Unsupported command")); break;
    case 0x01: Serial.println(F("Module busy or no sources available")); break;
    case 0x02: Serial.println(F("Module sleeping")); break;
    case 0x03: Serial.println(F("Serial communication error")); break;
    case 0x04: Serial.println(F("Bad checksum")); break;
    case 0x05: Serial.println(F("File index out of range")); break;
    case 0x06: Serial.println(F("Track not found")); break;
    case 0x07: Serial.println(F("Insertion error")); break;
    case 0x08: Serial.println(F("SD card error")); break;
    case 0x0A: Serial.println(F("Entered sleep mode")); break;
    case 0x100: Serial.println(F("Timed out")); break;
    default:   Serial.println(F("Unknown error code")); break;
  }
}

void Aidtopia_SerialAudio::onDeviceFileCount(Device device, uint16_t count) {
  printDeviceName(device);
  Serial.print(F(" file count: "));
  Serial.println(count);
}

// Note that this hook receives a file index, even if the track
// was initialized using something other than its file index.
//
// The module sometimes sends these multiple times in quick
// succession.
//
// This hook does not trigger when the playback is stopped, only
// when a track finishes playing on its own.
//
// This hook does not trigger when an inserted track finishes.
// If you need to know that, you can try watching for a brief
// blink on the BUSY pin of the DF Player Mini.
void Aidtopia_SerialAudio::onFinishedFile(Device device, uint16_t file_index) {
  Serial.print(F("Finished playing file: "));
  printDeviceName(device);
  Serial.print(F(" "));
  Serial.println(file_index);
}

void Aidtopia_SerialAudio::onFirmwareVersion(uint16_t version) {
  Serial.print(F("Firmware Version: "));
  Serial.println(version);
}

void Aidtopia_SerialAudio::onFolderCount(uint16_t count) {
  Serial.print(F("Folder count: "));
  Serial.println(count);
}

void Aidtopia_SerialAudio::onFolderTrackCount(uint16_t count) {
  Serial.print(F("Folder track count: "));
  Serial.println(count);
}

void Aidtopia_SerialAudio::onInitComplete(uint8_t devices) {
  Serial.print(F("Hardware initialization complete.  Device(s) online:"));
  if (devices & (1u << DEV_SDCARD)) Serial.print(F(" SD Card"));
  if (devices & (1u << DEV_USB))    Serial.print(F(" USB"));
  if (devices & (1u << DEV_AUX))    Serial.print(F(" AUX"));
  if (devices & (1u << DEV_FLASH))  Serial.print(F(" Flash"));
  Serial.println();
}

void Aidtopia_SerialAudio::onMessageInvalid() {
  Serial.println(F("Invalid message received."));
}

void Aidtopia_SerialAudio::onMessageReceived([[maybe_unused]] const Message &msg) {
#if 0
  const auto *buf = msg.getBuffer();
  const auto len = msg.getLength();
  Serial.print(F("Received:"));
  for (int i = 0; i < len; ++i) {
    Serial.print(F(" "));
    Serial.print(buf[i], HEX);
  }
  Serial.println();
#endif
}

void Aidtopia_SerialAudio::onMessageSent(const uint8_t * /* buf */, int /* len */) {
#if 0
  Serial.print(F("Sent:    "));
  for (int i = 0; i < len; ++i) {
    Serial.print(F(" "));
    Serial.print(buf[i], HEX);
  }
  Serial.println();
#endif
}

void Aidtopia_SerialAudio::onPlaybackSequence(Sequence seq) {
  Serial.print(F("Playback Sequence: "));
  printSequenceName(seq);
  Serial.println();
}

void Aidtopia_SerialAudio::onStatus(Device device, ModuleState state) {
  Serial.print(F("State: "));
  if (device != DEV_SLEEP) {
    printDeviceName(device);
    Serial.print(F(" "));
  }
  printModuleStateName(state);
  Serial.println();
}

void Aidtopia_SerialAudio::onVolume(uint8_t volume) {
  Serial.print(F("Volume: "));
  Serial.println(volume);
}

void Aidtopia_SerialAudio::printDeviceName(Device src) {
  switch (src) {
    case DEV_USB:    Serial.print(F("USB")); break;
    case DEV_SDCARD: Serial.print(F("SD Card")); break;
    case DEV_AUX:    Serial.print(F("AUX")); break;
    case DEV_SLEEP:  Serial.print(F("SLEEP (does this make sense)")); break;
    case DEV_FLASH:  Serial.print(F("FLASH")); break;
    default:         Serial.print(F("Unknown Device")); break;
  }
}

void Aidtopia_SerialAudio::printEqualizerName(Equalizer eq) {
  switch (eq) {
    case EQ_NORMAL:    Serial.print(F("Normal"));    break;
    case EQ_POP:       Serial.print(F("Pop"));       break;
    case EQ_ROCK:      Serial.print(F("Rock"));      break;
    case EQ_JAZZ:      Serial.print(F("Jazz"));      break;
    case EQ_CLASSICAL: Serial.print(F("Classical")); break;
    case EQ_BASS:      Serial.print(F("Bass"));      break;
    default:           Serial.print(F("Unknown EQ")); break;
  }
}

void Aidtopia_SerialAudio::printModuleStateName(ModuleState state) {
  switch (state) {
    case MS_STOPPED: Serial.print(F("Stopped")); break;
    case MS_PLAYING: Serial.print(F("Playing")); break;
    case MS_PAUSED:  Serial.print(F("Paused"));  break;
    case MS_ASLEEP:  Serial.print(F("Asleep"));  break;
    default:         Serial.print(F("???"));     break;
  }
}

void Aidtopia_SerialAudio::printSequenceName(Sequence seq) {
  switch (seq) {
    case SEQ_LOOPALL:    Serial.print(F("Loop All")); break;
    case SEQ_LOOPFOLDER: Serial.print(F("Loop Folder")); break;
    case SEQ_LOOPTRACK:  Serial.print(F("Loop Track")); break;
    case SEQ_RANDOM:     Serial.print(F("Random")); break;
    case SEQ_SINGLE:     Serial.print(F("Single")); break;
    default:             Serial.print(F("???")); break;
  }
}
// END OF HOOKS


void Aidtopia_SerialAudio::checkForIncomingMessage() {
  while (m_stream->available() > 0) {
    if (m_in.receive(m_stream->read())) {
      receiveMessage(m_in);
    }
  }
}

void Aidtopia_SerialAudio::checkForTimeout() {
  if (m_timeout.expired()) {
    m_timeout.cancel();
    if (m_state) {
      setState(m_state->onEvent(this, MID_ERROR, high(EC_TIMEDOUT), low(EC_TIMEDOUT)));
    }
  }
}

void Aidtopia_SerialAudio::receiveMessage(const Message &msg) {
  callHooks(msg);
  if (!m_state) return;
  setState(m_state->onEvent(this, msg.getMessageID(), msg.getParamHi(), msg.getParamLo()));
}

void Aidtopia_SerialAudio::callHooks(const Message &msg) {
  onMessageReceived(msg);
  if (!msg.isValid()) return onMessageInvalid();

  switch (msg.getMessageID()) {
    case 0x3A: {
      const auto mask = msg.getParamLo();
      if (mask & 0x01) onDeviceInserted(DEV_USB);
      if (mask & 0x02) onDeviceInserted(DEV_SDCARD);
      if (mask & 0x04) onDeviceInserted(DEV_AUX);
      return;
    }
    case 0x3B: {
      const auto mask = msg.getParamLo();
      if (mask & 0x01) onDeviceRemoved(DEV_USB);
      if (mask & 0x02) onDeviceRemoved(DEV_SDCARD);
      if (mask & 0x04) onDeviceRemoved(DEV_AUX);
      return;
    }
    case 0x3C: return onFinishedFile(DEV_USB, msg.getParam());
    case 0x3D: return onFinishedFile(DEV_SDCARD, msg.getParam());
    case 0x3E: return onFinishedFile(DEV_FLASH, msg.getParam());

    // Initialization complete
    case 0x3F: {
      uint16_t devices = 0;
      const auto mask = msg.getParamLo();
      if (mask & 0x01) devices = devices | (1u << DEV_USB);
      if (mask & 0x02) devices = devices | (1u << DEV_SDCARD);
      if (mask & 0x04) devices = devices | (1u << DEV_AUX);
      if (mask & 0x10) devices = devices | (1u << DEV_FLASH);
      return onInitComplete(devices);
    }

    case 0x40: return onError(msg.getParamLo());
    case 0x41: return onAck();

    // Query responses
    case 0x42: {
      // Only Flyron documents this response to the status query.
      // The DFPlayer Mini always seems to report SDCARD even when
      // the selected and active device is USB, so maybe it uses
      // the high byte to signal something else?  Catalex also
      // always reports the SDCARD, but it only has an SDCARD.
      Device device = DEV_SLEEP;
      switch (msg.getParamHi()) {
        case 0x01: device = DEV_USB;     break;
        case 0x02: device = DEV_SDCARD;  break;
      }
      ModuleState state = MS_ASLEEP;
      switch (msg.getParamLo()) {
        case 0x00: state = MS_STOPPED;  break;
        case 0x01: state = MS_PLAYING;  break;
        case 0x02: state = MS_PAUSED;   break;
      }
      return onStatus(device, state);
    }
    case 0x43: return onVolume(msg.getParamLo());
    case 0x44: return onEqualizer(static_cast<Equalizer>(msg.getParamLo()));
    case 0x45: return onPlaybackSequence(static_cast<Sequence>(msg.getParamLo()));
    case 0x46: return onFirmwareVersion(msg.getParam());
    case 0x47: return onDeviceFileCount(DEV_USB, msg.getParam());
    case 0x48: return onDeviceFileCount(DEV_SDCARD, msg.getParam());
    case 0x49: return onDeviceFileCount(DEV_FLASH, msg.getParam());
    case 0x4B: return onCurrentTrack(DEV_USB, msg.getParam());
    case 0x4C: return onCurrentTrack(DEV_SDCARD, msg.getParam());
    case 0x4D: return onCurrentTrack(DEV_FLASH, msg.getParam());
    case 0x4E: return onFolderTrackCount(msg.getParam());
    case 0x4F: return onFolderCount(msg.getParam());
    default: break;
  }
}

void Aidtopia_SerialAudio::sendMessage(const Message &msg) {
  const auto buf = msg.getBuffer();
  const auto len = msg.getLength();
  m_stream->write(buf, len);
  m_timeout.set(200);
  onMessageSent(buf, len);
}

void Aidtopia_SerialAudio::sendCommand(MsgID msgid, uint16_t param, bool feedback) {
  m_out.set(msgid, param, feedback ? Message::FEEDBACK : Message::NO_FEEDBACK);
  sendMessage(m_out);
}

void Aidtopia_SerialAudio::sendQuery(MsgID msgid, uint16_t param) {
  // Since queries naturally have a response, we won't ask for feedback.
  sendCommand(msgid, param, false);
}

void Aidtopia_SerialAudio::setState(State *new_state, uint8_t arg1, uint8_t arg2) {
  const auto original_state = m_state;
  while (m_state != new_state) {
    m_state = new_state;
    if (m_state) {
      new_state = m_state->onEvent(this, MID_ENTERSTATE, arg1, arg2);
    }
    // break out of a cycle
    if (m_state == original_state) return;
  }
}

Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::State::onEvent(
  Aidtopia_SerialAudio * /*module*/,
  MsgID /*msgid*/,
  uint8_t /*paramHi*/,
  uint8_t /*paramLo*/
) {
  return this;
}

Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::InitResettingHardware::onEvent(
  Aidtopia_SerialAudio *module,
  MsgID msgid,
  uint8_t paramHi,
  uint8_t paramLo
) {
  switch (msgid) {
    case MID_ENTERSTATE:
      Serial.println(F("Resetting hardware."));
      module->sendCommand(MID_RESET, 0, false);
      // The default timeout is probably too short for a reset.
      module->m_timeout.set(10000);
      return this;
    case MID_INITCOMPLETE:
      return &Aidtopia_SerialAudio::s_init_getting_version;
    case MID_ERROR:
      if (combine(paramHi, paramLo) == EC_TIMEDOUT) {
        Serial.println(F("No response from audio module"));
      }
      return nullptr;
    default: break;
  }
  return this;
}
Aidtopia_SerialAudio::InitResettingHardware Aidtopia_SerialAudio::s_init_resetting_hardware;


Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::InitGettingVersion::onEvent(
    Aidtopia_SerialAudio *module,
    MsgID msgid,
    uint8_t paramHi,
    uint8_t paramLo
) {
  switch (msgid) {
    case MID_ENTERSTATE:
      module->queryFirmwareVersion();
      return this;
    case MID_FIRMWAREVERSION:
      return &s_init_checking_usb_file_count;
    case MID_ERROR:
      if (combine(paramHi, paramLo) == EC_TIMEDOUT) {
        return &s_init_checking_usb_file_count;
      }
      break;
    default: break;
  }
  return this;
}
Aidtopia_SerialAudio::InitGettingVersion Aidtopia_SerialAudio::s_init_getting_version;

Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::InitCheckingUSBFileCount::onEvent(
  Aidtopia_SerialAudio *module,
  MsgID msgid,
  uint8_t paramHi, uint8_t paramLo
) {
  switch (msgid) {
    case MID_ENTERSTATE:
      module->queryFileCount(Aidtopia_SerialAudio::DEV_USB);
      return this;
    case MID_USBFILECOUNT: {
      const auto count = (static_cast<uint16_t>(paramHi) << 8) | paramLo;
      module->m_files = count;
      if (count > 0) return &s_init_selecting_usb;
      return &s_init_checking_sd_file_count;
    }
    case MID_ERROR:
      return &s_init_checking_sd_file_count;
    default: break;
  }
  return this;
}
Aidtopia_SerialAudio::InitCheckingUSBFileCount Aidtopia_SerialAudio::s_init_checking_usb_file_count;

Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::InitCheckingSDFileCount::onEvent(
  Aidtopia_SerialAudio *module,
  MsgID msgid,
  uint8_t paramHi, uint8_t paramLo
) {
  switch (msgid) {
    case MID_ENTERSTATE:
      module->queryFileCount(Aidtopia_SerialAudio::DEV_SDCARD);
      return this;
    case MID_SDFILECOUNT: {
      const auto count = (static_cast<uint16_t>(paramHi) << 8) | paramLo;
      module->m_files = count;
      if (count > 0) return &s_init_selecting_sd;
      return nullptr;
    }
    case MID_ERROR:
      return nullptr;
    default: break;
  }
  return this;
}
Aidtopia_SerialAudio::InitCheckingSDFileCount Aidtopia_SerialAudio::s_init_checking_sd_file_count;

Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::InitSelectingUSB::onEvent(
  Aidtopia_SerialAudio *module,
  MsgID msgid,
  uint8_t /*paramHi*/, uint8_t /*paramLo*/
) {
  switch (msgid) {
    case MID_ENTERSTATE:
      module->selectSource(Aidtopia_SerialAudio::DEV_USB);
      return this;
    case MID_ACK:
      module->m_source = Aidtopia_SerialAudio::DEV_USB;
      return &s_init_checking_folder_count;
    default: break;
  }
  return this;
}
Aidtopia_SerialAudio::InitSelectingUSB Aidtopia_SerialAudio::s_init_selecting_usb;

Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::InitSelectingSD::onEvent(
  Aidtopia_SerialAudio *module,
  MsgID msgid,
  uint8_t /*paramHi*/, uint8_t /*paramLo*/
) {
  switch (msgid) {
    case MID_ENTERSTATE:
      module->selectSource(Aidtopia_SerialAudio::DEV_SDCARD);
      return this;
    case MID_ACK:
      module->m_source = Aidtopia_SerialAudio::DEV_SDCARD;
      return &s_init_checking_folder_count;
    default: break;
  }
  return this;
}
Aidtopia_SerialAudio::InitSelectingSD Aidtopia_SerialAudio::s_init_selecting_sd;

Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::InitCheckingFolderCount::onEvent(
  Aidtopia_SerialAudio *module,
  MsgID msgid,
  uint8_t /*paramHi*/, uint8_t paramLo
) {
  switch (msgid) {
    case MID_ENTERSTATE:
      module->queryFolderCount();
      return this;
    case MID_FOLDERCOUNT:
      module->m_folders = paramLo;
      Serial.print(F("Audio module initialized.\nSelected: "));
      module->printDeviceName(module->m_source);
      Serial.print(F(" with "));
      Serial.print(module->m_files);
      Serial.print(F(" files and "));
      Serial.print(module->m_folders);
      Serial.println(F(" folders"));
      return nullptr;
    default: break;
  }
  return this;
}
Aidtopia_SerialAudio::InitCheckingFolderCount  Aidtopia_SerialAudio::s_init_checking_folder_count;

Aidtopia_SerialAudio::State *Aidtopia_SerialAudio::InitStartPlaying::onEvent(
  Aidtopia_SerialAudio *module,
  MsgID msgid,
  uint8_t /*paramHi*/, uint8_t /*paramLo*/
) {
  switch (msgid) {
    case MID_ENTERSTATE:
      module->sendCommand(MID_LOOPFOLDER, 1);
      return this;
    case MID_ACK:
      return nullptr;
    default: break;
  }
  return this;
}
Aidtopia_SerialAudio::InitStartPlaying         Aidtopia_SerialAudio::s_init_start_playing;
