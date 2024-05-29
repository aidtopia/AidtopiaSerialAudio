#ifndef AIDTOPIA_SERIALAUDIOCORE_H
#define AIDTOPIA_SERIALAUDIOCORE_H

#include <utilities/message.h>
#include <utilities/timeout.h>

namespace aidtopia {

// This class handles the serial protocol.
class SerialAudioCore {
    public:
        template <typename SerialType>
        void begin(SerialType &serial) {
            serial.begin(9600);
            m_stream = &serial;
        }

        void update();

        // Union of the sets of message IDs used by the various modules.
        enum MsgID : uint8_t {
          // Commands
          MID_PLAYNEXT          = 0x01,
          MID_PLAYPREVIOUS      = 0x02,
          MID_PLAYFILE          = 0x03,
          MID_VOLUMEUP          = 0x04,
          MID_VOLUMEDOWN        = 0x05,
          MID_SETVOLUME         = 0x06,
          MID_SELECTEQ          = 0x07,
          MID_LOOPFILE          = 0x08,
          MID_LOOPFLASHTRACK    = MID_LOOPFILE,  // Alternate msg not used
          MID_SELECTSOURCE      = 0x09,
          MID_SLEEP             = 0x0A,
          MID_WAKE              = 0x0B,  // If not supported, MID_SELECTSOURCE
          MID_RESET             = 0x0C,
          MID_RESUME            = 0x0D,
          MID_UNPAUSE           = MID_RESUME,
          MID_PAUSE             = 0x0E,
          MID_PLAYFROMFOLDER    = 0x0F,
          MID_AMPLIFIER         = 0x10,
          MID_LOOPALL           = 0x11,
          MID_PLAYFROMMP3       = 0x12,  // "MP3" here refers to name of folder
          MID_INSERTADVERT      = 0x13,
          MID_PLAYFROMBIGFOLDER = 0x14,
          MID_STOPADVERT        = 0x15,
          MID_STOP              = 0x16,
          MID_LOOPFOLDER        = 0x17,
          MID_RANDOMPLAY        = 0x18,
          MID_LOOPCURRENTFILE   = 0x19,
          MID_DISABLEDAC        = 0x1A,
          MID_PLAYLIST          = 0x21,  // requires longer message length
          MID_PLAYWITHVOLUME    = 0x22,  // seems redundant
          MID_INSERTADVERTN     = 0x25,

          // Asynchronous messages from the module
          MID_DEVICEINSERTED    = 0x3A,
          MID_DEVICEREMOVED     = 0x3B,
          MID_FINISHEDUSBFILE   = 0x3C,
          MID_FINISHEDSDFILE    = 0x3D,
          MID_FINISHEDFLASHFILE = 0x3E,

          // Quasi-asynchronous
          // This arrives some time after MID_RESET (or power on), so I've been
          // viewing it as an asynchronous notification.  The Flyron
          // documentation says the user can send this to query which storage
          // devices are online, but that doesn't work for any of the modules
          // I've tried.
          MID_INITCOMPLETE      = 0x3F,

          // Basic replies
          MID_ERROR             = 0x40,
          MID_ACK               = 0x41,

          // Queries and their responses
          MID_STATUS            = 0x42,
          MID_VOLUME            = 0x43,
          MID_EQ                = 0x44,
          MID_PLAYBACKSEQUENCE  = 0x45,
          MID_FIRMWAREVERSION   = 0x46,
          MID_USBFILECOUNT      = 0x47,
          MID_SDFILECOUNT       = 0x48,
          MID_FLASHFILECOUNT    = 0x49,
          // no 0x4A? MID_AUXFILECOUNT?
          MID_CURRENTUSBFILE    = 0x4B,
          MID_CURRENTSDFILE     = 0x4C,
          MID_CURRENTFLASHFILE  = 0x4D,
          MID_FOLDERTRACKCOUNT  = 0x4E,
          MID_FOLDERCOUNT       = 0x4F,
        };

    private:
        void checkForIncomingMessage();
        void checkForTimeout();
        void receiveMessage(const Message &msg);

        void sendCommand(
            MsgID msgid,
            uint16_t param = 0,
            Message::Feedback feedback = Message::FEEDBACK
        );

        void sendMessage(const Message &msg);
        void sendQuery(MsgID msgid, uint16_t param = 0);

    private:
        Stream  *m_stream;
        Message  m_in;
        Message  m_out;
        Timeout<MillisClock> m_timeout;
};

}

#endif
