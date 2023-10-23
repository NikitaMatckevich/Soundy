#pragma once

#include <cstdint>
#include <functional>

namespace alsa {

struct AudioBuffer {
    uint64_t periodSize;
    uint64_t ringBufferSize;
    uint32_t numChannels;
    uint32_t rate;
    uint8_t* samples;
};

using AudioCallback = std::function<void(const AudioBuffer&, float&)>;

} // namespace alsa
