#pragma once 

#include <stdexcept>
#include <type_traits>

#include <alsa/asoundlib.h>

namespace alsa {

struct AlsaException : public std::runtime_error {
    inline AlsaException(int ec, const std::string& msg) 
        : std::runtime_error(std::string("ALSA ERROR: failed to ") + msg + " " + snd_strerror(ec)) {}
};

template <typename F, typename... Args, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<F, Args...>, int>>>
void alsaCall(F callable, std::string_view description, Args... args) {
    int ec = callable(args...);
    if (ec < 0) {
        throw AlsaException(ec, std::string(description));
    }
}

template <typename F, typename... Args, typename = std::enable_if_t<std::is_same_v<std::invoke_result_t<F, Args...>, int>>>
void alsaCall(F callable, F recoveryCallback, std::string_view description, Args... args) {
    int ec = callable(args...);
    if (ec < 0) {
        ec = recoveryCallback(args...);
        if (ec < 0) {
            throw AlsaException(ec, std::string(description));
        }
    }
}

} // namespace alsa
