#ifndef AIDTOPIA_SERIALAUDIOMESSAGE_H
#define AIDTOPIA_SERIALAUDIOMESSAGE_H

namespace aidtopia {

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
            SETEQPROFILE      = 0x07,
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
            LOOPCURRENTTRACK  = 0x19,
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
            // This arrives some time after Message::ID::RESET (or power on), so
            // I've been viewing it as an asynchronous notification.  The Flyron
            // documentation says the user can send this to query which storage
            // devices are online, but that doesn't work for any of the modules
            // I've tried.
            INITCOMPLETE      = 0x3F,

            // Basic replies
            ERROR             = 0x40,
            ACK               = 0x41,

            // Queries and their responses
            DEVICESONLINE     = INITCOMPLETE,
            STATUS            = 0x42,
            VOLUME            = 0x43,
            EQPROFILE         = 0x44,
            PLAYBACKSEQUENCE  = 0x45,
            FIRMWAREVERSION   = 0x46,
            USBFILECOUNT      = 0x47,
            SDFILECOUNT       = 0x48,
            FLASHFILECOUNT    = 0x49,
            // no 0x4A? AUXFILECOUNT?
            CURRENTUSBFILE    = 0x4B,
            CURRENTSDFILE     = 0x4C,
            CURRENTFLASHFILE  = 0x4D,
            FOLDERFILECOUNT   = 0x4E,
            FOLDERCOUNT       = 0x4F,
            CURRENTFLASHFOLDER= 0x61
        };
        
        Message() : m_id(Message::ID::NONE), m_data(0) {}
        explicit Message(Message::ID id, uint16_t data = 0) :
            m_id(id), m_data(data) {}

        void clear() { m_id = ID::NONE; m_data = 0; }
        bool operator==(Message const &rhs) const {
            return m_id == rhs.m_id && m_data == rhs.m_data;
        }

        Message::ID getID()      const { return m_id; }
        uint16_t    getParam()   const { return m_data; }

    private:
        Message::ID m_id = Message::ID::NONE;
        uint16_t    m_data = 0;
};

inline bool isCommand(Message::ID msgid) {
    auto const id = static_cast<uint8_t>(msgid);
    return 0x00 < id && id < 0x30;
}
inline bool isCommand(Message const &msg) { return isCommand(msg.getID()); }

inline bool isAsyncNotification(Message::ID msgid) {
    auto const id = static_cast<uint8_t>(msgid);
    return 0x30 <= id && id < 0x3F;
}
inline bool isAsyncNotification(Message const &msg) {
    return isAsyncNotification(msg.getID());
}

inline bool isAck(Message::ID id) { return id == Message::ID::ACK; }
inline bool isAck(Message const &msg) { return isAck(msg.getID()); }

inline bool isError(Message::ID id) { return id == Message::ID::ERROR; }
inline bool isError(Message const &msg) { return isError(msg.getID()); }

inline bool isQuery(Message::ID msgid) {
    // Supposedly 0x3F (normally a notification of a reset being completed) can
    // be a query to determine which devices are attached.  I've not yet seen
    // a module that supports it as a query, but we'll allow it.
    if (msgid == Message::ID::DEVICESONLINE) return true;
    return 0x42 <= static_cast<uint8_t>(msgid);
}
inline bool isQuery(Message const &msg) { return isQuery(msg.getID()); }

inline bool isQueryResponse(Message::ID id) { return isQuery(id); }
inline bool isQueryResponse(Message const &msg) { return isQuery(msg.getID()); }

inline bool isInitComplete(Message::ID id) { return id == Message::ID::INITCOMPLETE; }
inline bool isInitComplete(Message const &msg) { return isInitComplete(msg.getID()); }

}

#endif
