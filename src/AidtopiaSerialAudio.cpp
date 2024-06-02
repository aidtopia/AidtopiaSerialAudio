// AidtopiaSerialAudio
// Adrian McCarthy 2018-

#include <Arduino.h>
#include <AidtopiaSerialAudio.h>

#if 0
void Aidtopia_SerialAudioWithLogging::printDeviceName(Device src) {
  switch (src) {
    case DEV_USB:    Serial.print(F("USB")); break;
    case DEV_SDCARD: Serial.print(F("SD Card")); break;
    case DEV_AUX:    Serial.print(F("AUX")); break;
    case DEV_FLASH:  Serial.print(F("FLASH")); break;
    case DEV_SLEEP:  Serial.print(F("SLEEP")); break;
    default:         Serial.print(F("Unknown Device")); break;
  }
}

void Aidtopia_SerialAudioWithLogging::printEqualizerName(EqProfile eq) {
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

void Aidtopia_SerialAudioWithLogging::printModuleStateName(ModuleState state) {
  switch (state) {
    case MS_STOPPED: Serial.print(F("Stopped")); break;
    case MS_PLAYING: Serial.print(F("Playing")); break;
    case MS_PAUSED:  Serial.print(F("Paused"));  break;
    case MS_ASLEEP:  Serial.print(F("Asleep"));  break;
    default:         Serial.print(F("???"));     break;
  }
}

void Aidtopia_SerialAudioWithLogging::printSequenceName(Sequence seq) {
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
