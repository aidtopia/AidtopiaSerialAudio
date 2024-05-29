#ifndef AIDTOPIA_SERIALAUDIOMESSAGE_H
#define AIDTOPIA_SERIALAUDIOMESSAGE_H

namespace aidtopia {

inline constexpr uint16_t combine(uint8_t hi, uint8_t lo) {
  return (static_cast<uint16_t>(hi) << 8) | lo;
}
inline constexpr uint8_t MSB(uint16_t x) { return x >> 8; }
inline constexpr uint8_t LSB(uint16_t x) { return x & 0xFF; }

// Manages a message buffer.
class Message {
  public:
    Message();

    enum Feedback : uint8_t { NO_FEEDBACK = 0x00, FEEDBACK = 0x01 };
    void set(uint8_t msgid, uint16_t param, Feedback feedback);

    const uint8_t *getBuffer() const;
    uint8_t getLength() const;
    bool isValid() const;
    uint8_t getMessageID() const;
    uint8_t getParamHi() const;
    uint8_t getParamLo() const;
    uint16_t getParam() const;

    // Returns true if the byte `b` completes a message.
    bool receive(uint8_t b);
    
  private:
    // Sums the bytes used to compute the Message's checksum.
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
