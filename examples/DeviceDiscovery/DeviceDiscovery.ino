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
      m_folderCount(0),
      m_folder(0) {}

    void onInitComplete(Devices devices) override {
      Serial.println(F("Initialization completed."));
      Serial.print(F("Devices Available:"));
      printDevices(devices);
      Serial.println();
      m_toexplore = devices;
      explore();
    }

    void onError(Error code) override {
      if (code == Error::TRACKNOTFOUND &&
          m_device != Device::NONE &&
          m_folder < m_folderCount
      ) {
        onQueryResponse(Parameter::FOLDERFILECOUNT, 0);
      }
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
        case Parameter::FOLDERCOUNT:
          m_folderCount = value;
          m_folder = 1;
          printDeviceName(m_device);
          Serial.print(F(" has "));
          Serial.print(m_folderCount);
          Serial.println(F(" folders."));
          if (m_folder < m_folderCount) {
            audio.queryFolderFileCount(m_folder);
          }
          break;
        case Parameter::FOLDERFILECOUNT:
          printDeviceName(m_device);
          Serial.print(F(" folder "));
          Serial.print(m_folder);
          Serial.print(F(" has "));
          Serial.print(value);
          Serial.println(F(" file(s)."));
          m_folder += 1;
          if (m_folder >= m_folderCount) {
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
      audio.selectSource(m_device);
      audio.queryFolderCount();
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

    Devices m_toexplore;  // devices we want to explore
    Device m_device;  // the device we're currently exploring
    uint16_t m_folderCount;  // the number of folders on m_device
    uint16_t m_folder;  // the current folder
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
