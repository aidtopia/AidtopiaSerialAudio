#include <Arduino.h>
#include "utilities/messagebuffer.h"

namespace aidtopia {

static constexpr uint16_t combine(uint8_t hi, uint8_t lo) {
  return (static_cast<uint16_t>(hi) << 8) | lo;
}

MessageBuffer::MessageBuffer() :
    m_buf{START, VERSION, LENGTH, 0, 0, 0, 0, 0, 0, END},
    m_length(0) {}

MessageBuffer::MessageBuffer(uint8_t msgid, uint16_t param, bool feedback) :
    m_buf{START, VERSION, LENGTH,
          msgid,
          static_cast<uint8_t>(feedback ? 0x01 : 0x00),
          static_cast<uint8_t>((param >> 8) & 0xFF),
          static_cast<uint8_t>((param     ) & 0xFF),
          0, 0, END},
    m_length(10)
{
    auto const checksum = ~sum() + 1u;
    m_buf[7] = (checksum >> 8) & 0xFF;
    m_buf[8] = (checksum     ) & 0xFF;
}

uint8_t const *MessageBuffer::getBytes() const { return m_buf; }
uint8_t MessageBuffer::getLength() const { return m_length; }

bool MessageBuffer::isValid() const {
    if (m_length == 8 && m_buf[7] == END) return true;
    if (m_length != 10) return false;
    auto const checksum = combine(m_buf[7], m_buf[8]);
    return sum() + checksum == 0;
}

uint8_t  MessageBuffer::getID()   const { return m_buf[3]; }
uint16_t MessageBuffer::getData() const { return combine(m_buf[5], m_buf[6]); }

bool MessageBuffer::receive(uint8_t b) {
    switch (m_length) {
        default:
            // `m_length` is out of bounds, so start fresh.
            m_length = 0;
            [[fallthrough]];
        case 0: case 1: case 2: case 9:
            // These bytes must always match the template.
            if (b == m_buf[m_length]) { ++m_length; return m_length == 10; }
            // No match; try to resync.
            if (b == START) { m_length = 1; return false; }
            m_length = 0;
            return false;
        case 7:
            // If there's no checksum, the message may end here.
            if (b == END) { m_length = 8; return true; }
            [[fallthrough]];
        case 3: case 4: case 5: case 6: case 8:
            // These are the payload bytes we care about.
            m_buf[m_length++] = b;
            return false;
    }
}

uint16_t MessageBuffer::sum() const {
    uint16_t s = 0;
    for (int i = 1; i <= LENGTH; ++i) s += m_buf[i];
    return s;
}

}
