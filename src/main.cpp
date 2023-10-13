#include <device.hpp>

#include <cmath>
#include <iostream>

int main() {
    try {
        alsa::Device d("default", alsa::StreamUsageMode::SND_PCM_STREAM_PLAYBACK, alsa::StreamOpenMode::NONBLOCK);
        
        d.setResampleRate(1);
        d.setAccessMode(alsa::AccessMode::SND_PCM_ACCESS_RW_INTERLEAVED);
        d.setNumChannels(1);
        
        uint32_t rate = d.setRateNear(44100);
        uint32_t bufferTime = d.setBufferTimeNear(500000);
        uint32_t periodTime = d.setPeriodTimeNear(100000);
        std::cout << rate << " " << bufferTime << " " << periodTime << std::endl;
        
        alsa::AudioCallback callback = [&](const alsa::AudioBuffer& buffer, double& phase) {
            constexpr const double pi = 3.14159265358979323846;
            constexpr const double freq = 440;
            double step = 2. * pi * freq / (double)buffer.rate;
            for (int i = 0; i < buffer.periodTime; i++) {
                buffer.samples[i] = (int8_t)(std::sin(phase) * 255) - 128;
                phase += step;
            }
        };
        d.registerAudioCallback(callback);
        d.start();
        
        std::cin.get();
        
        d.drain();

    } catch (const alsa::AlsaException& e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
