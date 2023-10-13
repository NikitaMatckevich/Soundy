#pragma once

#include <cstdint>
#include <functional>

namespace alsa {

struct AudioBuffer {
    uint8_t* samples;
    uint32_t periodTime;
    uint32_t ringBufferTime;
    uint32_t numChannels;
    uint32_t rate;
};

using AudioCallback = std::function<void(const AudioBuffer&, double&)>;

} // namespace alsa
