# Granny

`Granny` is a JUCE AU/VST3/Standalone plugin scaffold for a granular delay and freeze sampler.

## Build

Point CMake at your JUCE checkout:

```bash
cmake -B build -DJUCE_DIR=/path/to/JUCE
cmake --build build --config Release
```

## Current feature set

- Mono or stereo input, stereo output
- Circular capture buffer adjustable from 0.1 to 20 seconds
- Freeze mode with scrub position control
- Granular controls for grain size, launch density, speed, regen, and wet mix
- Playback direction modes: forward, reverse, random reverse
- Pitch modes: original, random octave up, random octave up/down
- Global wet-clock transposition from `-12` to `+12` semitones
