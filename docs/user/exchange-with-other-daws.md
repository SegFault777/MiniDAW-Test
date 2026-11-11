# Working with other programs (Logic Pro, Ableton Live, Cubase and more)

MiniDAW reads and writes the file types that every DAW understands.

## Opening and importing files
**File > Open**, drag and drop onto the window, and "Open with" accept three kinds of files:

| Kind | Files | What happens |
| --- | --- | --- |
| MiniDAW project | `.mdaw` | opens (after asking about unsaved changes) |
| Audio | `.wav` `.aif` `.aiff` `.aifc` `.mp3` `.mp2` `.m4a` `.mp4` `.m4b` `.3gp` `.aac` `.flac` `.wma` `.ogg` `.oga` `.opus` | imported as a new audio track at the playhead (or onto the selected audio track) |
| MIDI | `.mid` `.midi` `.kar` | imported as new tracks; in an empty project the tempo changes, time signatures and markers of the file are adopted too |

WAV and AIFF are read by MiniDAW itself. The other audio formats are decoded by Windows, so a format that Windows has no codec for (for example Ogg Vorbis on some systems) shows an error message for that file only. Files with another sample rate (48, 88.2, 96 kHz ...) are converted to 44.1 kHz with a high quality filter.

## Bounce / Export Audio (Ctrl+B)
| Choice | Options |
| --- | --- |
| Format | WAV 16-bit, WAV 24-bit, WAV 32-bit float, AIFF 16-bit, AIFF 24-bit, MP3 |
| Sample rate | 44.1, 48, 88.2, 96 kHz (MP3: 44.1 or 48 kHz) |
| MP3 bit rate | 128, 160, 192, 224, 256, 320 kbps |
| Tags (MP3) | title, artist, album; the comment "Made with MiniDAW" is added |
| Range | whole project or only the cycle range; optional 2 s tail for reverb and echo |
| Level | optional normalize, optional dither for 16-bit |
| Stems | one file for every track |

MP3 export uses the MP3 encoder of Windows (Media Foundation), so nothing extra is installed. WAV and AIFF are written by MiniDAW itself, so they work on any system.

### Stems (one file for every track)
Tick **One file for every track** and choose a folder. Every track that makes sound becomes a file called `01 Name.wav`, `02 Name.wav` ... All files start at the same point and have exactly the same length, so you can drop them all into another DAW at bar 1 and they line up. Level and panning of the tracks are included, the master effects are too, and the mute / solo state of your project is not changed.

Tips for the other program:
- **Logic Pro**: File > Import > Audio File (or drag the files onto the tracks area); use **AIFF 24-bit** or **WAV 24-bit** at your project's sample rate.
- **Ableton Live, Cubase, Pro Tools, FL Studio**: drag the stems into the arrangement at bar 1.

## MIDI export and import
**File > Export MIDI File...** writes a Standard MIDI File (format 1). Besides the notes it contains:
- the whole **tempo map** (a tempo glide becomes one step per beat, because MIDI has no gliding tempo),
- every **time signature** change,
- the **markers** of the project,
- one track per instrument or Drummer track, with its name (Drummer tracks on channel 10).

Importing reads the same things back. Logic Pro, Ableton Live, Cubase, Pro Tools, FL Studio and others read and write these files, including their tempo and marker tracks.

## What is not exchanged
- Plug-in settings and automation are not part of audio or MIDI files.
- MP3 is a compressed format: use WAV or AIFF to move material between programs, and MP3 only for listening.
