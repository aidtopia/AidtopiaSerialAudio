// AidtopiaSerialAudio
// Adrian McCarthy 2018-

// A library that works with various serial audio modules,
// like DFPlayer Mini, Catalex, etc.

#include <AidtopiaSerialAudio.h>

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


Aidtopia_SerialAudio::InitResettingHardware    Aidtopia_SerialAudio::s_init_resetting_hardware;
Aidtopia_SerialAudio::InitGettingVersion       Aidtopia_SerialAudio::s_init_getting_version;
Aidtopia_SerialAudio::InitCheckingUSBFileCount Aidtopia_SerialAudio::s_init_checking_usb_file_count;
Aidtopia_SerialAudio::InitCheckingSDFileCount  Aidtopia_SerialAudio::s_init_checking_sd_file_count;
Aidtopia_SerialAudio::InitSelectingUSB         Aidtopia_SerialAudio::s_init_selecting_usb;
Aidtopia_SerialAudio::InitSelectingSD          Aidtopia_SerialAudio::s_init_selecting_sd;
Aidtopia_SerialAudio::InitCheckingFolderCount  Aidtopia_SerialAudio::s_init_checking_folder_count;
Aidtopia_SerialAudio::InitStartPlaying         Aidtopia_SerialAudio::s_init_start_playing;
