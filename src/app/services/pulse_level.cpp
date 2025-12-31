#include "app/services/pulse_level.hpp"

#include <pulse/pulseaudio.h>

#include <thread>
#include <vector>
#include <cmath>
#include <cstring>

static std::string to_std(const char *s){ return s ? std::string(s) : std::string(); }

PulseLevel::PulseLevel(QObject *p) : QObject(p) {}
PulseLevel::~PulseLevel() { stop(); }

void PulseLevel::start()
{
    if (running_.exchange(true)) return;
    std::thread([this]{ run(); }).detach();
}

void PulseLevel::stop()
{
    running_.store(false);
}

struct PulseCtx {
    pa_mainloop *ml = nullptr;
    pa_context *ctx = nullptr;
};

static bool wait_ready(PulseCtx &pc)
{
    while (true) {
        pa_mainloop_iterate(pc.ml, 1, nullptr);
        auto st = pa_context_get_state(pc.ctx);
        if (st == PA_CONTEXT_READY) return true;
        if (st == PA_CONTEXT_FAILED || st == PA_CONTEXT_TERMINATED) return false;
    }
}

void PulseLevel::run()
{
    PulseCtx pc;
    pc.ml = pa_mainloop_new();
    pc.ctx = pa_context_new(pa_mainloop_get_api(pc.ml), "opndash-vu");

    pa_context_connect(pc.ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    if (!wait_ready(pc)) { pa_context_unref(pc.ctx); pa_mainloop_free(pc.ml); return; }

    // 1) Get default sink name
    std::string defaultSink;
    bool gotServerInfo = false;
    pa_operation *op = pa_context_get_server_info(pc.ctx,
        [](pa_context*, const pa_server_info *info, void *userdata){
            auto *pair = static_cast<std::pair<std::string*, bool*>*>(userdata);
            *pair->first = to_std(info->default_sink_name);
            *pair->second = true;
        },
        new std::pair<std::string*, bool*>(&defaultSink, &gotServerInfo)
    );

    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) pa_mainloop_iterate(pc.ml, 1, nullptr);
    pa_operation_unref(op);

    if (defaultSink.empty()) { pa_context_disconnect(pc.ctx); pa_context_unref(pc.ctx); pa_mainloop_free(pc.ml); return; }

    // 2) Get monitor source name for that sink
    std::string monitorSource;
    bool gotSink = false;

    struct SinkReq { std::string *mon; bool *done; };
    SinkReq req{ &monitorSource, &gotSink };

    op = pa_context_get_sink_info_by_name(pc.ctx, defaultSink.c_str(),
        [](pa_context*, const pa_sink_info *i, int eol, void *userdata){
            if (eol || !i) return;
            auto *r = static_cast<SinkReq*>(userdata);
            *r->mon = to_std(i->monitor_source_name);
            *r->done = true;
        }, &req);

    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) pa_mainloop_iterate(pc.ml, 1, nullptr);
    pa_operation_unref(op);

    if (monitorSource.empty()) { pa_context_disconnect(pc.ctx); pa_context_unref(pc.ctx); pa_mainloop_free(pc.ml); return; }

    // 3) Create a record stream from monitor source (mono float32)
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = 25;
    ss.channels = 1;

    pa_stream *stream = pa_stream_new(pc.ctx, "vu-meter", &ss, nullptr);
    if (!stream) { pa_context_disconnect(pc.ctx); pa_context_unref(pc.ctx); pa_mainloop_free(pc.ml); return; }

    pa_buffer_attr attr;
    std::memset(&attr, 0, sizeof(attr));
    attr.fragsize = (uint32_t)-1;

    //pa_stream_connect_record(stream, monitorSource.c_str(), &attr,
    //                         (pa_stream_flags_t)(PA_STREAM_ADJUST_LATENCY));

pa_stream_connect_record(
    stream,
    "@DEFAULT_MONITOR@",
    &attr,
    (pa_stream_flags_t)(PA_STREAM_ADJUST_LATENCY | PA_STREAM_PEAK_DETECT)
);


    // Simple polling read loop
    while (running_.load()) {
        pa_mainloop_iterate(pc.ml, 1, nullptr);

        const void *data = nullptr;
        size_t bytes = 0;
        if (pa_stream_peek(stream, &data, &bytes) < 0) break;
        if (!data || bytes == 0) { pa_stream_drop(stream); continue; }

// With PEAK_DETECT, server sends float(s) in range 0..1
const float *f = static_cast<const float*>(data);
const size_t n = bytes / sizeof(float);

float peak = 0.f;
for (size_t i = 0; i < n; ++i)
    if (f[i] > peak) peak = f[i];

// mild gain; clamp
float lvl = peak * 1.6f;
if (lvl > 1.f) lvl = 1.f;

emit level(lvl);

        pa_stream_drop(stream);
    }

    pa_stream_disconnect(stream);
    pa_stream_unref(stream);

    pa_context_disconnect(pc.ctx);
    pa_context_unref(pc.ctx);
    pa_mainloop_free(pc.ml);
}
