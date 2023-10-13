#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>
#include <thread>

#include "callback.hpp"
#include "states.hpp"
#include "utils.hpp"

namespace alsa {

struct Device {

    inline Device(std::string_view deviceId, StreamUsageMode usageMode, StreamOpenMode openMode)
        : _device(initDevice(deviceId, usageMode, openMode), &snd_pcm_close)
        , _hwParams(initHwParams(_device.get()), &snd_pcm_hw_params_free)
    {}

    void setResampleRate(uint32_t resampleRate);
    void setAccessMode(AccessMode mode);
    void setNumChannels(std::uint32_t numChannels);
    uint32_t setRateNear(uint32_t rate);
    uint32_t setBufferTimeNear(uint32_t bufferTime);
    uint32_t setPeriodTimeNear(uint32_t periodTime);
    void registerAudioCallback(const AudioCallback& callback);
    void start();
    void drain();
    void drop();

private:
    using NativeDeviceT = snd_pcm_t;
    using NativeDeviceHwParamsT = snd_pcm_hw_params_t;
    using NativeStreamT = snd_pcm_stream_t;
    using NativeAccessModeT = snd_pcm_access_t;
    using NativeDeviceDeleterT = decltype(&snd_pcm_close);
    using NativeDeviceHwParamsDeleterT = decltype(&snd_pcm_hw_params_free);

    std::unique_ptr<NativeDeviceT, NativeDeviceDeleterT> _device;
    std::unique_ptr<NativeDeviceHwParamsT, NativeDeviceHwParamsDeleterT> _hwParams;

    std::atomic<std::thread*> _audioThread;
    AudioBuffer _audioBuffer;
    AudioCallback _audioCallback;

    void audioThreadLoop();

    static NativeDeviceT* initDevice(std::string_view deviceId, StreamUsageMode usageMode, StreamOpenMode openMode);
    static NativeDeviceHwParamsT* initHwParams(NativeDeviceT* device);
    static void prepare(NativeDeviceT* device);
    static int recovery(NativeDeviceT* device, int err);
};

} // namespace alsa
