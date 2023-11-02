#include <device.hpp>

#include <cinttypes>
#include <chrono>
#include <cmath>
#include <thread>
#include <iostream>

// Define the frequencies array
float frequencies[] = {
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 
    415.30, 440.00, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 
    659.26, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77
};

// Define the Note enum class
enum class Note {
    C4, CS4, D4, DS4, E4, F4, FS4, G4, GS4, A4, AS4, B4,
    C5, CS5, D5, DS5, E5, F5, FS5, G5, GS5, A5, AS5, B5,
    NONE
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
        _device.setNumChannels(2);
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
            float freq = frequencies[static_cast<int>(this->_note)]; // Frequency in Hz   
            float step = 2. * pi * freq / (float)buffer.rate;
            for (uint64_t i = 0; i < buffer.periodSize; i++) {
                for (uint32_t ch = 0; ch < buffer.numChannels; ch++) {
                    buffer.samples[buffer.numChannels * i + ch] = (uint8_t)((std::sin(phase) + 1) * 127);
                }
                phase += step;   
            }

            if (phase >= 2. * pi) {
                phase -= 2. * pi;
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
   }

};

void playSong() {
    MusicBox b(55);
    // Tubular Bells by Mike Oldfield
    for (int loopId = 0; loopId < 8; loopId++) {
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::A4, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::B4, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::G4, Duration::D16);
        b.playNote(Note::A4, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::C5, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::D5, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::B4, Duration::D16);
        b.playNote(Note::C5, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::A4, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::B4, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::G4, Duration::D16);
        b.playNote(Note::A4, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::C5, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::D5, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::B4, Duration::D16);
        b.playNote(Note::C5, Duration::D16);
        b.playNote(Note::E4, Duration::D16);
        b.playNote(Note::B4, Duration::D16);
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
