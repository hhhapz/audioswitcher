#pragma once

#include <cstdio>
#include <optionparser.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/thread-mainloop.h>

#include <string>
#include <vector>

struct Sink {
  uint index;
  std::string name;
  std::string desc;
};

class AudioSwitcher {
public:
  std::string server_name;
  std::string server_version;
  std::string default_sink;
  std::vector<Sink> sinks;

  AudioSwitcher();

  void init();

  ~AudioSwitcher();

  void load_server_info();
  void load_sinks();
  void set_default_sink(const std::string &name);

private:
  pa_threaded_mainloop *loop;
  pa_mainloop_api *loop_api;
  pa_context *ctx;

  /// CALLBACKS

  void context_notify_cb(pa_context *ctx);
  void context_success_cb(pa_context *ctx);
  void server_info_cb(pa_context *ctx, const pa_server_info *info);
  void sink_list_cb(pa_context *ctx, const pa_sink_info *info, int eol);

  /// WRAPPERS

  static void context_notify_wrapper(pa_context *ctx, void *data);
  static void context_success_wrapper(pa_context *ctx, int success, void *data);
  static void server_info_wrapper(pa_context *ctx, const pa_server_info *info, void *data);
  static void sink_list_wrapper(pa_context *ctx, const pa_sink_info *info, int eol, void *data);
};
