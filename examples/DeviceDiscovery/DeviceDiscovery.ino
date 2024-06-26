#include <AidtopiaSerialAudio.h>

// This example demonstrates how to query a serial audio player to
// learn more about its state and about the storage devices
// attached to it.
//
// The Queries example shows how to use callback hooks to receive
// answers to simple queries.
//
// The PlayList example shows how to sequence commands in response
// to callbacks.
//
// This example is a more complex demonstration of both of those
// techniques.

AidtopiaSerialAudio audio;

// As usual, we derive a class from AidtopiaSerialAudio::Hooks in order
// to receive responses and asynchronous notifications from the audio
// player.
class DeviceDiscoveryHooks : public AidtopiaSerialAudio::Hooks {
  public:
    DeviceDiscoveryHooks() :
      m_toexplore(),
      m_device(Device::NONE),
      m_fileCount(0),
      m_folderCount(0),
      m_folder(0),
      m_fileTally(0) {}

    void onInitComplete(Devices devices) override {
      Serial.print(F("Devices Available:"));
      printDevices(devices);
      Serial.println();
      m_toexplore = devices;
      explore();
    }

    void onError(Error code, ID msgid) override {
      if (code == Error::TRACKNOTFOUND &&
          msgid == ID::FOLDERFILECOUNT &&
          m_device != Device::NONE &&
          m_folder < m_folderCount
      ) {
        onQueryResponse(Parameter::FOLDERFILECOUNT, 0);
        return;
      }
      Serial.print(F("Error "));
      Serial.print(static_cast<uint16_t>(code), HEX);
      Serial.print(F(" after sending "));
      Serial.println(static_cast<uint8_t>(msgid), HEX);
    }

    void onDeviceChange(Device device, DeviceChange change) override {
      switch (change) {
        case DeviceChange::INSERTED:
          Serial.print(F("Inserted "));
          printDeviceName(device);
          Serial.println();
          m_toexplore.insert(device);
          explore();
          break;
        case DeviceChange::REMOVED:
          Serial.print(F("Removed "));
          printDeviceName(device);
          Serial.println();
          m_toexplore.remove(device);
          if (device == m_device) {
            m_device = Device::NONE;
            m_folderCount = 0;
            m_folder = 0;
            explore();
          }
          break;
      }
    }

    void onQueryResponse(Parameter param, uint16_t value) override {
      switch (param) {
        case Parameter::USBFILECOUNT:   [[fallthrough]];
        case Parameter::SDFILECOUNT:    [[fallthrough]];
        case Parameter::FLASHFILECOUNT:
          m_fileCount = value;
          m_fileTally = 0;
          if (m_fileCount == 0) {
            printDeviceName(m_device);
            Serial.println(F(" has no files."));
            m_device = Device::NONE;
            explore();
            return;
          }
          audio.selectSource(m_device);
          audio.queryFolderCount();
          break;

        case Parameter::FOLDERCOUNT:
          m_folderCount = value;
          m_folder = 0;
          printDeviceName(m_device);
          Serial.print(F(" has "));
          Serial.print(m_fileCount);
          Serial.print(F(" files in "));
          Serial.print(m_folderCount);
          Serial.println(F(" folders."));
          if (m_folder < m_folderCount) {
            audio.queryFolderFileCount(m_folder);
          }
          break;

        case Parameter::FOLDERFILECOUNT:
          m_fileTally += value;
          if (value > 0) {
            printDeviceName(m_device);
            Serial.print(F(" folder \""));
            printNumberedFolderName(m_folder);
            Serial.print(F("\" has "));
            Serial.print(value);
            if (value == 1) Serial.println(F(" file."));
            else            Serial.println(F(" files."));
          }
          m_folder += 1;
          if (m_folder >= m_folderCount) {
            auto const remainder = m_fileCount - m_fileTally;
            if (remainder > 0) {
              Serial.print(F("The other "));
              if (remainder == 1) {
                Serial.print(F("file"));
              } else {
                Serial.print(remainder);
                Serial.print(F(" files"));
              }
              Serial.print(F(" may be in the root folder, unnumbered "));
              Serial.print(F("folders (like \"MP3\"), or folders numbered "));
              Serial.print(F("higher than \""));
              printNumberedFolderName(m_folderCount-1);
              Serial.println(F("\"."));
            }
            m_device = Device::NONE;
            m_folderCount = 0;
            m_folder = 0;
            explore();
          } else {
            audio.queryFolderFileCount(m_folder);
          }
          break;

        default:
          Serial.print(F("Unexpected response for parameter "));
          Serial.println(static_cast<uint8_t>(param), HEX);
          break;
      }
    }

  private:
    void explore() {
      if (m_device != Device::NONE) return;
      if (m_toexplore.empty()) return;
      if      (m_toexplore.has(Device::USB))    m_device = Device::USB;
      else if (m_toexplore.has(Device::SDCARD)) m_device = Device::SDCARD;
      else if (m_toexplore.has(Device::FLASH))  m_device = Device::FLASH;
      else                                      m_device = Device::NONE;
      m_toexplore.remove(m_device);
      audio.queryFileCount(m_device);
    }

    void printDevices(Devices devices) {
      if (devices.empty())              Serial.print(F(" none"));
      if (devices.has(Device::USB))     Serial.print(F(" USB"));
      if (devices.has(Device::SDCARD))  Serial.print(F(" SD"));
      if (devices.has(Device::FLASH))   Serial.print(F(" FLASH"));
    }

    void printDeviceName(Device device) {
      switch (device) {
        case Device::USB:     Serial.print(F("USB drive"));     break;
        case Device::SDCARD:  Serial.print(F("SD card"));       break;
        case Device::FLASH:   Serial.print(F("flash memory"));  break;
        default: break;
      }
    }

    void printNumberedFolderName(uint8_t folder) {
      if (folder < 10) Serial.print('0');
      Serial.print(folder);
    }

    Devices m_toexplore;  // devices we want to explore
    Device m_device;  // the device we're currently exploring
    uint16_t m_fileCount;  // the number of files on m_device
    uint16_t m_folderCount;  // the number of folders on m_device
    uint16_t m_folder;  // the current folder
    uint16_t m_fileTally;  // count of files found in numbered folders
};

DeviceDiscoveryHooks hooks;

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nDeviceDiscovery example for AidtopiaSerialAudio\n"));

  audio.begin(Serial1);
}

void loop() {
  audio.update(hooks);
}
