# DI Guided Gate

`DI Guided Gate` is a JUCE-based plugin concept for cleaning up high-gain guitar recordings by using a clean DI performance as the detector signal.

The intended workflow is:

1. Put the plugin on the mic'd amp track.
2. Route the clean DI track into the plugin's sidechain input.
3. Let the plugin follow the DI envelope and reduce amp hiss, hum, and squeal between phrases without destroying note attacks and sustains.

## Current Implementation

The current version includes:

- Stereo main input/output with stereo sidechain input
- `AudioProcessorValueTreeState` parameter management
- Envelope-following detector driven by the sidechain bus
- Threshold, range, attack, hold, release, and hysteresis controls
- Hold logic to reduce chatter on note decay
- Expander-style gain taper instead of a hard binary closed floor
- Minimal JUCE editor for fast listening tests

## Files

- `CMakeLists.txt`
- `PluginProcessor.h`
- `PluginProcessor.cpp`
- `PluginEditor.h`
- `PluginEditor.cpp`

## Suggested Listening Start Point

- `Threshold`: `-36 dB`
- `Range`: `-60 dB`
- `Attack`: `2 ms`
- `Hold`: `50 ms`
- `Release`: `180 ms`
- `Hysteresis`: `3 dB`

## Build

This project was developed inside the JUCE source tree under `examples/CMake/DIGuidedGate`.

The `CMakeLists.txt` in this repo expects JUCE CMake functions such as `juce_add_plugin`, so you should either:

1. add this folder into a JUCE-based superproject, or
2. place it inside a JUCE checkout and add it with `add_subdirectory(...)`.

### Recommended Local Setup

Keep this repo standalone and point a small wrapper CMake project at your JUCE clone.

For example, if you have:

- `../JUCE-repo`
- `../di-guided-gate`

you can create a small parent project with a `CMakeLists.txt` like this:

```cmake
cmake_minimum_required(VERSION 3.22)
project(DIGuidedGateDev)

add_subdirectory(../JUCE-repo JUCE-build)
add_subdirectory(../di-guided-gate DIGuidedGate-build)
```

Then configure and build from that wrapper project:

```sh
cmake -S . -B build
cmake --build build --target DIGuidedGate_VST3 -j4
```

This is the cleanest setup for ongoing work because:

- the plugin stays in its own public repo
- JUCE stays in its own upstream clone
- you can update either one independently

### If You Want It Inside a JUCE Checkout

If you prefer to work directly inside a JUCE clone, copy this folder into the JUCE tree and add it from the parent examples CMake file, for example:

```cmake
add_subdirectory(DIGuidedGate)
```

That is fine for local experimentation, but it is not the better long-term repo layout unless you plan to keep the plugin as part of a JUCE fork.

Example configure/build flow:

```sh
cmake -S /path/to/JUCE -B /tmp/juce-di-gate-build -DJUCE_BUILD_EXAMPLES=ON -DJUCE_BUILD_EXTRAS=ON
cmake --build /tmp/juce-di-gate-build --target DIGuidedGate_VST3 -j4
```

The built VST3 artifact is currently produced at:

`/tmp/juce-di-gate-build/examples/CMake/DIGuidedGate/DIGuidedGate_artefacts/VST3/DI Guided Gate.vst3`

This repo may also include a prebuilt macOS VST3 bundle under `dist/` for convenience.

## Test Flow

One useful way to audition it:

1. Open `AudioPluginHost` or a DAW with sidechain routing support.
2. Insert `DI Guided Gate` on the amp track.
3. Feed the clean DI into the plugin's sidechain bus.
4. Adjust `Hold` and `Range` first, then fine-tune threshold and release by ear.

## Next Step

The logical next feature after a listening pass is lookahead.

That means delaying the amp track slightly so gain reduction starts before the transient, which should make the gate feel cleaner on fast pick attacks. That work should be done in a separate pass because it needs a delay line plus correct host latency reporting via `setLatencySamples()`.
