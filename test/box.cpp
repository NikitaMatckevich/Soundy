#include <device.hpp>

#include <cinttypes>
#include <chrono>
#include <cmath>
#include <thread>
#include <iostream>

enum class Note {
    C1    = 261,
    CS1   = 277,
    D1    = 293,
    DS1   = 311,
    E1    = 329,
    F1    = 349,
    FS1   = 369,
    G1    = 391,
    GS1   = 415,
    A1    = 440,
    AS1   = 466,
    B1    = 493,
    C2    = 523,
    CS2   = 554,
    D2    = 587,
    DS2   = 622,
    E2    = 659,
    F2    = 698,
    FS2   = 739,
    G2    = 783,
    GS2   = 830,
    A2    = 880,
    AS2   = 932,
    B2    = 987,
    NONE  = -1
};

enum class Duration {
    D1  = 1,
    D2  = 2,
    D4  = 4,
    D8  = 8,
    D16 = 16,
    D32 = 32
};

class MusicBox {
    static constexpr const float pi = 3.14159265f;
    
    alsa::Device _device;
    uint32_t _tempo;
 
    volatile Note _note;

public:
    MusicBox(uint32_t tempo) :
            _device("default", alsa::StreamUsageMode::SND_PCM_STREAM_PLAYBACK, alsa::StreamOpenMode::NONBLOCK),
            _tempo(tempo),
            _note(Note::NONE)
    {
        _device.setAccessMode(alsa::AccessMode::SND_PCM_ACCESS_RW_INTERLEAVED);
        _device.setResampleRate(1);
        _device.setNumChannels(1);
        _device.setRateNear(44100);
        _device.setBufferTimeNear(100000);
        _device.setPeriodTimeNear(100000);
        _device.prepare();

        alsa::AudioCallback callback = [&](const alsa::AudioBuffer& buffer, float& phase) {
            if (this->_note == Note::NONE) {
                for (uint64_t i = 0; i < buffer.periodSize; i++) {
                    buffer.samples[i] = 0;
                }
                return;
            }

            float freq = static_cast<float>(static_cast<int>(this->_note)); // Frequency in Hz    
            float inversePeriod = freq / (float)buffer.rate ;

            for (uint64_t i = 0; i < buffer.periodSize; i++) {
                buffer.samples[i] = (int8_t)(100 * (phase - std::floor(0.5f + phase)));
                phase += inversePeriod;
            }

            if (phase >= 1) {
                phase -= 1;
            }
        };
        _device.registerAudioCallback(callback);
        _device.start(); 
    }
    
    ~MusicBox() {
        _device.drain();
    }

    void playNote(Note note, Duration duration) {
        // Calculate the duration of each whole note (the bar) based on _tempo
        float barDuration = 240.f / _tempo;
    
        // Calculate the total duration for the given note
        float noteDuration = barDuration / static_cast<int>(duration);
    
        _note = note;

        // Sleep for the duration of the note
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(noteDuration * 1000)));

        _note = Note::NONE;
    }

};

void playSong() {
    MusicBox b(40);
    // Tubular Bells by Mike Oldfield
    for (int loopId = 0; loopId < 10; loopId++) {
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::A2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::B2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::G1, Duration::D16);
        b.playNote(Note::A2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::C2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::D2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::B2, Duration::D16);
        b.playNote(Note::C2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::A2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::B2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::G1, Duration::D16);
        b.playNote(Note::A2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::C2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::D2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::B2, Duration::D16);
        b.playNote(Note::C2, Duration::D16);
        b.playNote(Note::E1, Duration::D16);
        b.playNote(Note::B2, Duration::D16);
    }
}

int main() {
   try {
        playSong();
   } catch (const alsa::AlsaException& e) {
       std::cout << e.what() << std::endl;
   }
   return 0;
}
