#include <atomic>
#include <math.h>
#include <memory>
#include <thread>

#include <device.hpp>
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

uint32_t Device::getBufferTime() const {
    uint32_t actual_buffer_time;
    snd_pcm_hw_params_get_buffer_time(_hwParams.get(), &actual_buffer_time, NULL);
    return actual_buffer_time;
}

uint32_t Device::getPeriodTime() const {
    uint32_t actual_period_time;
    snd_pcm_hw_params_get_period_time(_hwParams.get(), &actual_period_time, NULL);
    return actual_period_time;
}

uint32_t Device::getRate() const {
    uint32_t actual_rate;
    snd_pcm_hw_params_get_rate(_hwParams.get(), &actual_rate, NULL);
    return actual_rate;
}

uint32_t Device::getNumChannels() const {
    uint32_t numChannels;
    snd_pcm_hw_params_get_channels(_hwParams.get(), &numChannels);
    return numChannels;
}

uint64_t Device::getBufferSize() const {
    snd_pcm_uframes_t bufferSize;
    snd_pcm_hw_params_get_buffer_size(_hwParams.get(), &bufferSize);
    return bufferSize;
}

uint64_t Device::getPeriodSize() const {
    snd_pcm_uframes_t periodSize;
    snd_pcm_hw_params_get_period_size(_hwParams.get(), &periodSize, NULL);
    return periodSize;
}

void Device::setResampleRate(const uint32_t resampleRate) {
    alsaCall(snd_pcm_hw_params_set_rate_resample, "set resample rate", _device.get(), _hwParams.get(), resampleRate);
}

void Device::setAccessMode(const AccessMode mode) {
    alsaCall(snd_pcm_hw_params_set_access, "set access mode", _device.get(), _hwParams.get(), static_cast<NativeAccessModeT>(mode));
}

void Device::setNumChannels(uint32_t numChannels) {
    alsaCall(snd_pcm_hw_params_set_channels, "set number of channels", _device.get(), _hwParams.get(), numChannels);
}

uint32_t Device::setRateNear(uint32_t rate) {
    alsaCall(snd_pcm_hw_params_set_rate_near, "set rate near", _device.get(), _hwParams.get(), &rate, nullptr);
    return rate;
}

uint32_t Device::setBufferTimeNear(uint32_t bufferTime) {
    alsaCall(snd_pcm_hw_params_set_buffer_time_near, "set buffer time near", _device.get(), _hwParams.get(), &bufferTime, nullptr);
    return bufferTime;
}

uint32_t Device::setPeriodTimeNear(uint32_t periodTime) {
    alsaCall(snd_pcm_hw_params_set_period_time_near, "set period_time near", _device.get(), _hwParams.get(), &periodTime, nullptr);
    return periodTime;
}

void Device::registerAudioCallback(const AudioCallback& callback) {
    _audioCallback = callback;
}

void Device::prepare() {
    alsaCall(snd_pcm_hw_params, "propagate hw params on device", _device.get(), _hwParams.get());
    prepare(_device.get());
    _audioBuffer.periodSize = getPeriodSize();
    _audioBuffer.ringBufferSize = getBufferSize();
    _audioBuffer.numChannels = getNumChannels();
    _audioBuffer.rate = getRate();
}

void Device::start() {
    std::thread* oldThread = _audioThread.load(std::memory_order_relaxed);
    if (oldThread == nullptr) {
        throw AlsaException(-EEXIST, "start already started device");
    }
  
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
    float phase = 0.;
    _audioBuffer.samples = new uint8_t[_audioBuffer.periodSize * _audioBuffer.numChannels]; 
   
    while (_audioThread.load(std::memory_order_acquire)) {
        _audioCallback(_audioBuffer, phase);
        uint8_t* ptr = _audioBuffer.samples;
        uint64_t cptr = _audioBuffer.periodSize;
        while (cptr > 0) {
            int err = snd_pcm_writei(_device.get(), ptr, cptr);
            if (err == -EAGAIN)
                continue;
            if (err < 0) {
                alsaCall(recovery, "write from buffer to device", _device.get(), err);
            }
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
