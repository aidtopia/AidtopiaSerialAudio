#ifndef AIDTOPIA_SERIALAUDIOMESSAGEBUFFER_H
#define AIDTOPIA_SERIALAUDIOMESSAGEBUFFER_H

#include <utilities/message.h>

namespace aidtopia {

enum class Feedback : uint8_t {
    NO_FEEDBACK = 0x00,
    FEEDBACK    = 0x01
};

// Manages a message buffer.
class MessageBuffer {
  public:
    MessageBuffer();

    void set(uint8_t msgid, uint16_t data, Feedback feedback);

    const uint8_t *getBytes() const;
    uint8_t getLength() const;
    bool isValid() const;
    uint8_t getID() const;
    uint16_t getData() const;

    // Returns true if the byte `b` completes a message.
    bool receive(uint8_t b);
    
  private:
    // Sums the bytes used to compute the message's checksum.
    uint16_t sum() const;

    enum : uint8_t {
        START   = 0x7E,
        VERSION = 0xFF,
        LENGTH  = 0x06,
        END     = 0xEF
    };

    uint8_t m_buf[10];
    uint8_t m_length;
};

}

#endif
