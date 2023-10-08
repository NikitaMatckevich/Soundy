#pragma once 

#include <stdexcept>
#include <type_traits>

#include <alsa/asoundlib.h>

namespace alsa {

struct AlsaException : public std::runtime_error {
    inline AlsaException(int ec, const std::string& msg) 
        : std::runtime_error(std::string("ALSA ERROR: ") + msg + " " + snd_strerror(ec)) {}
};

template <typename F, typename... Args, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<F, Args...>, int>>>
void alsaCall(F callable, std::string_view errorMsg, Args... args) {
    int ec = callable(args...);
    if (ec < 0) {
        throw AlsaException(ec, std::string(errorMsg));
    }
}

} // namespace alsa
