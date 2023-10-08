#include <device.hpp>

int main() {
    alsa::Device d("default", alsa::StreamMode::SND_PCM_STREAM_PLAYBACK);
    return 0;
}
