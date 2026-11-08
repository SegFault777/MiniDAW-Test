// Tests and screenshots of the Vulkan user interface (src/vulkan) rendered by the CPU twin of its shader: no GPU needed.
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/core/Files.h"
#include "../src/core/Session.h"
#include "../src/vulkan/SoftRaster.h"
#include "../src/vulkan/Studio.h"
#include "../src/vulkan/Theme.h"

int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)
using namespace core;
using namespace vkui;

static std::string wav(const char* name, double secs, bool beats = false) {
    std::vector<float> pcm(size_t(secs * 44100) * 2);
    for (size_t i = 0; i < pcm.size() / 2; ++i) {
        const double t = double(i) / 44100.0;
        double env = 0.15 + 0.85 * std::fabs(std::sin(t * 3.1)) * std::exp(-std::fmod(t, 0.5) * 3.0);
        if (beats) env = std::exp(-std::fmod(t, 0.5) * 9.0);
        pcm[2 * i] = pcm[2 * i + 1] = float(env * std::sin(t * 2 * 3.14159 * (beats ? 110 : 220) + 3 * std::sin(t * 17)));
    }
    const std::string p = std::string("/tmp/minidaw_vk_") + name;
    std::string e;
    writeWavFile(p, pcm, &e);
    return p;
}

static void buildDemo(Session& s) {
    while (s.trackCount() > 0) s.removeTrack(0);
    s.setBpm(112);
    const int vox = s.addTrack(TrackType::Audio, "Lead Vocal");
    s.importAudio(wav("vox.wav", 14), vox, 0);
    const int loopT = s.addTrack(TrackType::Audio, "Drum Loop");
    s.importAudio(wav("beat.wav", 8, true), loopT, 4);
    const int keys = s.addTrack(TrackType::Instrument, "Keys");
    int r = s.addMidiRegion(keys, 0, 8);
    std::vector<Note> notes;
    const int chords[4][3] = {{60, 64, 67}, {57, 60, 64}, {53, 57, 60}, {55, 59, 62}};
    for (int bar = 0; bar < 8; ++bar) for (int k = 0; k < 3; ++k) notes.push_back({chords[bar % 4][k] + (k == 2 ? 12 : 0), bar * 1.0, 0.9, 90});
    for (int i = 0; i < 16; ++i) notes.push_back({72 + (i * 5) % 9, i * 0.5, 0.4, 70 + (i % 4) * 10});
    s.setRegionNotes(keys, r, notes);
    s.addMidiRegion(keys, 12, 4);
    const int drm = s.addTrack(TrackType::Drummer, "Drummer");
    s.addDrummerRegion(drm, 0, 4);
    s.addDrummerRegion(drm, 16, 4);
    const int bass = s.addTrack(TrackType::Instrument, "Bass");
    r = s.addMidiRegion(bass, 4, 12);
    std::vector<Note> bn;
    for (int i = 0; i < 24; ++i) bn.push_back({36 + (i % 3) * 3 + (i % 8 == 7 ? 7 : 0), i * 0.5, 0.45, 100});
    s.setRegionNotes(bass, r, bn);
    const int steps = s.addTrack(TrackType::Drums, "Drum Machine");
    for (int st = 0; st < 16; st += 4) s.setStep(steps, 0, st, true);
    for (int st = 4; st < 16; st += 8) s.setStep(steps, 1, st, true);
    for (int st = 2; st < 16; st += 2) s.setStep(steps, 2, st, true);
    s.setVolume(vox, 0.9f); s.setVolume(keys, 0.62f); s.setPan(keys, -0.4f); s.setPan(bass, 0.25f);
    s.setMute(loopT, true); s.setArmed(vox, true);
    s.addInsert(vox, "chorus"); s.addInsert(vox, "filter");
    s.addMarker(0); s.addMarker(16);
    s.saveMixSnapshot("Verse");
    s.setVolume(keys, 0.9f);
    s.saveMixSnapshot("Chorus");
    s.setLoop(true, 8, 16);
}

struct Rig {
    Session s;
    std::shared_ptr<Fonts> fonts = std::make_shared<Fonts>(makeBuiltinFontSource());
    Gui gui;
    DrawList dl;
    std::unique_ptr<Studio> st;
    double t = 1.0;
    float W = 1440, H = 900;
    Rig() {
        buildDemo(s);
        Host h;
        h.loopFolder = "../loops";
        st = std::make_unique<Studio>(s, h);
    }
    void wait(double sec) { t += sec; }
    void frame(int n = 2) { for (int i = 0; i < n; ++i) { dl.begin(W, H, 1.0f, fonts.get()); gui.beginFrame(dl, t); st->frame(gui, t); gui.endFrame(); t += 0.02; } }
    void shot(const char* path) { frame(1); softRender(dl, *fonts, theme::bg0).savePpm(path); }
    void move(float x, float y) { gui.onMouseMove(x, y); }
    void down(float x, float y, int b = 0) { gui.onMouseMove(x, y); gui.onMouseButton(b, true, t); frame(1); }
    void up(float x, float y, int b = 0) { gui.onMouseMove(x, y); gui.onMouseButton(b, false, t); frame(1); }
    void click(float x, float y, int b = 0) { move(x, y); frame(1); down(x, y, b); up(x, y, b); }
    void key(int k, int mods = 0) { gui.onKey(k, mods, true, false); frame(1); gui.onKey(k, mods, false, false); gui.setMods(0); }
    void type(const std::string& s) { gui.onText(s); frame(1); }
};

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    Rig r;
    r.frame(3);
    r.shot("/tmp/vk_main.ppm");
    Studio& st = *r.st;
    Session& s = r.s;
    auto laneY = [&](int ti) { return st.lanesRect().y + float(ti) * st.trackHeight() + st.trackHeight() * 0.5f; };
    auto bx = [&](double b) { return float(st.beatToX(b)); };
    CHECK(st.tracks().size() == 6);

    // ---- transport and tools
    r.click(120, 58); CHECK(s.isPlaying());
    r.click(120, 58); CHECK(!s.isPlaying());
    r.key(' '); CHECK(s.isPlaying()); r.key(' '); CHECK(!s.isPlaying());
    r.key('2'); CHECK(st.tool() == Tool::Pencil); r.key('3'); CHECK(st.tool() == Tool::Scissors); r.key('4'); CHECK(st.tool() == Tool::Eraser); r.key('5'); CHECK(st.tool() == Tool::Mute); r.key('1'); CHECK(st.tool() == Tool::Pointer);
    r.key('G'); CHECK(st.snapGrid() == 0.0); r.key('G'); CHECK(st.snapGrid() > 0);

    // ---- select, move, duplicate, undo
    r.click(bx(2), laneY(2)); CHECK(st.selectedTrack() == 2 && st.selectedRegion() == 0);
    r.click(bx(2), laneY(2) + 400);   // below the tracks: nothing under the mouse
    r.down(bx(6), laneY(4)); r.move(bx(7), laneY(4)); r.frame(1); r.move(bx(8), laneY(4)); r.frame(1); r.up(bx(8), laneY(4));
    CHECK(std::fabs(s.snapshot()[4].regions[0].start - 6.0) < 1e-9);
    r.key('Z', ModCtrl); CHECK(std::fabs(s.snapshot()[4].regions[0].start - 4.0) < 1e-9);
    r.wait(1.0);
    r.gui.setMods(ModAlt);
    r.down(bx(6), laneY(4)); r.move(bx(8), laneY(4)); r.frame(1); r.up(bx(8), laneY(4));
    r.gui.setMods(0);
    CHECK(s.snapshot()[4].regions.size() == 2);
    r.key('Z', ModCtrl); CHECK(s.snapshot()[4].regions.size() == 1);

    // ---- multi-selection: Shift+click, rubber band, Ctrl+A, group move across tracks, group delete / duplicate (one undo each)
    {
        r.key('1'); r.wait(1.0);
        r.click(bx(2), laneY(2)); r.wait(1.0);
        CHECK(st.selectionList().size() == 1);
        r.gui.setMods(ModShift); r.click(bx(13), laneY(2)); r.gui.setMods(0); r.wait(1.0);
        CHECK(st.selectionList().size() == 2 && st.regionSelected(2, 0) && st.regionSelected(2, 1));
        r.gui.setMods(ModShift); r.click(bx(13), laneY(2)); r.gui.setMods(0); r.wait(1.0);          // toggles it off again
        CHECK(st.selectionList().size() == 1);
        // rubber band from an empty spot (Keys lane, beat 10) down-left over the Keys, Drummer and Bass lanes: 3 regions
        r.click(bx(10), laneY(2)); r.wait(1.0);
        r.down(bx(10), laneY(2)); r.move(bx(6), laneY(3)); r.frame(1); r.move(bx(1), laneY(4)); r.frame(1);
        r.shot("/tmp/vk_rubber.ppm");
        r.up(bx(1), laneY(4)); r.wait(1.0);
        CHECK(st.selectionList().size() == 3);
        // Ctrl+A selects every region
        size_t total = 0; for (const auto& t : s.snapshot()) total += t.regions.size();
        r.click(bx(10), laneY(2)); r.wait(1.0);
        CHECK(st.selectionList().empty());
        r.key('A', ModCtrl); CHECK(st.selectionList().size() == total);
        // the two Keys regions move to the Bass track together (2 tracks down, 2 beats later); one undo brings them back
        r.click(bx(10), laneY(2)); r.wait(1.0);
        r.click(bx(2), laneY(2)); r.wait(1.0);
        r.gui.setMods(ModShift); r.click(bx(13), laneY(2)); r.gui.setMods(0); r.wait(1.0);
        r.down(bx(2), laneY(2)); r.move(bx(3), laneY(4)); r.frame(1); r.move(bx(4), laneY(4)); r.frame(1);
        r.shot("/tmp/vk_groupmove.ppm");
        r.up(bx(4), laneY(4)); r.wait(1.0);
        {
            const auto snap = s.snapshot();
            CHECK(snap[2].regions.empty() && snap[4].regions.size() == 3);
            std::vector<double> starts; for (const auto& rg : snap[4].regions) starts.push_back(rg.start);
            std::sort(starts.begin(), starts.end());
            CHECK(std::fabs(starts[0] - 2.0) < 1e-6 && std::fabs(starts[1] - 4.0) < 1e-6 && std::fabs(starts[2] - 14.0) < 1e-6);
        }
        r.key('Z', ModCtrl);
        CHECK(s.snapshot()[2].regions.size() == 2 && s.snapshot()[4].regions.size() == 1);
        // delete and Alt-drag duplicate of a group
        r.click(bx(10), laneY(2)); r.wait(1.0);
        r.click(bx(2), laneY(2)); r.wait(1.0);
        r.gui.setMods(ModShift); r.click(bx(13), laneY(2)); r.gui.setMods(0); r.wait(1.0);
        r.key(KeyDelete);
        CHECK(s.snapshot()[2].regions.empty());
        r.key('Z', ModCtrl);
        CHECK(s.snapshot()[2].regions.size() == 2);
        r.click(bx(10), laneY(2)); r.wait(1.0);
        r.click(bx(2), laneY(2)); r.wait(1.0);
        r.gui.setMods(ModShift); r.click(bx(13), laneY(2)); r.gui.setMods(0); r.wait(1.0);
        r.gui.setMods(ModAlt);
        r.down(bx(2), laneY(2)); r.move(bx(3), laneY(2)); r.frame(1); r.move(bx(4), laneY(2)); r.frame(1); r.up(bx(4), laneY(2));
        r.gui.setMods(0);
        CHECK(s.snapshot()[2].regions.size() == 4);
        r.key('Z', ModCtrl);
        CHECK(s.snapshot()[2].regions.size() == 2);
        // copy / paste, mute and ripple delete of a group
        r.click(bx(10), laneY(2)); r.wait(1.0);
        r.click(bx(2), laneY(2)); r.wait(1.0);
        r.gui.setMods(ModShift); r.click(bx(13), laneY(2)); r.gui.setMods(0); r.wait(1.0);
        r.key('C', ModCtrl);
        r.key('V', ModCtrl);
        CHECK(s.snapshot()[2].regions.size() == 4 && st.selectionList().size() == 2);
        r.shot("/tmp/vk_grouppaste.ppm");
        r.key('Z', ModCtrl);
        CHECK(s.snapshot()[2].regions.size() == 2);
        r.click(bx(10), laneY(2)); r.wait(1.0);
        r.click(bx(2), laneY(2)); r.wait(1.0);
        r.gui.setMods(ModShift); r.click(bx(13), laneY(2)); r.gui.setMods(0); r.wait(1.0);
        CHECK(st.runCommand("Mute Region"));
        CHECK(s.snapshot()[2].regions[0].mute && s.snapshot()[2].regions[1].mute);
        r.shot("/tmp/vk_groupmute.ppm");
        r.key('Z', ModCtrl);
        CHECK(!s.snapshot()[2].regions[0].mute && !s.snapshot()[2].regions[1].mute);
        r.key('A', ModCtrl);   // undo drops the multi-selection: select again (the other tracks have regions too)
        r.click(bx(10), laneY(2)); r.wait(1.0);
        r.click(bx(2), laneY(2)); r.wait(1.0);
        r.gui.setMods(ModShift); r.click(bx(13), laneY(2)); r.gui.setMods(0); r.wait(1.0);
        CHECK(st.runCommand("Ripple Delete Region"));
        CHECK(s.snapshot()[2].regions.empty());
        r.key('Z', ModCtrl);
        CHECK(s.snapshot()[2].regions.size() == 2);
        r.click(bx(6), laneY(4)); r.wait(1.0);   // leave the selection as the tests below expect it
    }

    // ---- tempo automation lane: add, move and ramp project tempo points
    {
        CHECK(st.runCommand("Show Automation"));
        st.setAutomationParam(4);
        r.frame(2);
        const Rect L = st.autoLaneRect();
        CHECK(L.w > 100 && s.tempoMap()->tempoPoints().size() == 1);
        const float x1 = L.x + L.w * 0.3f, yHigh = L.y + L.h * 0.25f, yLow = L.y + L.h * 0.75f;
        CHECK(st.runCommand("Snap to Grid"));   // snap off: the point lands exactly where it is clicked
        r.wait(1.0);
        r.click(x1, yHigh); r.wait(1.0);
        auto tp = s.tempoMap()->tempoPoints();
        CHECK(tp.size() == 2 && tp[1].beat > 0.5 && tp[1].bpm > 150.0);
        r.down(x1, yHigh); r.move(x1, (yHigh + yLow) / 2); r.frame(1); r.move(x1, yLow); r.frame(1); r.up(x1, yLow); r.wait(1.0);
        tp = s.tempoMap()->tempoPoints();
        CHECK(tp.size() == 2 && tp[1].bpm < 150.0);
        // double-click the first point: it ramps to the next
        const float y0 = L.bottom() - 6 - (L.h - 12) * float(tp[0].bpm - 20.0) / 280.f;
        r.click(L.x, y0); r.click(L.x, y0); r.wait(1.0);
        CHECK(s.tempoMap()->tempoPoints()[0].ramp);
        r.shot("/tmp/vk_tempolane.ppm");
        // bring the project back to one constant tempo with Clear Lane
        s.setTempoPoints({{0, 120, false}}, false);
        st.setAutomationParam(0);
        CHECK(st.runCommand("Snap to Grid"));
    }

    // ---- Drummer tab: change the groove of a Drummer region
    {
        const int dtrack = 3;
        r.click(bx(2), laneY(dtrack)); r.wait(1.0);
        CHECK(st.selectedTrack() == dtrack && st.selectedRegion() >= 0 && s.snapshot()[size_t(dtrack)].regions[size_t(st.selectedRegion())].drummer);
        CHECK(st.runCommand("Show Drummer"));
        r.frame(2);
        const int ri = st.selectedRegion();
        const auto before = s.snapshot()[size_t(dtrack)].regions[size_t(ri)];
        const Rect D = st.dockRect();
        const float rowY = D.y + 30 + 12 + 32 * 6 + 11;            // the Seed row (below the 30 px tab bar)
        r.click(D.x + 108 + 78 + 30, rowY); r.wait(1.0);            // Roll New Variation
        auto after = s.snapshot()[size_t(dtrack)].regions[size_t(ri)];
        CHECK(after.drum.seed == before.drum.seed % 9999 + 1);
        r.shot("/tmp/vk_drummer.ppm");
        r.click(D.x + 108 + 20, D.y + 30 + 12 + 32 * 4 + 11); r.wait(1.0);   // Fills checkbox
        after = s.snapshot()[size_t(dtrack)].regions[size_t(ri)];
        CHECK(after.drum.fills != before.drum.fills);
        r.key('Z', ModCtrl); r.key('Z', ModCtrl);
        after = s.snapshot()[size_t(dtrack)].regions[size_t(ri)];
        CHECK(after.drum.seed == before.drum.seed && after.drum.fills == before.drum.fills);
        CHECK(st.runCommand("Show Mixer"));
    }

    // ---- Help > Tutorial: 15 pages, keys and buttons, "Try it"
    {
        CHECK(Studio::tutorialPageCount() == 15);
        CHECK(st.runCommand("Tutorial..."));
        r.frame(2);
        CHECK(st.tutorialOpen() && st.tutorialPage() == 0);
        r.shot("/tmp/vk_tutorial1.ppm");
        for (int i = 1; i < 15; ++i) { r.key(KeyRight); r.frame(1); CHECK(st.tutorialPage() == i); if (i == 8) r.shot("/tmp/vk_tutorial9.ppm"); }
        r.key(KeyRight); CHECK(st.tutorialPage() == 14);   // stays on the last page
        r.shot("/tmp/vk_tutorial15.ppm");
        r.key(KeyLeft); CHECK(st.tutorialPage() == 13);
        // the buttons: Back / Next are at the bottom of the centred panel
        const float pw = std::min(680.f, r.W - 40), ph = std::min(470.f, r.H - 40);
        const float px = (r.W - pw) / 2, py = (r.H - ph) / 2;
        const float btnY = py + ph - 44 + 15;
        r.click(px + pw - 24 - 55, btnY); r.wait(1.0);
        CHECK(st.tutorialPage() == 14);
        r.click(px + 24 + 48, btnY); r.wait(1.0);
        CHECK(st.tutorialPage() == 13);
        r.key(KeyEscape); r.frame(1);
        CHECK(!st.tutorialOpen());
        // "Try it" on the Drummer page closes the tutorial and opens the Drummer tab
        st.openTutorial(5); r.frame(2);
        CHECK(st.tutorialOpen() && st.tutorialPage() == 5);
        r.click(px + pw - 24 - 110 - 8 - 55, btnY); r.wait(1.0);
        CHECK(!st.tutorialOpen());
        CHECK(st.runCommand("Show Mixer"));
        // the menu bar has it as the first item of the Help menu
        r.frame(1);
    }

    // ---- playback does not copy the whole model every frame: only the meters are refreshed
    {
        r.frame(2);
        const int copies = st.fullSnapshotCount();
        s.seekBeats(0);
        s.play();
        std::vector<std::int16_t> pcm(2 * 2048);
        float seen = 0;
        for (int i = 0; i < 40; ++i) {
            s.fillInt16(pcm.data(), 2048);
            r.frame(1);
            for (const auto& t : st.tracks()) seen = std::max(seen, std::max(t.peakL, t.peakR));
        }
        CHECK(st.fullSnapshotCount() == copies);      // 40 frames of playback, no whole-model copy
        CHECK(seen > 0.001f);                         // the meters still move
        s.stop();
        for (int i = 0; i < 140; ++i) s.fillInt16(pcm.data(), 2048);   // the audio thread runs on: reverb tails ring out (about 4 s), then the meters are cleared
        r.frame(2);
        float after = 0;
        for (const auto& t : st.tracks()) after = std::max(after, std::max(t.peakL, t.peakR));
        CHECK(after == 0.f);                          // and they are cleared after the stop
        // an edit during playback is picked up at once
        s.play(); r.frame(1);
        s.setVolume(0, 0.5f); r.frame(2);
        CHECK(st.fullSnapshotCount() > copies && std::fabs(st.tracks()[0].volume - 0.5f) < 1e-4);
        s.stop(); r.frame(2);
    }

    // ---- scissors, eraser, mute tool, pencil
    r.key('3'); r.click(bx(6.1), laneY(0));
    CHECK(s.snapshot()[0].regions.size() == 2 && std::fabs(s.snapshot()[0].regions[1].start - 6.0) < 1e-6);
    r.key('Z', ModCtrl); CHECK(s.snapshot()[0].regions.size() == 1);
    r.key('4'); r.click(bx(2), laneY(2)); CHECK(s.snapshot()[2].regions.size() == 1);
    r.key('Z', ModCtrl); CHECK(s.snapshot()[2].regions.size() == 2);
    r.key('5'); r.click(bx(6), laneY(1)); CHECK(s.snapshot()[1].regions[0].mute);
    r.key('Z', ModCtrl); CHECK(!s.snapshot()[1].regions[0].mute);
    r.key('2');
    r.down(bx(0) + 3, laneY(4)); r.move(bx(2), laneY(4)); r.frame(1); r.move(bx(3), laneY(4)); r.frame(1); r.up(bx(3), laneY(4));
    CHECK(s.snapshot()[4].regions.size() == 2 && std::fabs(s.snapshot()[4].regions[1].start) < 1e-9 && std::fabs(s.regionBeats(s.snapshot()[4].regions[1]) - 3.0) < 1e-9);
    r.key('Z', ModCtrl); r.key('1');

    // ---- Pro Tools: Tab to Transient
    r.click(bx(6), laneY(1)); CHECK(st.selectedTrack() == 1);
    s.seekBeats(0);
    r.key(KeyTab); CHECK(std::fabs(s.positionBeats() - 4.0) < 0.1);
    r.key(KeyTab); CHECK(s.positionBeats() > 4.5 && s.positionBeats() < 5.4);
    r.key(KeyTab, ModShift); CHECK(std::fabs(s.positionBeats() - 4.0) < 0.1);

    // ---- command palette: Riff Machine on the Keys region (FL Studio)
    r.click(bx(2), laneY(2));
    r.key('K', ModCtrl); CHECK(st.paletteOpen());
    r.type("riff mach"); r.key(KeyEnter);
    CHECK(!st.paletteOpen() && st.dialogOpen() && st.dialogTitle() == "Riff Machine");
    r.shot("/tmp/vk_riff_dialog.ppm");
    st.acceptDialog();
    {
        const auto notes = s.snapshot()[2].regions[0].notes;
        CHECK(!notes.empty());
        bool ok = true;
        for (const Note& n : notes) ok = ok && inScale(n.pitch, 0, ScaleParams::Major);
        CHECK(ok);
    }
    r.key('Z', ModCtrl);
    // ---- Cubase logical editor and Logic Pro groove through the registry
    CHECK(st.runCommand("Make Groove Template") && s.hasGrooveTemplate());
    CHECK(st.runCommand("Logical Editor...") && st.dialogOpen()); r.frame(1);
    st.acceptDialog(); r.frame(1);
    // ---- Ableton Live: Capture MIDI
    {
        const int id = s.snapshot()[2].id;
        s.injectCapture(id, 60, 100, 100.0, 100.5); s.injectCapture(id, 64, 90, 101.0, 101.5);
        const size_t before = s.snapshot()[2].regions.size();
        CHECK(st.runCommand("Capture MIDI") && s.snapshot()[2].regions.size() == before + 1);
    }
    // ---- Cubase snapshots
    CHECK(s.mixSnapshots().size() == 2);
    r.key(KeyEnter, ModCtrl | ModShift); CHECK(s.mixSnapshots().size() == 3);
    s.setVolume(2, 0.1f);
    r.key('1', ModAlt); CHECK(std::fabs(s.snapshot()[2].volume - 0.62f) < 1e-6);

    // ---- piano roll: fold, scale highlight, chord stamp
    r.click(bx(2), laneY(2));
    CHECK(st.runCommand("Show Editor")); r.frame(2);
    r.shot("/tmp/vk_pianoroll.ppm");
    CHECK(st.runCommand("Chord Stamp: Major")); r.frame(1);
    {
        const Rect d = st.dockRect();
        const size_t before = s.snapshot()[2].regions[st.selectedRegion()].notes.size();
        r.click(d.x + 66 + 400, d.y + 30 + 58 + 24 + 40);
        CHECK(s.snapshot()[2].regions[size_t(st.selectedRegion())].notes.size() == before + 3);
    }
    st.runCommand("Chord Stamp: Off");
    CHECK(st.runCommand("Fold Notes")); r.frame(2);
    r.shot("/tmp/vk_fold.ppm");
    st.runCommand("Fold Notes");

    // ---- piano roll in the style of Logic Pro / Ableton Live
    {
        const Rect d = st.dockRect();
        Region rgn = s.snapshot()[2].regions[size_t(st.selectedRegion())];
        const size_t before = rgn.notes.size();
        // B toggles draw mode while the mouse is over the roll; a click then writes one note
        r.move(d.x + 66 + 300, d.y + 30 + 58 + 24 + 50); r.frame(2);
        r.key('B'); CHECK(st.tool() == Tool::Pencil);
        r.click(d.x + 66 + 300, d.y + 30 + 58 + 24 + 50);
        CHECK(s.snapshot()[2].regions[size_t(st.selectedRegion())].notes.size() == before + 1);
        r.key('B'); CHECK(st.tool() == Tool::Pointer);
        // delete the new note with Delete (it is selected), then undo brings it back
        r.move(d.x + 66 + 300, d.y + 30 + 58 + 24 + 50); r.frame(2);
        r.key(KeyDelete); CHECK(s.snapshot()[2].regions[size_t(st.selectedRegion())].notes.size() == before);
        r.key('Z', ModCtrl); CHECK(s.snapshot()[2].regions[size_t(st.selectedRegion())].notes.size() == before + 1);
        r.shot("/tmp/vk_pianoroll2.ppm");
    }
    // ---- Ableton Live transform tools, Pro Tools ripple delete, workspaces
    {
        const int sr = st.selectedRegion();
        const size_t n0 = s.snapshot()[2].regions[size_t(sr)].notes.size();
        const double len0 = s.regionBeats(s.snapshot()[2].regions[size_t(sr)]);
        CHECK(st.runCommand("Double Pattern"));
        CHECK(s.snapshot()[2].regions[size_t(sr)].notes.size() == n0 * 2 && std::fabs(s.regionBeats(s.snapshot()[2].regions[size_t(sr)]) - len0 * 2) < 1e-9);
        r.key('Z', ModCtrl);
        CHECK(s.snapshot()[2].regions[size_t(sr)].notes.size() == n0);
        const double firstStart = s.snapshot()[2].regions[size_t(sr)].notes.empty() ? 0 : s.snapshot()[2].regions[size_t(sr)].notes.back().start;
        CHECK(st.runCommand("Stretch Notes x2"));
        CHECK(s.snapshot()[2].regions[size_t(sr)].notes.back().start >= firstStart);
        r.key('Z', ModCtrl);
        // ripple delete: select the first region of the Keys track, the one behind it moves up
        const auto regs = s.snapshot()[2].regions;
        double second = 1e9; for (const Region& g : regs) if (g.start > 11.5 && g.start < second) second = g.start;
        r.click(bx(2), laneY(2));
        const double dlen = s.regionBeats(s.snapshot()[2].regions[size_t(st.selectedRegion())]);
        CHECK(st.runCommand("Ripple Delete Region"));
        double second2 = 1e9; for (const Region& g : s.snapshot()[2].regions) if (g.start > 3.0 && g.start < second2) second2 = g.start;
        CHECK(std::fabs(second2 - (second - dlen)) < 1e-6 || second == 1e9);
        r.key('Z', ModCtrl);
        st.runCommand("Workspace: Mix"); r.frame(2); CHECK(!st.browserVisible() && !st.inspectorVisible() && st.dockTab() == DockTab::Mixer);
        r.shot("/tmp/vk_mixspace.ppm");
        st.runCommand("Workspace: Edit"); r.frame(2); CHECK(st.dockTab() == DockTab::Editor && st.dockVisible());
        r.key(KeyF5); CHECK(st.browserVisible() && st.inspectorVisible());
        r.key('X', ModShift); r.frame(2); r.key('X', ModShift);
        st.runCommand("Show Snapshots"); r.frame(2); r.shot("/tmp/vk_snapshots.ppm");
        st.runCommand("Show Devices"); r.frame(2); r.shot("/tmp/vk_devices.ppm");
        r.key(KeyF1); CHECK(st.helpOpen()); r.frame(2); r.shot("/tmp/vk_help.ppm"); r.key(KeyF1); CHECK(!st.helpOpen());
        r.key('K', ModCtrl); r.type("snap"); r.frame(2); r.shot("/tmp/vk_palette.ppm"); r.key(KeyEscape); CHECK(!st.paletteOpen());
        st.runCommand("Workspace: Arrange"); r.frame(2);
    }
    // ---- step sequencer
    st.runCommand("Track Height: Small"); r.frame(2); CHECK(st.trackHeight() == 40.f);
    r.click(bx(0), laneY(5) + 5);
    CHECK(st.selectedTrack() == 5);
    st.runCommand("Show Step Sequencer"); r.frame(2);
    {
        const Rect d = st.dockRect();
        const float cw = (d.w - 90 - 32) / 16.f, ch = std::min(52.f, (d.h - 30 - 50) / 4.f);
        const float cx = d.x + 16 + 90 + cw * 1.5f, cy = d.y + 30 + 30 + ch * 0.5f;
        CHECK(!s.drumPattern(5)[0][1]);
        r.click(cx, cy); CHECK(s.drumPattern(5)[0][1]);
    }

    st.runCommand("Track Height: Medium"); r.frame(2); CHECK(st.trackHeight() == 72.f);
    // ---- five-DAW interface: overview strip, browser tabs + clip list, mixer filter / narrow strips / peak hold
    {
        st.runCommand("Workspace: Arrange"); r.frame(2);
        const Rect ov = st.overviewRect();
        CHECK(ov.h > 10 && ov.y < st.lanesRect().y);
        st.runCommand("Zoom In"); st.runCommand("Zoom In"); st.runCommand("Zoom In"); r.frame(2);
        const double sc0 = st.scrollBeats();
        r.down(ov.right() - 40, ov.y + ov.h * 0.5f); r.up(ov.right() - 40, ov.y + ov.h * 0.5f);
        CHECK(st.scrollBeats() > sc0 + 1.0);   // clicking the right end of the overview scrolls the arrangement there
        r.click(ov.x + st.lanesRect().x - ov.x + 8, ov.y + ov.h * 0.5f);
        CHECK(st.scrollBeats() < 5.0);
        r.click(bx(2), laneY(2));
        CHECK(st.selectionText().find("Start") == 0 && st.selectionText().find("Len") != std::string::npos);
        // browser: tabs filter by kind, the Project tab lists regions and selects them
        const Rect br = st.browserRect();
        CHECK(st.browserTab() == 0);
        r.click(br.x + 12 + 70, br.y + 74);   // second chip: Loops
        CHECK(st.browserTab() == 1);
        st.runCommand("Browser: Project Clips"); r.frame(2); CHECK(st.browserTab() == 4);
        r.shot("/tmp/vk_five_arrange.ppm");
        s.seekBeats(0);
        r.click(br.x + 60, br.y + 92 + 26 + 36);   // second row of the first track: nothing, the first track has one region -> second track header is skipped
        r.click(br.x + 60, br.y + 92 + 26 + 11);   // first region of the first track
        CHECK(st.selectedTrack() == 0 && st.selectedRegion() == 0);
        st.runCommand("Show Mixer"); r.frame(2);
        const Rect dk = st.dockRect();
        r.click(dk.x + 8 + 40, dk.y + 30 + 12);   // "Audio" filter
        CHECK(st.mixerFilter() == 1);
        r.shot("/tmp/vk_five_mixer_filter.ppm");
        r.click(dk.x + 8 + 10, dk.y + 30 + 12);   // back to "All"
        CHECK(st.mixerFilter() == 0);
        CHECK(st.runCommand("Mixer: Narrow Strips") && st.mixerNarrow()); r.frame(2);
        r.shot("/tmp/vk_five_mixer_narrow.ppm");
        st.runCommand("Mixer: Narrow Strips"); CHECK(!st.mixerNarrow());
        st.runCommand("Browser: Project Clips"); r.click(br.x + 12 + 14, br.y + 74); CHECK(st.browserTab() == 0);
        for (int i = 0; i < 3; ++i) st.runCommand("Zoom Out");
        r.frame(2);
    }
    // ---- dialogs and the unsaved-changes guard
    r.click(bx(2), laneY(2));
    CHECK(st.runCommand("Quantize...") && st.dialogOpen()); r.key(KeyEscape); CHECK(!st.dialogOpen());
    s.setVolume(0, 0.3f);
    r.key('N', ModCtrl); CHECK(st.dialogOpen() && st.dialogTitle() == "Unsaved changes");
    r.key(KeyEscape); CHECK(!st.dialogOpen() && s.trackCount() == 6);
    // ---- zoom
    const double ppb0 = st.ppb();
    r.gui.setMods(ModCtrl); r.move(bx(8), laneY(1)); r.frame(1); r.gui.onWheel(0, 1); r.frame(1); r.gui.setMods(0);
    CHECK(st.ppb() > ppb0);
    r.key('Z'); r.frame(1); CHECK(st.ppb() != ppb0);

    // ---- every shortcut is unique unless the command is only enabled in a mode
    {
        std::map<std::pair<int, int>, std::vector<std::string>> keys;
        for (const Command& c : st.commands()) if (c.key && c.run && !c.enabled) keys[{c.key, c.mods}].push_back(c.label);
        for (const auto& kv : keys) if (kv.second.size() > 1) { std::printf("shortcut clash:"); for (const auto& l : kv.second) std::printf(" [%s]", l.c_str()); std::printf("\n"); ++fails; }
    }
    // ---- every command runs without crashing and the frame after it draws
    {
        int ran = 0;
        for (const Command& c : st.commands()) {
            if (c.label == "-" || c.category == "File" || c.label == "Quantize..." ) continue;
            if (c.enabled && !c.enabled()) continue;
            c.run();
            r.frame(1);
            for (int i = 0; i < 3; ++i) { r.key(KeyEscape); }
            ++ran;
        }
        std::printf("smoke-ran %d commands of %zu\n", ran, st.commands().size());
        CHECK(ran > 60);
    }
    // ---- a bigger session still draws fast enough: count vertices
    std::printf("tracks %zu verts %zu\n", st.tracks().size(), r.dl.vertices().size());
    return fails ? 1 : 0;
}
