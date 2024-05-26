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
  onMessageReceived(msg);
  if (!msg.isValid()) return onMessageInvalid();
  if (!m_state) return;
  setState(m_state->onEvent(this, msg.getMessageID(), msg.getParamHi(), msg.getParamLo()));
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
