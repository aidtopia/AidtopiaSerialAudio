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

        void reset();
        void queryFileCount(Device device);
        void selectSource(Device source);
        void queryStatus();
        void queryFirmwareVersion();

        void setVolume(uint8_t volume);
        void increaseVolume();
        void decreaseVolume();
        void queryVolume();

        void setEqProfile(EqProfile eq);
        void queryEqProfile();

        // Commands with "File" access sounds by their file system indexes.
        void playFile(uint16_t index);
        void playNextFile();
        void playPreviousFile();
        void loopFile(uint16_t index);
        void loopAllFiles();
        void playFilesInRandomOrder();

        // Commands with "Track" access sounds by the file name's prefix.
        void queryFolderCount();
        void playTrack(uint16_t track);  // from "MP3" folder
        void playTrack(uint16_t folder, uint16_t track);

        void queryCurrentFile(Device device);
        void queryPlaybackSequence();

        void stop();
        void pause();
        void unpause();

        // You can interrupt a track with an "advertisement" track from a folder
        // named "ADVERT" or "ADVERTn" for n in [1..9].  When the advertisement
        // track completes, the interrupted track will resume.  The module does
        // not indicate when the advertisement track completes (except for a
        // brief blip on the BUSY line if the module has one).  Inserting an
        // advertisement will fail if nothing is currently being played (or even
        // if it's paused).  Using stop during an advertisement stops all
        // playback.  Using stopAdvert stops only the advertisement, allowing
        // the interrupted track to resume.
        void insertAdvert(uint16_t track);
        void insertAdvert(uint8_t folder, uint8_t track);
        void stopAdvert();

#if 0
    void sleep();
    void wake();
    void disableDACs();
    void enableDACs();
#endif

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
        uint8_t                 m_head : 3;
        uint8_t                 m_tail : 3;
        uint8_t                 m_reserved : 2;
};

}

#endif
