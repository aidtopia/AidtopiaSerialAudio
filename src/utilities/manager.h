#ifndef AIDTOPIA_SERIALAUDIOMANAGER_H
#define AIDTOPIA_SERIALAUDIOMANAGER_H

#include <utilities/core.h>
#include <utilities/timeout.h>

namespace aidtopia {

class SerialAudioManager : public SerialAudio {
    public:
        using SerialAudio::Device;
        using SerialAudio::ModuleState;
        using SerialAudio::EqProfile;
        using SerialAudio::Sequence;
        using SerialAudio::Hooks;
    
        template <typename SerialType>
        void begin(SerialType &stream) {
            m_core.begin(stream);

            // Arduino IDE compiler isn't up-to-date enough to allow us to put
            // initializers for bitfield members in the class definition.  So
            // we'll init the queue pointers here.
            clearQueue();

            // begin is usually called right after power-up, so we'll assume the
            // audio module also just powered up.  If it's already powered up,
            // then, once the timeout expires, the state machine will do what it
            // needs to do.
            m_state = State::INITPENDING;
            m_timeout.set(3000);
        }
        
        void update(Hooks *hooks = nullptr);

        // Commands with "File" access sounds by their file system indexes.
        // Commands with "Track" access them by the file name's prefix.
        void playFile(uint16_t index);
        void playTrack(uint16_t track);  // from "MP3" folder
        void playTrack(uint16_t folder, uint16_t track);
        void reset();
        void selectSource(Device source);
        void setVolume(uint8_t volume);

    // These will become private.
        void sendMessage(Message const &msg);
        void enqueue(Message const &msg);

    private:
        void clearQueue();
        void dispatch();
        void onEvent(Message const &msg, Hooks *hooks);
        void ready();

        SerialAudioCore         m_core;
        Timeout<MillisClock>    m_timeout;
        enum class State {
            READY,
            ACKPENDING,
            RESPONSEPENDING,
            INITPENDING,
            RECOVERING,
            SOURCEPENDINGACK,
            SOURCEPENDINGDELAY,
            STUCK
        }                       m_state = State::INITPENDING;
        Message::ID             m_lastRequest = Message::ID::NONE;
        Message                 m_queue[8];
        uint8_t                 m_head : 4;
        uint8_t                 m_tail : 4;
};

}

#endif
