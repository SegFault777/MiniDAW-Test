# Icon style (Vulkan version)

**File > Preferences > Icon style** chooses how the toolbar and panel icons look. The change applies at once.

| Style | Look |
| --- | --- |
| Material Symbols Outlined | Google's Material Symbols, outlined strokes (the default) |
| Material Symbols Rounded | the same icons with rounded corners |
| Material Symbols Sharp | the same icons with sharp corners |
| Classic | the icons MiniDAW has always drawn itself |

## Where the Material Symbols come from
The three Material Symbols styles are font files (Apache-2.0, Google). The installer of the Vulkan version contains them in the `fonts` folder next to
`MiniDAWVulkan.exe`. If you build MiniDAW yourself, run `tools/fetch-material-symbols.sh` once (it needs `curl`); `build.sh` then copies the files.

MiniDAW looks for them in the `fonts` folder next to the program and in the `fonts` folder of the data folder (`%LOCALAPPDATA%\MiniDAW\fonts`, or `data\fonts`
in a portable copy). For each style it needs two files whose names start with `MaterialSymbolsOutlined`, `MaterialSymbolsRounded` or `MaterialSymbolsSharp`:

* the font, `MaterialSymbols<Style>.ttf` (the variable font of Google works; the default weight and size are used), and
* the list of names, `MaterialSymbols<Style>.codepoints` (published by Google next to the font).

A style whose files are missing is marked "(missing)" in the list. Choosing it still works: every icon falls back to the classic drawing, so no button
ever disappears. The same happens for a single icon that the font does not have.

The Native version (`MiniDAW.exe`) keeps its own icons.
