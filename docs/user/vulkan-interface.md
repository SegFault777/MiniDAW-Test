# The Vulkan interface (`MiniDAWVulkan.exe`)

A new interface, written from scratch and drawn by the graphics card through Vulkan. It uses the same engine, project files and sounds as the Native version; what differs is how you work.
It collects the best ideas of five DAWs - and only those.

![Arrangement and mixer](../screenshots/vulkan_01_arrangement_mixer.png)

## Layout
Menu bar - **transport bar** (play, record, cycle; LCD with bar.beat.tick, time, tempo, signature; metronome, count-in, tap tempo; tools; snap and grid; zoom; follow) -
**browser** (left) - **arrangement** (track headers, ruler with cycle strip and markers, lanes) - **inspector** (right) - **bottom panel** with the tabs *Mixer, Editor, Steps, Devices, Snapshots, Automation* - status bar.
Every divider can be dragged. The interface is drawn at your Windows display scale (and the *Interface scale* preference), so text stays sharp on every monitor.

## What it takes from which DAW
| From | Idea | In MiniDAW |
| --- | --- | --- |
| **Logic Pro** | One window with inspector, mixer and editors; the LCD; velocity-coloured notes and the region bar in the piano roll; screensets; Smart Help | Everything in one window, the LCD in the transport bar, velocity colours (`Colour` menu), **workspaces** `F5` Arrange / `F6` Mix / `F7` Edit, hints in the status bar. New functions: **Adapt Project Tempo to Region**, **Groove Template**. |
| **Ableton Live** | Browser with search and drag and drop; device chains with knobs; the **Auto** grid; **Fold** and Scale mode; Info View; Capture MIDI; Transform tools | Browser, Devices tab, Auto grid, Fold / Highlight / Lock, info line in the status bar, **Capture MIDI**, Stretch / Strum / Double Pattern, `Alt`+drag copies. |
| **FL Studio** | Chord stamps, Riff Machine, ghost notes, step sequencer you paint, colour-coded browser | **Stamp** menu, **Riff Machine**, **Ghost** notes, painted steps and painted velocities. |
| **Cubase** | Tool box (1-5), Logical Editor, MixConsole snapshots, workspaces, lower zone | **Tools** `1`-`5`, **Logical Editor**, **Snapshots** tab, workspaces, bottom panel. |
| **Pro Tools** | Edit tools, grid / slip, Tab to Transient, Beat Detective, Shuffle, nudge, zoom | **Snap** (grid / slip), **Tab to Transient**, **Beat Detective**, **Ripple Delete** (`Shift+Delete`), `Alt`+arrows nudge, zoom to fit. |

## Everything is a command: `Ctrl+K`
Press **`Ctrl+K`** (or click the search field in the menu bar), type a few letters - "riff", "quant", "snap", "ripple" - and press `Enter`. Every function of the program is there, with its shortcut.
**`F1`** shows the sheet of all shortcuts. The same command table feeds the menus, the shortcuts, the palette and the sheet, so they can never disagree.

![Command palette](../screenshots/vulkan_palette.png)

## Tools (keys `1` - `5`)
| Key | Tool | What it does |
| --- | --- | --- |
| `1` | Pointer | Select, move, resize regions. `Alt`+drag duplicates. Double-click a MIDI region to edit it. |
| `2` | Pencil | Draw MIDI regions on instrument tracks; in the piano roll, write notes (`B` toggles draw mode there). |
| `3` | Scissors | Click a region to split it at the grid position. |
| `4` | Eraser | Click or drag over regions (arrangement) or notes (piano roll). |
| `5` | Mute | Click a region to mute or unmute it. |

## Snap and grid
**Snap** (`G`) turns the grid on or off; **Auto** follows the zoom level (the coarsest grid whose lines are at least 12 units apart), or choose Bar, 1/2, 1/4, 1/8, 1/16, 1/32, 1/8 triplet (`Shift+G` cycles).
Hold `Alt` while dragging to ignore the grid. The piano roll has its own grid menu.

## Zoom and scrolling
`Ctrl`+wheel zooms at the pointer, `Shift`+wheel or the sideways wheel scrolls in time, the wheel scrolls tracks, `Alt`+wheel changes the track height (or `H` cycles it). `Z` zooms to fit the project, `Shift+Z` to the selected region,
`F` makes the view follow the playhead.

## Mixer and snapshots
Mixer strips line up their faders. Click an insert to open it in **Devices**; right-click for Bypass / Remove. The **Snapshots** tab keeps named mixer states (Cubase MixConsole snapshots); `Alt+1` ... `Alt+9` recall them.

## Piano roll
See the [piano roll section of Editing](editing.md). Open it with a double-click on a MIDI region, or `E`.

## Workspaces
`F5` **Arrange** (browser, inspector, mixer), `F6` **Mix** (the mixer fills the window), `F7` **Edit** (the editor fills the window). `X` shows or hides the bottom panel, `Shift+X` maximizes it, `B` and `I` toggle browser and inspector.

![Mix workspace](../screenshots/vulkan_05_mix_workspace_F6.png)

## If Vulkan does not start
The program tells you what is missing (no Vulkan driver, missing `shaders` folder, ...) and offers to start the Native version `MiniDAW.exe`. Update your graphics driver first.

## Selecting and moving several regions
- **Shift+click** or **Ctrl+click** a region adds it to the selection (or removes it again).
- **Drag on an empty part of a lane** draws a rubber band; every region it touches is selected. **Ctrl+A** selects every region.
- **Drag** any selected region to move the whole group. The group keeps its spacing and never moves before bar 1.
- **Drag over another track** to move the regions there. A region only changes track when it fits: audio on audio tracks, MIDI on instrument and Drummer tracks, Drummer regions on Drummer tracks. Others only move in time.
- **Alt+drag** duplicates the whole selection. **Delete**, **Ctrl+D**, **Ctrl+X / C / V** (the group keeps its spacing and its track layout), **Ctrl+Shift+M** (mute) and **Shift+Delete** (ripple delete) also work on the whole selection. Undo and redo clear the multi-selection.
- Every one of these is a single undo step.

## Tempo lane
Open the Automation tab and choose **Parameter > Tempo (BPM)**. Click to add a tempo change, drag a point to move it, **double-click** a point to make the tempo glide (ramp) to the next point, and right-click to delete it. The first point (bar 1) always stays. The curve is a staircase unless a point ramps. Every change can be undone.

## Drummer tab
Select a Drummer region and press `Ctrl+7` (or double-click the region). Choose a **Style**, then shape the groove with **Complexity**, **Loudness**, **Swing**, **Fills**, **Bars** and **Seed**; **Roll New Variation** picks a new seed. The pattern is shown on the right, one row per drum, and it plays as soon as you change something. Dragging a slider is one undo step.

## Tutorial
**Help > Tutorial...** (the first item of the Help menu) opens a 15-page tutorial that walks through the window, tracks, the browser, the Drummer, recording, the five tools, selecting several regions, the piano roll, the step sequencer, the mixer, automation, snapshots and workspaces, and saving. Use **Next** / **Back**, the Left / Right arrow keys or the dots at the bottom; **Try it** (on some pages) closes the tutorial and opens the matching panel; **Esc** closes it.
