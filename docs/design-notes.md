# Design Notes: DI-Guided Guitar Performance Isolation Plugin

## Description

Build a sidechain-capable guitar noise-isolation plugin that uses a clean DI guitar track to
control gain reduction on a high-gain amp track.

The goal is to remove amp hiss, pedalboard hum, microphonic noise, and handling noise between
guitar phrases without damaging pick attacks, note sustains, or the tone of the recorded amp.

This is a better first audio plugin candidate than more complex circuit-modeled compressors,
overdrives, or machine-learning drum bleed tools because the detection problem is simpler:
the clean DI track has sharp transients and a low noise floor, while the distorted amp track is
compressed and noisy.

## Product Concept

The plugin is inserted on the distorted amp track. The clean DI track is routed into the
plugin's sidechain input.

```text
Clean DI Track ----------------------> Sidechain Input
                                           |
                                           v
High-Gain Amp Track --> Main Input --> DI-Guided Gate/Expander --> Cleaned Amp Track
```

The plugin does not make gating decisions from the amp track. Instead, it follows the DI
envelope and uses that envelope to apply smooth attenuation to the amp track.

## Recommended First Milestone

Implement a minimal JUCE audio plugin with:

- Stereo main input and output
- Optional stereo sidechain input
- DI envelope follower
- Threshold control
- Attack control
- Release control
- Floor/range control
- Smooth gain application to the amp track
- Basic editor UI using standard controls

The first milestone should prioritize stable DSP behavior and listening tests over visual
design.

## DSP Approach

For each audio sample:

1. Read the DI sidechain sample
2. Rectify it with absolute value
3. Smooth it through an attack/release envelope follower
4. Compare the envelope against a threshold
5. Apply either unity gain or a configured attenuation floor to the amp track

Initial behavior can be a binary gate. A follow-on improvement should add expander-style soft
transition behavior to avoid abrupt open/close behavior.

## Initial Parameters

| Parameter | Suggested Range | Default | Purpose |
| --- | --- | --- | --- |
| Threshold | -60 dB to 0 dB | -40 dB | DI envelope level required to open the gate |
| Attack | 0.1 ms to 100 ms | 2 ms | How quickly the gate opens |
| Release | 10 ms to 1000 ms | 200 ms | How slowly the gate closes |
| Floor Range | -80 dB to 0 dB | -60 dB | Maximum attenuation when DI is below threshold |

## Implementation Notes

### JUCE Bus Configuration

The processor should expose:

- Main input: stereo
- Main output: stereo
- Sidechain input: optional stereo

The sidechain bus should be used as the detection source. If the host does not provide a
sidechain signal, the plugin should fail gracefully by either passing audio unchanged or using
the main input as a fallback detector, depending on the selected product behavior.

### Envelope Follower

Attack and release coefficients should be recalculated when sample rate changes and whenever
attack/release parameters change.

The initial scaffold calculates coefficients in `prepareToPlay`, but production behavior
should update them when parameters are changed during playback.

### Gain Smoothing

The first scaffold applies target gain directly per sample. That is acceptable for prototype
testing, but the production implementation should smooth gain changes to prevent clicks and
chatter.

Potential improvements:

- Add hold time before release starts
- Add hysteresis between open and close thresholds
- Add soft-knee expansion instead of binary gating
- Add lookahead if pick attacks are clipped

## Risks and Open Questions

- Does the target host support sidechain routing for this plugin format?
- Should the plugin ship as AU, VST3, standalone, or multiple formats?
- What should happen if no sidechain input is connected?
- Is binary gating sufficient, or is expander behavior required?
- Should the plugin preserve stereo amp tracks while detecting from mono DI?
- Should sidechain detection use left channel only, summed stereo, or peak across channels?
- How much latency is acceptable if lookahead is added?
- Should this be a utility-style plugin or a guitar-branded creative tool?

## UI Direction

The first UI should be minimal and functional:

- Threshold knob or slider
- Attack knob or slider
- Release knob or slider
- Floor/range knob or slider
- Gain-reduction meter
- DI envelope meter
- Sidechain-present indicator

Vintage-style visual design can wait until the DSP is validated. A minimal UI will make it
easier to tune behavior and identify whether the core algorithm is working.

## Test Plan

### Local Recording Setup

Record or import paired tracks:

- Clean DI guitar track
- Reamped or simultaneously recorded high-gain Vox AC30 amp track

The DI track should feed the plugin sidechain. The amp track should feed the plugin main input.

### Listening Tests

- Verify amp hiss is attenuated between phrases
- Verify pick attacks are not clipped
- Verify sustained notes are not cut off too early
- Verify release behavior feels natural across palm-muted and ringing parts
- Verify the gate does not chatter during quiet playing
- Verify stereo amp tracks remain phase-stable

### Technical Tests

- Validate behavior when sidechain is present
- Validate behavior when sidechain is absent
- Validate mono and stereo input configurations
- Validate parameter automation during playback
- Validate sample-rate changes
- Validate silence and denormal handling

---

## Checklist

### Discovery

- [x] Confirm target plugin framework and template — JUCE with CMake
- [x] Confirm target plugin formats — AAX, AU, VST3, Standalone
- [x] Confirm target DAW or test host sidechain support — Logic (AU), Pro Tools (AAX), VST3 hosts
- [x] Confirm desired behavior when sidechain is missing — passes main audio unmodified
- [x] Confirm whether prototype should be binary gate or soft expander — expander-style taper with hysteresis

### Phase 1 DSP Prototype

- [x] Configure main input/output and optional sidechain bus
- [x] Add threshold, attack, release, and floor/range parameters
- [x] Implement DI envelope follower
- [x] Apply sidechain-controlled attenuation to main amp input
- [x] Add denormal protection
- [x] Handle sidechain channel selection or summing (peak across channels)
- [x] Recalculate coefficients on sample-rate and parameter changes

### Phase 1 UI

- [x] Add basic controls for threshold, attack, release, floor/range, hold, and hysteresis
- [ ] Add DI envelope meter
- [ ] Add gain-reduction meter
- [ ] Add sidechain-present indicator

### Verification

- [x] Build plugin locally — VST3, AU, AAX artifacts confirmed
- [ ] Load plugin in AudioPluginHost or target DAW
- [ ] Route clean DI track to sidechain
- [ ] Route high-gain amp track to main input
- [ ] Run listening tests on real paired DI/amp recordings
- [ ] Tune defaults based on listening results

### Follow-On Enhancements

- [x] Add hold control
- [x] Add hysteresis
- [x] Add soft-knee expander mode
- [ ] Add lookahead mode
- [ ] Add preset management
- [ ] Add polished guitar-focused UI after DSP behavior is validated
