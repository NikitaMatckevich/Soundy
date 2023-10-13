#include <atomic>
#include <math.h>
#include <memory>

#include <device.hpp>
#include <thread>
#include <utils.hpp>

namespace alsa {

static inline void clearAudioThread(std::atomic<std::thread*>* audioThread) {
    std::thread* thread;
    while (!audioThread->compare_exchange_weak(
            thread,
            nullptr,
            std::memory_order_release,
            std::memory_order_relaxed));
    thread->join();
    delete thread;
}

void Device::setResampleRate(const uint32_t resampleRate) {
    alsaCall(snd_pcm_hw_params_set_rate_resample, "set resample rate", _device.get(), _hwParams.get(), resampleRate);
}

void Device::setAccessMode(const AccessMode mode) {
    alsaCall(snd_pcm_hw_params_set_access, "set access mode", _device.get(), _hwParams.get(), static_cast<NativeAccessModeT>(mode));
}

void Device::setNumChannels(uint32_t numChannels) {
    _audioBuffer.numChannels = numChannels;
    alsaCall(snd_pcm_hw_params_set_channels, "set number of channels", _device.get(), _hwParams.get(), numChannels);
}

uint32_t Device::setRateNear(uint32_t rate) {
    alsaCall(snd_pcm_hw_params_set_rate_near, "set rate near", _device.get(), _hwParams.get(), &rate, nullptr);
    _audioBuffer.rate = rate;
    return rate;
}

uint32_t Device::setBufferTimeNear(uint32_t bufferTime) {
    alsaCall(snd_pcm_hw_params_set_buffer_time_near, "set buffer time near", _device.get(), _hwParams.get(), &bufferTime, nullptr);
    _audioBuffer.ringBufferTime = bufferTime;
    return bufferTime;
}

uint32_t Device::setPeriodTimeNear(uint32_t periodTime) {
    alsaCall(snd_pcm_hw_params_set_period_time_near, "set period_time near", _device.get(), _hwParams.get(), &periodTime, nullptr);
    _audioBuffer.periodTime = periodTime;
    return periodTime;
}

void Device::registerAudioCallback(const AudioCallback& callback) {
    _audioCallback = callback;
}

void Device::start() {
    std::thread* oldThread = _audioThread.load(std::memory_order_relaxed);
    if (oldThread == nullptr) {
        throw AlsaException(-EEXIST, "start already started device");
    }
 
    alsaCall(snd_pcm_hw_params, "propagate hw params on device", _device.get(), _hwParams.get());
    prepare(_device.get());
   
    std::thread* newThread = new std::thread(&Device::audioThreadLoop, this);
    if (!_audioThread.compare_exchange_weak(
            oldThread,
            newThread,
            std::memory_order_relaxed,
            std::memory_order_relaxed)){
        throw AlsaException(-EEXIST, "start device concurrently from mutiple threads");
    }
}

void Device::drain() {
    clearAudioThread(&_audioThread);
    alsaCall(snd_pcm_drain, "drain device", _device.get());
}

void Device::drop() {
    clearAudioThread(&_audioThread);
    alsaCall(snd_pcm_drop, "drop device immediately", _device.get());
}

Device::NativeDeviceT* Device::initDevice(std::string_view deviceId, StreamUsageMode usageMode, StreamOpenMode openMode) {
    NativeDeviceT* device = nullptr;
    alsaCall(snd_pcm_open, "open device", &device, deviceId.data(), static_cast<NativeStreamT>(usageMode), static_cast<int32_t>(openMode));
    return device;
}

Device::NativeDeviceHwParamsT* Device::initHwParams(NativeDeviceT* device) {
    NativeDeviceHwParamsT* hwParams = nullptr;
    alsaCall(snd_pcm_hw_params_malloc, "allocate hw params", &hwParams);
    alsaCall(snd_pcm_hw_params_any, "assign default hw params", device, hwParams);
    alsaCall(snd_pcm_hw_params_set_format, "set format", device, hwParams, SND_PCM_FORMAT_U8);
    return hwParams;
}

void Device::prepare(Device::NativeDeviceT* device) {
    alsaCall(snd_pcm_prepare, "prepare device", device); 
}

void Device::audioThreadLoop() {
    double phase = 0.;

    _audioBuffer.samples = new uint8_t[_audioBuffer.periodTime * _audioBuffer.numChannels];
    
    while (_audioThread.load(std::memory_order_acquire)) {
        _audioCallback(_audioBuffer, phase);
        uint8_t* ptr = _audioBuffer.samples;
        uint32_t cptr = _audioBuffer.periodTime;
        while (cptr > 0) {
            int err = snd_pcm_writei(_device.get(), ptr, cptr);
            if (err == -EAGAIN)
                continue;
            alsaCall(recovery, "write from buffer to device", _device.get(), err);
            ptr += err * _audioBuffer.numChannels;
            cptr -= err;
        }
    }

    delete [] _audioBuffer.samples;
}

int Device::recovery(Device::NativeDeviceT* device, int err) {
    if (err == -EPIPE) {    /* under-run */
        prepare(device);
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(device)) == -EAGAIN)
            sleep(1);   /* wait until the suspend flag is released */
        if (err < 0) {
            prepare(device);
        }
        return 0;
    }
    return err;
}

} // namespace alsa
