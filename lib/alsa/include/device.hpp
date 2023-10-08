#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "states.hpp"
#include "utils.hpp"

namespace alsa {

struct Device {

    inline Device(std::string_view deviceId, StreamMode mode)
        : _device(initDevice(deviceId, mode), &snd_pcm_close)
        , _hwParams(initHwParams(_device.get()), &snd_pcm_hw_params_free)
    {}
   
    Device& setResampleRate(std::uint32_t resampleRate);
    Device& setAccess(AccessMode mode);
    Device& setChannels(std::uint32_t numChannels);
    Device& setRateNear(std::uint32_t rate);
    void prepare();

private:
    using NativeDeviceT = snd_pcm_t;
    using NativeDeviceHwParamsT = snd_pcm_hw_params_t;
    using NativeStreamT = snd_pcm_stream_t;
    using NativeDeviceDeleterT = decltype(&snd_pcm_close);
    using NativeDeviceHwParamsDeleterT = decltype(&snd_pcm_hw_params_free);

    std::unique_ptr<NativeDeviceT, NativeDeviceDeleterT> _device;
    std::unique_ptr<NativeDeviceHwParamsT, NativeDeviceHwParamsDeleterT> _hwParams;

    static inline NativeDeviceT* initDevice(std::string_view deviceId, StreamMode mode) {
        NativeDeviceT* device = nullptr;
        alsaCall(snd_pcm_open, "cannot open", &device, deviceId.data(), static_cast<NativeStreamT>(mode), 0);
        return device;
    }
    
    static inline NativeDeviceHwParamsT* initHwParams(NativeDeviceT* device) {
        NativeDeviceHwParamsT* hwParams = nullptr;
        alsaCall(snd_pcm_hw_params_malloc, "cannot allocate hw params", &hwParams);
        alsaCall(snd_pcm_hw_params_any, "cannot set default hw params", device, hwParams);
        return hwParams;
    }
};

} // namespace alsa
