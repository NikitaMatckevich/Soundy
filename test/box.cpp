#include <chrono>
#include <device.hpp>

#include <cinttypes>
#include <array>
#include <cmath>
#include <thread>
#include <atomic>
#include <iostream>
#include <cstdlib>

// Define the frequencies array
static constexpr const float frequencies[] = {
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

template <class T, uint8_t Order>
struct SpScQueue {

    constexpr static uint32_t getCapacity() {
        return 1U << Order;
    }

    constexpr static uint32_t getMask() {
        return getCapacity() - 1;
    }

    explicit SpScQueue(const T& defaultValue) : _defaultValue(defaultValue) {}
    SpScQueue(const SpScQueue& other) = delete;
    SpScQueue& operator=(const SpScQueue& other) = delete;

    bool enqueue(const T& item) {
        uint32_t currentTail = _tail.load(std::memory_order_relaxed);
        uint32_t nextTail = nextIndex(currentTail);

        if (nextTail == _head.load(std::memory_order_acquire)) {
            return false; // Queue is full  
        }

        new (_buffer.data() + currentTail) T(item);
         _tail.store(nextTail, std::memory_order_release);
        return true;
    }

    T dequeue() {
        uint32_t currentTail = _tail.load(std::memory_order_acquire); 
        uint32_t currentHead = _head.load(std::memory_order_relaxed);
        if (currentHead == currentTail) {
            return _defaultValue; // Queue is empty
        }

        T item = _buffer[currentHead];

        uint32_t nextHead = nextIndex(currentHead);
        _head.store(nextHead, std::memory_order_release);
        return item;
    }

private:    
    std::array<T, getCapacity()>  _buffer;
    T _defaultValue;

    alignas(64) std::atomic<uint32_t> _head{0};
    alignas(64) std::atomic<uint32_t> _tail{0};

    uint32_t nextIndex(uint32_t current) const {
        return (current + 1) & getMask();
    }
};

class MusicBox {
    static constexpr const float pi = 3.14159265f;
    
    alsa::Device _device;
    SpScQueue<Note, 10> _notes;
    uint64_t _expectedPeriodsPlayed = 0;
    uint64_t _actualPeriodsPlayed = 0;
    uint32_t _tempo; 

public:
    MusicBox(uint32_t tempo) :
            _device("default", alsa::StreamUsageMode::SND_PCM_STREAM_PLAYBACK, alsa::StreamOpenMode::NONBLOCK),
            _notes(Note::NONE),
            _tempo(tempo)
    {
        _device.setAccessMode(alsa::AccessMode::SND_PCM_ACCESS_RW_INTERLEAVED);
        _device.setResampleRate(1);
        _device.setNumChannels(2);
        _device.setRateNear(44100);
        _device.setBufferTimeNear(200000);
        _device.setPeriodTimeNear(100000);
        _device.prepare();
        
        alsa::AudioCallback callback = [&](const alsa::AudioBuffer& buffer, float& phase) {
            Note note = this->_notes.dequeue();
            if (note == Note::NONE) {
                for (uint64_t i = 0; i < buffer.periodSize; i++) {
                    buffer.samples[i] = 0;
                }
                return;
            }

            float freq = frequencies[static_cast<int>(note)]; // Frequency in Hz
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
            
            this->_actualPeriodsPlayed++;
        };

        _device.registerAudioCallback(callback);
        _device.start();
    }
    
    ~MusicBox() {
        while (_actualPeriodsPlayed < _expectedPeriodsPlayed) {
            printf("actual periods = %lu, expected periods = %lu\n", _actualPeriodsPlayed, _expectedPeriodsPlayed);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        _device.drain();
    }

    void playNote(Note note, Duration duration) {
        // Calculate the duration of each whole note (the bar) based on _tempo
        float barDuration = 240.f / _tempo;
    
        // Calculate the total duration for the given note
        uint32_t noteDuration = (uint32_t)((1'000'000 / static_cast<int>(duration)) * barDuration);
        uint32_t periodDuration = _device.getPeriodTime();

        for (uint32_t time = 0; time < noteDuration; time += periodDuration) {
             _expectedPeriodsPlayed++;
            while (!_notes.enqueue(note)) {}
        }
   }

};

void playSong() {
    MusicBox b(90);
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
