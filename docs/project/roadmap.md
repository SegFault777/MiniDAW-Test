# Roadmap

In order of priority. This list is a direction, not a promise.

## Near term (around v0.12 / v0.13)
1. **Build and run the Vulkan interface on Windows.** `VkMain.cpp`, `VkRenderer.cpp`, the shaders and the new `build.sh` / CI parts were only written and syntax-checked on Linux. Fix the errors, then compare a real GPU's output with the CPU screenshots.
2. **Check CI and stabilise.** Compile the Native build (especially VST3) and the `win32` changes (portable paths, folder dialog) on Windows for the first time.
3. **Finish the translations:** about 150 new English texts need `ko`, `ja`, `zh_CN` and `es` entries.
4. **Vulkan interface gaps:** a sample editor for audio regions, mixer groups, IME composition in text input.
5. ~~Performance~~ done: playback refreshes only the meters; the whole project is copied when it changes.
6. **Project file compatibility:** the `snapshots` field is new (older versions ignore it); decide whether to raise the file version.
7. **Decide the future of the Qt version** once nothing exists only in Qt (the Native and Vulkan versions already have its features).

## Next
- **Session view (clip launcher):** an engine function that starts and stops clips on the beat, and a grid screen.
- **Flex editing** of audio regions (beat markers), pitch correction.
- More automation targets (plug-in parameters), curve shapes.
- Choice of metronome sound, pre-roll.
- A "package save" that includes the samples in the project.
- Touch and pen input, accessibility.

## Long term
- ASIO or WASAPI exclusive mode for lower latency
- Porting to macOS / Linux (only the `platform` and shell layers must be rewritten; Vulkan helps here)
- Score view
- Instruments and MIDI effects in the plug-in SDK

## Decided against
- Introducing an external UI framework (Qt and the like) again
- Cloud accounts or online features
