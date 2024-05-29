#include <Arduino.h>
#include <utilities/core.h>

namespace aidtopia {

void SerialAudioCore::update() {
  checkForIncomingMessage();
  checkForTimeout();
}


void SerialAudioCore::checkForIncomingMessage() {
  while (m_stream->available() > 0) {
    if (m_in.receive(m_stream->read())) {
      receiveMessage(m_in);
    }
  }
}

void SerialAudioCore::checkForTimeout() {
  if (m_timeout.expired()) {
    m_timeout.cancel();
    // onError(EC_TIMEDOUT);
  }
}

void SerialAudioCore::receiveMessage(const Message &msg) {
  // onMessageReceived(msg);
  if (!msg.isValid()) {
      // onMessageInvalid();
      return;
  }
  //callHooks(msg);
  if (msg.getMessageID() >= MID_INITCOMPLETE) m_timeout.cancel();
}

void SerialAudioCore::sendMessage(const Message &msg) {
  const auto buf = msg.getBuffer();
  const auto len = msg.getLength();
  m_stream->write(buf, len);
  m_timeout.set(200);
  //onMessageSent(buf, len);
}

void SerialAudioCore::sendCommand(
    MsgID msgid,
    uint16_t param,
    Message::Feedback feedback
) {
  m_out.set(static_cast<uint8_t>(msgid), param, feedback);
  sendMessage(m_out);
}

void SerialAudioCore::sendQuery(MsgID msgid, uint16_t param) {
  // Since queries naturally have a response, we won't ask for feedback.
  sendCommand(msgid, param, Message::NO_FEEDBACK);
}

}
