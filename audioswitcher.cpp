#include "audioswitcher.h"
#include <cstdio>
#include <format>
#include <iostream>
#include <optionparser.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/thread-mainloop.h>
#include <stdexcept>
#include <vector>

#define PA_SYNC_ACTION(func, args, body)                                                                               \
  void AudioSwitcher::func args {                                                                                      \
    pa_threaded_mainloop_lock(loop);                                                                                   \
    pa_operation *op;                                                                                                  \
    body;                                                                                                              \
    pa_threaded_mainloop_wait(loop);                                                                                   \
    pa_threaded_mainloop_unlock(loop);                                                                                 \
    pa_operation_unref(op);                                                                                            \
  }

AudioSwitcher::AudioSwitcher() {
  loop = pa_threaded_mainloop_new();
  loop_api = pa_threaded_mainloop_get_api(loop);
  ctx = pa_context_new(loop_api, "audioswitcher");
};

void AudioSwitcher::init() {
  pa_context_set_state_callback(ctx, &AudioSwitcher::context_notify_wrapper, this);
  if (pa_context_connect(ctx, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
    throw std::runtime_error(std::format("failed to connect to PulseAudio: {}", pa_context_errno(ctx)));
  }
  pa_threaded_mainloop_lock(loop);
  pa_threaded_mainloop_start(loop);

  pa_threaded_mainloop_wait(loop);
  pa_threaded_mainloop_unlock(loop);
};

AudioSwitcher::~AudioSwitcher() {
  pa_threaded_mainloop_stop(loop);
  pa_context_disconnect(ctx);
  pa_context_unref(ctx);
  pa_threaded_mainloop_free(loop);
}

PA_SYNC_ACTION(load_server_info, (), {
  op = pa_context_get_server_info(ctx, server_info_wrapper, this); //
});

PA_SYNC_ACTION(load_sinks, (), {
  sinks.clear();
  op = pa_context_get_sink_info_list(ctx, sink_list_wrapper, this);
});

PA_SYNC_ACTION(set_default_sink, (const std::string &name),
               { op = pa_context_set_default_sink(ctx, name.c_str(), context_success_wrapper, this); });

void AudioSwitcher::context_notify_cb(pa_context *ctx) {
  auto state = pa_context_get_state(ctx);
  switch (state) {
  case PA_CONTEXT_READY:
    break;
  case PA_CONTEXT_FAILED:
    std::cerr << "Failed to complete action: " << pa_context_errno(ctx) << std::endl;
    break;
  default:
    return;
  }
  pa_threaded_mainloop_signal(loop, 0);
}

void AudioSwitcher::context_success_cb(pa_context *ctx) {
  pa_threaded_mainloop_signal(loop, 0);
}

void AudioSwitcher::server_info_cb(pa_context *ctx, const pa_server_info *info) {
  server_name = info->server_name;
  server_version = info->server_version;
  default_sink = info->default_sink_name;
  pa_threaded_mainloop_signal(loop, 0);
}

void AudioSwitcher::sink_list_cb(pa_context *ctx, const pa_sink_info *info, int eol) {
  if (eol) {
    pa_threaded_mainloop_signal(loop, 0);
    return;
  }
  sinks.push_back(Sink{info->index, info->name, info->description});
}

/// WRAPPERS

void AudioSwitcher::context_notify_wrapper(pa_context *ctx, void *data) {
  static_cast<AudioSwitcher *>(data)->context_notify_cb(ctx);
};

void AudioSwitcher::context_success_wrapper(pa_context *ctx, int, void *data) {
  static_cast<AudioSwitcher *>(data)->context_success_cb(ctx);
};

void AudioSwitcher::server_info_wrapper(pa_context *ctx, const pa_server_info *info, void *data) {
  static_cast<AudioSwitcher *>(data)->server_info_cb(ctx, info);
}

void AudioSwitcher::sink_list_wrapper(pa_context *ctx, const pa_sink_info *info, int eol, void *data) {
  static_cast<AudioSwitcher *>(data)->sink_list_cb(ctx, info, eol);
};
