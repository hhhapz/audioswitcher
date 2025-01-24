## audioswitcher

simple pulseaudio tool to list and change audio sink case insensitively.

## Install:

### System requirements

- C++ compiler with C++23 support
- CMake (3.16 or later)
- `libpulse` on your system (aka `libpulse-dev`)

### Compile

```bash

$ cmake -S . -B build
$ cmake --build build
$ ./audioswitcher

```

## Usage

```
Usage: audioswitcher [opts]

Options:
  -h, --help   Show this help
  -l, --list   Show available sinks
  -s, --set    Set new default sink
  -s, --exact  Require sink identifier to match exactly

Notes:
  sink identifiers can match the value of the index,
  id or a case insensitive substring of the name (except with --exact).
```
