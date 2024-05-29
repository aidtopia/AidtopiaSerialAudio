#ifndef AIDTOPIA_SERIALAUDIOMESSAGE_H
#define AIDTOPIA_SERIALAUDIOMESSAGE_H

namespace aidtopia {

inline constexpr uint16_t combine(uint8_t hi, uint8_t lo) {
  return (static_cast<uint16_t>(hi) << 8) | lo;
}
inline constexpr uint8_t MSB(uint16_t x) { return x >> 8; }
inline constexpr uint8_t LSB(uint16_t x) { return x & 0xFF; }

class Message {
    public:
        // Union of the sets of message IDs used by the various modules.
        enum class ID : uint8_t {
            NONE              = 0x00,

            // Commands
            PLAYNEXT          = 0x01,
            PLAYPREVIOUS      = 0x02,
            PLAYFILE          = 0x03,
            VOLUMEUP          = 0x04,
            VOLUMEDOWN        = 0x05,
            SETVOLUME         = 0x06,
            SELECTEQ          = 0x07,
            LOOPFILE          = 0x08,
            LOOPFLASHTRACK    = LOOPFILE,  // Alternate msg not used
            SELECTSOURCE      = 0x09,
            SLEEP             = 0x0A,
            WAKE              = 0x0B,  // If not supported, MID_SELECTSOURCE
            RESET             = 0x0C,
            RESUME            = 0x0D,
            UNPAUSE           = RESUME,
            PAUSE             = 0x0E,
            PLAYFROMFOLDER    = 0x0F,
            AMPLIFIER         = 0x10,
            LOOPALL           = 0x11,
            PLAYFROMMP3       = 0x12,  // "MP3" here refers to name of folder
            INSERTADVERT      = 0x13,
            PLAYFROMBIGFOLDER = 0x14,
            STOPADVERT        = 0x15,
            STOP              = 0x16,
            LOOPFOLDER        = 0x17,
            RANDOMPLAY        = 0x18,
            LOOPCURRENTFILE   = 0x19,
            DISABLEDAC        = 0x1A,
            PLAYLIST          = 0x21,  // requires longer message length
            PLAYWITHVOLUME    = 0x22,  // seems redundant
            INSERTADVERTN     = 0x25,

            // Asynchronous messages from the module
            DEVICEINSERTED    = 0x3A,
            DEVICEREMOVED     = 0x3B,
            FINISHEDUSBFILE   = 0x3C,
            FINISHEDSDFILE    = 0x3D,
            FINISHEDFLASHFILE = 0x3E,

            // Quasi-asynchronous
            // This arrives some time after MID_RESET (or power on), so I've been
            // viewing it as an asynchronous notification.  The Flyron
            // documentation says the user can send this to query which storage
            // devices are online, but that doesn't work for any of the modules
            // I've tried.
            INITCOMPLETE      = 0x3F,

            // Basic replies
            ERROR             = 0x40,
            ACK               = 0x41,

            // Queries and their responses
            STATUS            = 0x42,
            VOLUME            = 0x43,
            EQ                = 0x44,
            PLAYBACKSEQUENCE  = 0x45,
            FIRMWAREVERSION   = 0x46,
            USBFILECOUNT      = 0x47,
            SDFILECOUNT       = 0x48,
            FLASHFILECOUNT    = 0x49,
            // no 0x4A? AUXFILECOUNT?
            CURRENTUSBFILE    = 0x4B,
            CURRENTSDFILE     = 0x4C,
            CURRENTFLASHFILE  = 0x4D,
            FOLDERTRACKCOUNT  = 0x4E,
            FOLDERCOUNT       = 0x4F
        };

        explicit Message(Message::ID id, uint16_t param = 0) :
            m_id(id), m_data(param) {}
        Message(Message::ID id, uint8_t paramHi, uint8_t paramLo) :
            m_id(id), m_data(combine(paramHi, paramLo)) {}

        Message::ID getID()      const { return m_id; }
        uint16_t    getParam()   const { return m_data; }
        uint8_t     getParamHi() const { return MSB(m_data); }
        uint8_t     getParamLo() const { return LSB(m_data); }

    private:
        Message::ID m_id = Message::ID::NONE;
        uint16_t    m_data = 0;
};

}

#endif
