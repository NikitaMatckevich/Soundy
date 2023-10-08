#include <memory>

#include <device.hpp>
#include <utils.hpp>

namespace alsa {

Device& Device::setResampleRate(std::uint32_t resampleRate) {
    return *this;
}

Device& Device::setAccess(AccessMode mode) {
    return *this;
}

Device& Device::setChannels(std::uint32_t numChannels) {
    return *this;
}

Device& Device::setRateNear(std::uint32_t rate) {
    return *this;
}

void Device::prepare() {

}

} // namespace alsa
