#include <device.hpp>

#include <cmath>
#include <iostream>

int main() {
    try {
        alsa::Device d("default", alsa::StreamUsageMode::SND_PCM_STREAM_PLAYBACK, alsa::StreamOpenMode::NONBLOCK);
        d.setAccessMode(alsa::AccessMode::SND_PCM_ACCESS_RW_INTERLEAVED);
        d.setResampleRate(1);
        d.setNumChannels(1);
        d.setRateNear(44100);
        d.setBufferTimeNear(500000);
        d.setPeriodTimeNear(100000);
        d.prepare();

        std::cout << "Actual Buffer Time: " << d.getBufferTime() << " microseconds" << std::endl;
        std::cout << "Actual Period Time: " << d.getPeriodTime() << " microseconds" << std::endl;
        std::cout << "Actual Sample Rate: " << d.getRate() << " Hz" << std::endl;
        std::cout << "Actual Number of Channels: " << d.getNumChannels() << std::endl;
        std::cout << "Actual Buffer Size: " << d.getBufferSize() << " frames" << std::endl;
        std::cout << "Actual Period Size: " << d.getPeriodSize() << " frames" << std::endl;

        alsa::AudioCallback callback = [&](const alsa::AudioBuffer& buffer, float& phase) {
            constexpr const float pi = 3.14159265;
            constexpr const float freq = 220;
            float step = 2. * pi * freq / (float)buffer.rate;
            for (uint64_t i = 0; i < buffer.periodSize; i++) {
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
