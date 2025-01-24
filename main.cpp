#include "audioswitcher.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <optionparser.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/thread-mainloop.h>
#include <stdexcept>
#include <string>

static option::ArgStatus OptRequired(const option::Option &option, bool msg) {
  if (option.arg != NULL && option.arg[0] != 0)
    return option::ARG_OK;
  if (msg)

    std::cerr << std::format("option '{:.{}s}' requires an argument\n", option.name, option.namelen);
  return option::ARG_ILLEGAL;
}

enum options { UNKNOWN, HELP, LIST, SET, EXACT };
const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None, "Usage: audioswitcher [opts]\n\nOptions:"},
    {HELP, 0, "h", "help", option::Arg::None, "  -h, --help  \tShow this help"},
    {LIST, 0, "-l", "--list", option::Arg::None, "  -l, --list  \tShow available sinks"},
    {SET, 0, "-s", "--set", OptRequired, "  -s, --set  \tSet new default sink"},
    {EXACT, 0, "-e", "--exact", option::Arg::None, "  -s, --exact  \tRequire sink identifier to match exactly"},
    {UNKNOWN, 0, "", "", option::Arg::None, "\nNotes:"},
    {UNKNOWN, 0, "", "", option::Arg::None,
     "  sink identifiers can match the value of the index,\n  id or a case insensitive substring of the name (except "
     "with --exact)."},
    {0, 0, 0, 0, 0, 0},
};

std::string to_lower(const std::string &str) {
  std::string result = str; // copy
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

int digits(int i) {
  int count = 1;
  while (i /= 10)
    count++;
  return count;
}

void list(const AudioSwitcher &as) {
  int indexLen = 5, nameLen = 4, descLen = 11;

  for (auto sink : as.sinks) {
    int len = digits(sink.index);
    if (len > indexLen)
      indexLen = len;

    len = sink.name.length();
    if (len > nameLen)
      nameLen = len;

    len = sink.desc.length();
    if (len > descLen)
      descLen = len;
  }

  std::cout << std::format("{:>{}}  {:<{}}  {:<{}}\n", "Index", indexLen, "Name", nameLen, "Description", descLen);
  for (auto s : as.sinks) {
    std::cout << std::format("{:>{}}  {:<{}}  {:<{}}\n", s.index, indexLen, s.name, nameLen, s.desc, descLen);
  }
}

void set(AudioSwitcher &as, std::string target, bool exact) {
  size_t read = 0;
  int index;
  try {
    index = std::stoi(target, &read, 10);
  } catch (...) {
    // ignore, treat as name/desc
    index = -1;
  }
  if (read != target.length()) {
    index = -1;
  }

  bool found = false;
  Sink sink;

  std::string lower = to_lower(target);
  for (auto s : as.sinks) {
    if (s.index == index) {
      sink = s;
      found = true;
      break;
    }
    if (exact) {
      if (s.name == target || s.desc == target) {
        sink = s;
        found = true;
        break;
      }
      continue;
    }
    if (to_lower(s.name).contains(lower) || to_lower(s.desc).contains(lower)) {
      sink = s;
      found = true;
      break;
    }
  }

  if (!found) {
    if (exact) {
      throw std::runtime_error(std::format("no sink found with exact match to {}", target));
    } else {
      throw std::runtime_error(std::format("no sink found with match to {}", target));
    }
  }

  as.set_default_sink(sink.name);
  std::cerr << std::format("new default sink:\n{}  {}  {}\n", sink.index, sink.name, sink.desc);
}

int run(const option::Option *opts) {
  AudioSwitcher as = AudioSwitcher();
  as.init();
  as.load_server_info();
  as.load_sinks();

  std::sort(as.sinks.begin(), as.sinks.end(), [](Sink const &a, Sink const &b) { return a.index < b.index; });

  if (opts[LIST])
    list(as);

  if (opts[SET]) {
    bool exact = opts[EXACT].count() > 0;
    set(as, opts[SET].arg, exact);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  // skip program name
  argc -= (argc > 0);
  argv += (argc > 0);

  option::Stats stats(usage, argc, argv);
  option::Option options[stats.options_max], buffer[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  if (parse.error())
    return 1;

  bool failed = false;
  for (option::Option *opt = options[UNKNOWN]; opt; opt = opt->next()) {
    std::cerr << std::format("unknown option '-{:.{}s}'\n", opt->name, opt->namelen);
    failed = true;
  }
  for (int i = 0; i < parse.nonOptionsCount(); ++i) {
    std::cerr << std::format("unknown argument '{}'\n", parse.nonOption(i));
    failed = true;
  }

  if (options[HELP] || failed || argc == 0) {
    option::printUsage(std::cerr, usage);
    return 0;
  }

  try {
    return run(options);
  } catch (std::runtime_error err) {
    std::cout << "error: " << err.what() << std::endl;
    return 1;
  }
}
