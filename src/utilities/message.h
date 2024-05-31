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

            // Asynchronous notifications from the module
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
            FOLDERCOUNT       = 0x4F,
            CURRENTFLASHFOLDER= 0x61
        };
        
        enum class Error : uint16_t {
          UNSUPPORTED        = 0x00,  // MsgID used is not supported
          NOSOURCES          = 0x01,  // module busy or no sources installed
          SLEEPING           = 0x02,  // module is sleeping
          SERIALERROR        = 0x03,  // serial communication error
          BADCHECKSUM        = 0x04,  // module received bad checksum
          FILEOUTOFRANGE     = 0x05,  // this is the file index
          TRACKNOTFOUND      = 0x06,  // couldn't find track by numeric prefix
          INSERTIONERROR     = 0x07,  // couldn't start ADVERT track
          SDCARDERROR        = 0x08,  // unformatted card??
          ENTEREDSLEEP       = 0x0A,  // entered sleep mode??

          // And reserving one for our state machine
          TIMEDOUT           = 0x0100
        };

        Message() : m_id(Message::ID::NONE), m_data(0) {}
        explicit Message(Message::ID id, uint16_t param = 0) :
            m_id(id), m_data(param) {}
        Message(Message::ID id, uint8_t paramHi, uint8_t paramLo) :
            m_id(id), m_data(combine(paramHi, paramLo)) {}
        Message(Message::Error errorCode) :
            m_id(Message::ID::ERROR), m_data(static_cast<uint16_t>(errorCode)) {}

        Message::ID getID()      const { return m_id; }
        uint16_t    getParam()   const { return m_data; }
        uint8_t     getParamHi() const { return MSB(m_data); }
        uint8_t     getParamLo() const { return LSB(m_data); }

    private:
        Message::ID m_id = Message::ID::NONE;
        uint16_t    m_data = 0;
};

inline bool isCommand(Message::ID id) {
    return 0x00 < static_cast<uint8_t>(id) && static_cast<uint8_t>(id) < 0x30;
}

inline bool isAsyncEvent(Message::ID id) {
    return 0x30 <= static_cast<uint8_t>(id) && static_cast<uint8_t>(id) < 0x40;
}

inline bool isAck(Message::ID id) {
    return id == Message::ID::ACK;
}

inline bool isError(Message::ID id) {
    return id == Message::ID::ERROR;
}

inline bool isQuery(Message::ID id) {
    return 0x42 < static_cast<uint8_t>(id);
    // Supposedly Message::ID::INITCOMPLETE (0x3F) can be a query, but I've not
    // yet seen a module that supports that.
}

inline bool isQueryResponse(Message::ID id) {
    return isQuery(id);
}

inline bool isTimeout(Message const &msg) {
    return isError(msg.getID()) &&
           static_cast<Message::Error>(msg.getParam()) == Message::Error::TIMEDOUT;
}

}

#endif
