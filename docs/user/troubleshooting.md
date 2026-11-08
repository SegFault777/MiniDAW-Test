# Troubleshooting

## There is no sound
1. Is the name of the audio device on the right of the status bar? If it says "No audio output", pick a device in **Preferences > Output device**. *Play Test Tone* helps to check it.
2. Does the master level meter move? If so, the program is making sound: check the Windows volume and your speakers.
3. Is **M** (mute) on for the track, or **S** (solo) on for another track?
4. Does the Software Instrument track have an **instrument**? Check in the inspector.

## Sound crackles or drops out
Raise **Preferences > Buffer size** by one step (for example 512 -> 1024). Closing other heavy programs or using fewer plug-ins helps too.

## "Audio device lost"
You unplugged the audio device or Windows switched devices. Restart MiniDAW.

## Recording does not work / comes in too late
- **R** must be on for the track ("Arm a track with the R button first.").
- Check that the microphone is not blocked in Windows Settings > Privacy > Microphone.
- If it comes in late, adjust **Recording latency compensation**.

## A file cannot be opened or imported
- WAV, MP3, AAC/M4A, FLAC and WMA open. OGG may not open with the default Windows codecs.
- Project files are `.mdaw`.

## The program closed strangely
Start it again: it offers to restore the autosaved project. Save it after restoring.
If the problem repeats, delete `native-settings.ini` / `vulkan-settings.ini` in `%LOCALAPPDATA%\MiniDAW` (portable: in `data`) - this resets the settings - and try again.

## The Vulkan version does not start
- "vulkan-1.dll was not found": update the graphics driver. You can use `MiniDAW.exe` meanwhile.
- "The shader files ... are missing": the `shaders` folder must sit next to `MiniDAWVulkan.exe`. Use the whole zip, not the exe alone.
- A black or frozen window: update the driver; if it persists, report it with the name of your graphics card (shown by the error message or Task Manager).

## Text looks blurry or too small (Native)
The Native version is stretched by Windows on high-resolution screens and can look a little soft. The **Vulkan version** is drawn at the display scale and stays sharp; its *Interface scale* preference makes everything larger or smaller.

## Reporting
Send bugs and suggestions as GitHub issues, with the version (`About MiniDAW`), what you tried to do and the message that appeared.
