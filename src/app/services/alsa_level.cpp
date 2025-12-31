#include "app/services/alsa_level.hpp"

#include <alsa/asoundlib.h>
#include <thread>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

AlsaLevel::AlsaLevel(QObject *p) : QObject(p) {}
AlsaLevel::~AlsaLevel() { stop(); }

void AlsaLevel::start()
{
    if (running_.exchange(true)) return;
    std::thread([this]{ run(); }).detach();
}

void AlsaLevel::stop()
{
    running_.store(false);
}

void AlsaLevel::run()
{
    snd_pcm_t *pcm = nullptr;

    // Capture side of snd-aloop
    const char *dev = "hw:Loopback,1,0";

    if (snd_pcm_open(&pcm, dev, SND_PCM_STREAM_CAPTURE, 0) < 0)
        return;

    // Configure: S16_LE, stereo, 48k (matches how we recorded)
    snd_pcm_hw_params_t *hw = nullptr;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(pcm, hw);

    snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, hw, 2);

    unsigned int rate = 48000;
    snd_pcm_hw_params_set_rate_near(pcm, hw, &rate, nullptr);

    // Low-latency-ish buffer
    snd_pcm_uframes_t period = 480; // 10ms @ 48k
    snd_pcm_hw_params_set_period_size_near(pcm, hw, &period, nullptr);

    if (snd_pcm_hw_params(pcm, hw) < 0) {
        snd_pcm_close(pcm);
        return;
    }

    snd_pcm_prepare(pcm);

    std::vector<int16_t> buf(period * 2); // stereo interleaved (L,R)

    while (running_.load()) {
        snd_pcm_sframes_t frames = snd_pcm_readi(pcm, buf.data(), period);
        if (frames < 0) {
            snd_pcm_recover(pcm, (int)frames, 1);
            continue;
        }
        if (frames == 0) continue;

        const size_t samples = (size_t)frames * 2;

double sumL = 0.0;
double sumR = 0.0;

for (size_t i = 0; i + 1 < samples; i += 2) {
    const int l = buf[i];
    const int r = buf[i + 1];

    const double lf = (double)l / 32768.0;
    const double rf = (double)r / 32768.0;

    sumL += lf * lf;
    sumR += rf * rf;
}

const size_t framesCount = (size_t)frames;
const float rmsL = (framesCount > 0) ? (float)std::sqrt(sumL / (double)framesCount) : 0.f;
const float rmsR = (framesCount > 0) ? (float)std::sqrt(sumR / (double)framesCount) : 0.f;

// mild gain; clamp
float lvlL = rmsL * 2.2f;
float lvlR = rmsR * 2.2f;

if (lvlL > 1.f) lvlL = 1.f;
if (lvlR > 1.f) lvlR = 1.f;
if (lvlL < 0.f) lvlL = 0.f;
if (lvlR < 0.f) lvlR = 0.f;

emit level(lvlL, lvlR);

    }

    snd_pcm_close(pcm);
}
