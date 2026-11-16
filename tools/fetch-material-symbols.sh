#!/usr/bin/env bash
# Downloads the Material Symbols fonts (Google, Apache-2.0) for the icon style setting of the Vulkan version into resources/fonts.
# build.sh copies that folder next to MiniDAWVulkan.exe (fonts\). Without the files MiniDAW uses its classic icons.
#
#   tools/fetch-material-symbols.sh                 all three styles (Outlined, Rounded, Sharp; about 10 MB each)
#   MATERIAL_STYLES="Outlined" tools/fetch-material-symbols.sh
#
# Each style needs two files: the font (.ttf) and the list of names and code points (.codepoints). They are saved as
# MaterialSymbols<Style>.ttf and MaterialSymbols<Style>.codepoints. The default instance of the variable font is used (weight 400,
# optical size 24, not filled). A style that cannot be downloaded is skipped with a message; the script never fails the build.
set -u
cd "$(dirname "${BASH_SOURCE[0]}")/.."
OUT="resources/fonts"
STYLES="${MATERIAL_STYLES:-Outlined Rounded Sharp}"
BASE="${MATERIAL_SYMBOLS_URL:-https://github.com/google/material-design-icons/raw/master/variablefont}"
AXES="%5BFILL%2CGRAD%2Copsz%2Cwght%5D"
mkdir -p "$OUT"
ok=0
for s in $STYLES; do
    for ext in ttf codepoints; do
        url="$BASE/MaterialSymbols${s}${AXES}.${ext}"
        tmp="$OUT/.MaterialSymbols${s}.${ext}.part"
        if curl -fL --retry 3 --connect-timeout 20 -o "$tmp" "$url" 2>/dev/null && [ -s "$tmp" ]; then
            mv -f "$tmp" "$OUT/MaterialSymbols${s}.${ext}"
        else
            rm -f "$tmp"
            echo "could not download $url" >&2
            rm -f "$OUT/MaterialSymbols${s}.ttf" "$OUT/MaterialSymbols${s}.codepoints"
            continue 2
        fi
    done
    echo "Material Symbols $s: $(wc -c < "$OUT/MaterialSymbols${s}.ttf") bytes"
    ok=$((ok + 1))
done
cat > "$OUT/LICENSE.txt" <<'LIC'
Material Symbols are Copyright Google LLC and licensed under the Apache License, Version 2.0
(https://www.apache.org/licenses/LICENSE-2.0). Source: https://github.com/google/material-design-icons
LIC
echo "$ok style(s) ready in $OUT"
exit 0
