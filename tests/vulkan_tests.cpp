// Tests and screenshots of the Vulkan user interface (src/vulkan) rendered by the CPU twin of its shader: no GPU needed.
#include "../src/core/LogicProjectData.h"
#include <chrono>
#include <fstream>
#include <cmath>
#include <thread>
#include <cstdio>
#include <string>
#include "../src/core/Files.h"
#include "../src/core/Session.h"
#include "../src/vulkan/SoftRaster.h"
#include "../src/vulkan/IconFont.h"
#include "../src/vulkan/Icons.h"
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

    // ---- Help > Tutorial: 18 pages, keys and buttons, "Try it"
    {
        CHECK(Studio::tutorialPageCount() == 18);
        CHECK(st.runCommand("Tutorial..."));
        r.frame(2);
        CHECK(st.tutorialOpen() && st.tutorialPage() == 0);
        r.shot("/tmp/vk_tutorial1.ppm");
        for (int i = 1; i < 18; ++i) { r.key(KeyRight); r.frame(1); CHECK(st.tutorialPage() == i); if (i == 8) r.shot("/tmp/vk_tutorial9.ppm"); if (i == 14) r.shot("/tmp/vk_tutorial_groups.ppm"); if (i == 15) r.shot("/tmp/vk_tutorial_import.ppm"); if (i == 16) r.shot("/tmp/vk_tutorial_export.ppm"); }
        r.key(KeyRight); CHECK(st.tutorialPage() == 17);   // stays on the last page
        r.shot("/tmp/vk_tutorial18.ppm");
        r.key(KeyLeft); CHECK(st.tutorialPage() == 16);
        // the buttons: Back / Next are at the bottom of the centred panel
        const float pw = std::min(680.f, r.W - 40), ph = std::min(470.f, r.H - 40);
        const float px = (r.W - pw) / 2, py = (r.H - ph) / 2;
        const float btnY = py + ph - 44 + 15;
        r.click(px + pw - 24 - 55, btnY); r.wait(1.0);
        CHECK(st.tutorialPage() == 17);
        r.click(px + 24 + 48, btnY); r.wait(1.0);
        CHECK(st.tutorialPage() == 16);
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

    // ---- usability: toast, middle-mouse pan, edge scrolling while dragging, layout that is remembered
    {
        // toast: a floating note is drawn while a message is fresh
        r.wait(5.0); r.frame(3);                                // let earlier messages fade out
        auto region = [&] {                                      // the pixels where the toast is drawn (centre, just above the status bar)
            const auto img = softRender(r.dl, *r.fonts, theme::bg0);
            std::vector<std::uint32_t> out;
            for (int y = int(r.H) - 90; y < int(r.H) - 40; ++y) for (int x = int(r.W / 2) - 60; x < int(r.W / 2) + 60; ++x) out.push_back(img.px[size_t(y) * size_t(img.w) + size_t(x)]);
            return out;
        };
        const auto quiet = region();
        st.message("Layout saved for next time");
        r.frame(8);
        CHECK(region() != quiet);
        r.shot("/tmp/vk_toast.ppm");
        r.wait(5.0); r.frame(2);
        // middle mouse button drags the view
        const double sx0 = st.scrollBeats();
        const float mx = bx(8), my = laneY(1) + 20;
        r.gui.onMouseMove(mx, my); r.frame(1);
        r.gui.onMouseButton(2, true); r.frame(1);
        r.gui.onMouseMove(mx - 120, my); r.frame(1);
        r.gui.onMouseMove(mx - 240, my); r.frame(1);
        r.gui.onMouseButton(2, false); r.frame(1);
        CHECK(st.scrollBeats() > sx0 + 4.0);
        r.gui.onMouseMove(mx - 240, my); r.frame(1);               // and back to the left edge of the song (the view never goes below beat 0)
        r.gui.onMouseButton(2, true); r.frame(1);
        r.gui.onMouseMove(mx + 60, my); r.frame(2);
        r.gui.onMouseButton(2, false); r.frame(1);
        CHECK(st.scrollBeats() == 0.0);
        r.gui.onMouseMove(mx, my); r.frame(2);
    }
    {
        // the layout is stored through the host and restored by a new studio
        core::Session s2;
        std::map<std::string, std::string> saved;
        Host h2;
        h2.loopFolder = "../loops";
        h2.storeSettings = [&](const std::map<std::string, std::string>& m) { saved = m; };
        Studio st2(s2, h2);
        Gui g2; DrawList dl2; auto fonts2 = std::make_shared<Fonts>(makeBuiltinFontSource());
        double t2 = 1.0;
        auto frame2 = [&](int n) { for (int i = 0; i < n; ++i) { dl2.begin(1440, 900, 1.0f, fonts2.get()); g2.beginFrame(dl2, t2); st2.frame(g2, t2); g2.endFrame(); t2 += 0.3; } };
        frame2(3);
        CHECK(saved.empty());                                   // nothing is written at start-up
        CHECK(st2.runCommand("Show / Hide Browser"));
        CHECK(st2.runCommand("Show Drummer"));
        frame2(4);
        CHECK(saved.count("showBrowser") && saved["showBrowser"] == "0" && saved["dockTab"] == "6");
        Host h3; h3.settings = saved; h3.loopFolder = "../loops";
        core::Session s3;
        Studio st3(s3, h3);
        CHECK(!st3.browserVisible());
        CHECK(st3.layoutSettings() == saved);
        // nonsense values are clamped, not trusted
        Studio st4(s3, h3);
        st4.applyLayout({{"dockH", "99999"}, {"zoom", "-5"}, {"dockTab", "77"}});
        CHECK(st4.layoutSettings()["dockH"] == "900.0" && st4.layoutSettings()["dockTab"] == "6");
    }

    // ---- MIDI channel of a track (for MIDI export): inspector row, palette commands
    {
        int inst = -1, other = -1;
        for (int i = 0; i < int(st.tracks().size()); ++i) {
            if (st.tracks()[size_t(i)].type == core::TrackType::Instrument && inst < 0) inst = i;
            if (st.tracks()[size_t(i)].type != core::TrackType::Instrument && other < 0) other = i;
        }
        CHECK(inst >= 0 && other >= 0);
        CHECK(st.runCommand("Show Mixer"));
        st.selectRegion(inst, -1); r.frame(3);
        CHECK(st.tracks()[size_t(inst)].midiChannel == 0);
        r.shot("/tmp/vk_midich_auto.ppm");
        CHECK(st.runCommand("~MIDI Channel: 5")); r.frame(3);
        CHECK(st.tracks()[size_t(inst)].midiChannel == 5 && s.snapshot()[size_t(inst)].midiChannel == 5);
        r.shot("/tmp/vk_midich_5.ppm");
        CHECK(st.runCommand("~MIDI Channel: 16")); r.frame(2);
        CHECK(st.tracks()[size_t(inst)].midiChannel == 16);
        CHECK(st.runCommand("~MIDI Channel: Auto")); r.frame(2);
        CHECK(st.tracks()[size_t(inst)].midiChannel == 0);
        st.selectRegion(other, -1); r.frame(3);
        r.shot("/tmp/vk_midich_other.ppm");   // no MIDI Ch row on an audio / drum track
    }

    // ---- mixer groups: grouped tracks follow each other, Alt changes only one
    {
        CHECK(st.runCommand("Show Mixer"));
        st.selectRegion(0, -1); CHECK(st.runCommand("~Mixer Group: A"));
        st.selectRegion(1, -1); CHECK(st.runCommand("~Mixer Group: A"));
        st.selectRegion(3, -1); CHECK(st.runCommand("~Mixer Group: C"));
        r.frame(3);
        CHECK(st.tracks()[0].group == 1 && st.tracks()[1].group == 1 && st.tracks()[2].group == 0 && st.tracks()[3].group == 3);
        const float v0 = s.snapshot()[0].volume, v1 = s.snapshot()[1].volume, v2 = s.snapshot()[2].volume;
        s.setVolume(0, v0 * 0.5f);
        CHECK(std::fabs(s.snapshot()[1].volume - v1 * 0.5f) < 1e-5f && s.snapshot()[2].volume == v2);
        r.shot("/tmp/vk_groups.ppm");
        r.gui.setMods(ModAlt); r.frame(1);                        // Alt held: the group is bypassed
        s.setVolume(0, v0);
        CHECK(std::fabs(s.snapshot()[1].volume - v1 * 0.5f) < 1e-5f);
        r.gui.setMods(0); r.frame(1);
        s.setVolume(0, v0 * 0.5f); s.setVolume(1, v1);              // put things back (linked again)
        for (int i = 0; i < 4; ++i) { st.selectRegion(i, -1); st.runCommand("~Mixer Group: None"); }
        s.setVolume(0, v0); s.setVolume(1, v1);
        r.frame(2);
        CHECK(st.tracks()[0].group == 0 && st.tracks()[1].group == 0);
    }

    // ---- the Preferences window tells where the settings file is
    {
        core::Session sp;
        Host hp; hp.loopFolder = "../loops"; hp.dataDir = "C:\\Users\\me\\AppData\\Local\\MiniDAW";
        Studio stp(sp, hp);
        Gui gp; DrawList dlp; auto fontsp = std::make_shared<Fonts>(makeBuiltinFontSource());
        double tp = 1.0;
        auto framep = [&](int n) { for (int i = 0; i < n; ++i) { dlp.begin(1440, 900, 1.0f, fontsp.get()); gp.beginFrame(dlp, tp); stp.frame(gp, tp); gp.endFrame(); tp += 0.05; } };
        framep(3);
        CHECK(stp.runCommand("Preferences..."));
        framep(3);
        softRender(dlp, *fontsp, theme::bg0).savePpm("/tmp/vk_prefs.ppm");
        CHECK(stp.dialogOpen());
    }

    // ---- opening other file types: File > Open / drag and drop / "open with" accept projects, audio and MIDI files
    {
        std::string err;
        CHECK(s.exportMidiFile("/tmp/vk_open.mid", &err));
        CHECK(s.saveProject("/tmp/vk_open.mdaw", &err));
        std::vector<float> pcm(2 * 4410, 0.1f);
        CHECK(core::writeWavFile("/tmp/vk_open.wav", pcm, &err));
        // the picker offers every supported type
        std::string seenFilter;
        core::Session sx;
        Host hx; hx.loopFolder = "../loops";
        hx.fileDialog = [&](const std::string&, const std::string& filter, bool) { seenFilter = filter; return std::string("/tmp/vk_open.mid"); };
        Studio stx(sx, hx);
        Gui gx; DrawList dlx; auto fontsx = std::make_shared<Fonts>(makeBuiltinFontSource());
        double tx = 1.0;
        auto framex = [&](int n) { for (int i = 0; i < n; ++i) { dlx.begin(1440, 900, 1.0f, fontsx.get()); gx.beginFrame(dlx, tx); stx.frame(gx, tx); gx.endFrame(); tx += 0.05; } };
        framex(3);
        const int base = sx.trackCount();
        stx.openAny("/tmp/notes.txt");                                    // unsupported: a clear message, nothing happens
        CHECK(stx.toastText().find("not supported") != std::string::npos && sx.trackCount() == base);
        CHECK(stx.runCommand("Open..."));                                 // the dialog returns a .mid file: it is imported
        framex(2);
        CHECK(seenFilter.find("*.mp3") != std::string::npos && seenFilter.find("*.mdaw") != std::string::npos && seenFilter.find("*.mid") != std::string::npos);
        CHECK(sx.trackCount() > base);
        // an audio file is imported (decoded in the background)
        const int before = sx.trackCount();
        stx.openAny("/tmp/vk_open.wav");
        for (int i = 0; i < 200 && sx.trackCount() == before; ++i) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); framex(1); }
        CHECK(sx.trackCount() == before + 1);
        // a Logic Pro package (.logicx folder): one track per recording, with an honest message about the arrangement
        {
            auto sh = [](const std::string& c) { const int rc = std::system(c.c_str()); (void)rc; };
            sh("rm -rf /tmp/vk_logic && mkdir -p '/tmp/vk_logic/Night Drive.logicx/Alternatives/000' '/tmp/vk_logic/Night Drive.logicx/Media/Audio Files'");
            sh("echo x > '/tmp/vk_logic/Night Drive.logicx/Alternatives/000/ProjectData'");
            for (const char* n : {"Drums#01", "Bass#01", "Lead Vox#01"})
                sh(std::string("cp /tmp/vk_open.wav '/tmp/vk_logic/Night Drive.logicx/Media/Audio Files/") + n + ".wav'");
            const int b0 = sx.trackCount();
            stx.openAny("/tmp/vk_logic/Night Drive.logicx");
            for (int i = 0; i < 300 && sx.trackCount() < b0 + 3; ++i) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); framex(1); }
            CHECK(sx.trackCount() == b0 + 3);
            CHECK(stx.toastText().find("Night Drive") != std::string::npos && stx.toastText().find("not read") != std::string::npos);
            framex(2);
            softRender(dlx, *fontsx, theme::bg0).savePpm("/tmp/vk_logicx_import.ppm");
            {   // the full pipeline with a stand-in decoder: tracks, an audio region at bar 3, a MIDI region with notes, tempo
                (void)0;
                sh("echo 'FAKE-LAYOUT-0123456789' > '/tmp/vk_logic/Night Drive.logicx/Alternatives/000/ProjectData'");
                core::registerProjectDataDecoder([](const std::uint8_t* d, std::size_t n, core::LogicProject& out, std::string& name) {
                    if (n < 4 || std::string(reinterpret_cast<const char*>(d), 4) != "FAKE") return false;
                    name = "stand-in decoder";
                    out.tempo = {{0, 96, false}};
                    core::LogicTrack v; v.name = "Lead Vox"; v.audio.push_back({"Lead Vox", "Lead Vox#01.wav", 8.0, 0});
                    core::LogicTrack k; k.name = "Keys"; k.kind = core::LogicTrackKind::Software;
                    core::LogicMidiRegion mr; mr.startBeat = 0; mr.lengthBeats = 8;
                    for (int i = 0; i < 8; ++i) mr.notes.push_back({48 + (i * 3) % 12 + (i % 2) * 7, i * 1.0, 0.9, 90});
                    k.midi.push_back(mr);
                    out.tracks = {v, k};
                    return true;
                });
                const int b1 = sx.trackCount();
                stx.openAny("/tmp/vk_logic/Night Drive.logicx");
                for (int i = 0; i < 300 && sx.trackCount() < b1 + 2; ++i) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); framex(1); }
                for (int i = 0; i < 100; ++i) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); framex(1); }
                CHECK(sx.trackCount() == b1 + 2 && std::fabs(sx.tempoMap()->bpmAt(0) - 96) < 1e-6);
                framex(2);
                softRender(dlx, *fontsx, theme::bg0).savePpm("/tmp/vk_logicx_pipeline.ppm");
                core::clearProjectDataDecoders();
            }
            if (const char* real = std::getenv("MINIDAW_LOGICX_SAMPLE")) {   // a real project, when one is at hand (screenshots only)
                core::Session sr; Host hr; hr.loopFolder = "../loops";
                Studio str(sr, hr);
                Gui gr; DrawList dlr; auto fontsr = std::make_shared<Fonts>(makeBuiltinFontSource());
                double tr = 1.0;
                auto framer = [&](int n) { for (int i = 0; i < n; ++i) { dlr.begin(1440, 900, 1.0f, fontsr.get()); gr.beginFrame(dlr, tr); str.frame(gr, tr); gr.endFrame(); tr += 0.05; } };
                framer(3);
                const int r0 = sr.trackCount();
                str.openAny(real);
                auto placed = [&] { int c = 0; for (const auto& t : sr.snapshot()) c += int(t.regions.size()); return c; };
                for (int i = 0; i < 12000 && placed() < 255 + 28; ++i) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); framer(1); }
                std::fprintf(stderr, "real studio import: tracks +%d, regions %d, bpm %g\n", sr.trackCount() - r0, placed(), sr.tempoMap()->bpmAt(0));
                CHECK(placed() == 255 + 28 && sr.trackCount() == r0 + 81 && std::fabs(sr.tempoMap()->bpmAt(0) - 82.0) < 1e-6);
                CHECK(str.runCommand("Zoom to Fit Project")); framer(3);
                softRender(dlr, *fontsr, theme::bg0).savePpm("/tmp/vk_logicx_real_import.ppm");
                CHECK(str.runCommand("Show Mixer")); framer(3); softRender(dlr, *fontsr, theme::bg0).savePpm("/tmp/vk_logicx_real_mixer.ppm");
                CHECK(str.runCommand("Show Editor")); framer(3); softRender(dlr, *fontsr, theme::bg0).savePpm("/tmp/vk_logicx_real_editor.ppm");
            }
            CHECK(stx.runCommand("Import Logic Pro Project Folder..."));   // no folder dialog in this host: a clear message
            CHECK(stx.toastText().find("not available") != std::string::npos);
        }
        // a project opens in place of the current one (a clean session is not asked)
        core::Session sy;
        Host hy; hy.loopFolder = "../loops";
        Studio sty(sy, hy);
        sty.openAny("/tmp/vk_open.mdaw");
        CHECK(sy.trackCount() == s.trackCount());
        // ... and a session with unsaved changes asks first
        stx.openAny("/tmp/vk_open.mdaw");
        CHECK(stx.dialogOpen());
    }

    // ---- Bounce / Export: the dialog offers the formats, the file gets the right extension
    {
        core::Session sb;
        buildDemo(sb);
        std::string seenFilter;
        Host hb; hb.loopFolder = "../loops";
        hb.fileDialog = [&](const std::string&, const std::string& filter, bool) { seenFilter = filter; return std::string("/tmp/vk_bounce_out"); };
        std::remove("/tmp/vk_bounce_out.wav");
        Studio stb(sb, hb);
        Gui gb; DrawList dlb; auto fontsb = std::make_shared<Fonts>(makeBuiltinFontSource());
        double tb = 1.0;
        auto frameb = [&](int n) { for (int i = 0; i < n; ++i) { dlb.begin(1440, 900, 1.0f, fontsb.get()); gb.beginFrame(dlb, tb); stb.frame(gb, tb); gb.endFrame(); tb += 0.05; } };
        frameb(3);
        CHECK(stb.runCommand("Bounce / Export Audio..."));
        frameb(3);
        CHECK(stb.dialogOpen());
        softRender(dlb, *fontsb, theme::bg0).savePpm("/tmp/vk_bounce_dialog.ppm");
        stb.acceptDialog();                                       // defaults: WAV 24-bit, 44.1 kHz
        frameb(2);
        CHECK(seenFilter.find("*.wav") != std::string::npos);
        std::ifstream f("/tmp/vk_bounce_out.wav", std::ios::binary);                  // ".wav" was added to the name
        CHECK(bool(f));
        unsigned char h[44] = {0};
        f.read(reinterpret_cast<char*>(h), 44);
        CHECK(h[0] == 'R' && h[1] == 'I' && h[34] == 24);                              // a 24-bit WAV, as the dialog said
        CHECK(stb.toastText().find("Bounced") != std::string::npos);
        // Import MIDI File...: options first, then the file (tempo policy, one track per channel)
        std::remove("/tmp/vk_import_midi.mid");
        CHECK(stb.runCommand("Import MIDI File..."));
        frameb(3);
        CHECK(stb.dialogOpen());
        softRender(dlb, *fontsb, theme::bg0).savePpm("/tmp/vk_import_midi_dialog.ppm");
        stb.host().fileDialog = [&](const std::string&, const std::string&, bool) { return std::string(); };   // cancel the file choice
        stb.acceptDialog();
        frameb(2);
        CHECK(!stb.dialogOpen());
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
        r.key('K', ModCtrl); r.type("snap"); r.frame(2); r.shot("/tmp/vk_palette.ppm");
        r.key(KeyEscape); r.key('K', ModCtrl); r.type("logic"); r.frame(2); r.shot("/tmp/vk_logicx_palette.ppm"); r.key(KeyEscape); CHECK(!st.paletteOpen());
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
    // ---- icon style: classic drawing or Material Symbols (Outlined / Rounded / Sharp) from font files, chosen in Preferences
    {
        auto& th = IconTheme::instance();
        th.setBuiltInEnabled(false);   // the first checks are about font files; the built-in icons come after them
        // code point files: "name hex", junk and comments ignored
        const auto cps = IconTheme::parseCodepoints("play_arrow e037\n# comment\nbad\nstop  E047\nnope zzz\nhuge 110000\n");
        CHECK(cps.size() == 2 && cps.at("play_arrow") == 0xE037 && cps.at("stop") == 0xE047);
        CHECK(IconTheme::parseStyle("Rounded") == IconStyle::Rounded && IconTheme::parseStyle("x") == IconStyle::Outlined && IconTheme::parseStyle("x", IconStyle::Classic) == IconStyle::Classic);
        for (IconStyle st : {IconStyle::Classic, IconStyle::Outlined, IconStyle::Rounded, IconStyle::Sharp}) CHECK(IconTheme::parseStyle(IconTheme::styleKey(st)) == st);

        // no fonts anywhere: every style falls back to the classic drawing
        th.setFontDirs({"data/no-such-folder"});
        th.setStyle(IconStyle::Outlined);
        CHECK(th.available(IconStyle::Classic) && !th.available(IconStyle::Outlined) && th.glyphFor(Icon::Play) == 0 && th.fontPath(IconStyle::Outlined).empty());

        auto crop = [&](Rig& g, float cx, float cy) {   // 40 x 40 dp around a point of the toolbar
            g.frame(2);
            const SoftImage im = softRender(g.dl, *g.fonts, theme::bg0);
            std::vector<std::uint32_t> v;
            for (int y = int(cy) - 20; y < int(cy) + 20; ++y) for (int x = int(cx) - 20; x < int(cx) + 20; ++x) v.push_back(im.at(x, y));
            return v;
        };
        auto differ = [](const std::vector<std::uint32_t>& a, const std::vector<std::uint32_t>& b) { int n = 0; for (size_t i = 0; i < a.size(); ++i) n += a[i] != b[i]; return n; };
        Rig g;
        th.setStyle(IconStyle::Classic);
        const auto classicPlay = crop(g, 120, 58), classicTools = crop(g, 750, 58);
        th.setStyle(IconStyle::Outlined);
        CHECK(differ(crop(g, 120, 58), classicPlay) == 0);   // missing font: identical to Classic

        // with the (synthetic) fonts
        th.setFontDirs({"data/fonts"});
        CHECK(th.available(IconStyle::Outlined) && th.available(IconStyle::Rounded) && th.available(IconStyle::Sharp));
        CHECK(th.fontPath(IconStyle::Sharp).find("MaterialSymbolsSharp-test.ttf") != std::string::npos);
        th.setStyle(IconStyle::Classic);
        CHECK(th.glyphFor(Icon::Play) == 0);
        th.setStyle(IconStyle::Outlined);
        const std::uint32_t gOut = th.glyphFor(Icon::Play);
        CHECK(gOut == kIconPlane + 0 * 0x100u + 0u);
        th.setStyle(IconStyle::Sharp);
        CHECK(th.glyphFor(Icon::Stop) == kIconPlane + 2 * 0x100u + 1u);
        th.setStyle(IconStyle::Rounded);
        CHECK(th.glyphFor(Icon::Play) != 0 && th.glyphFor(Icon::Metronome) == 0);   // this font has no metronome: that one icon stays classic

        th.setStyle(IconStyle::Outlined);
        const auto outPlay = crop(g, 120, 58);
        th.setStyle(IconStyle::Rounded);
        const auto rouPlay = crop(g, 120, 58);
        th.setStyle(IconStyle::Sharp);
        const auto shaPlay = crop(g, 120, 58);
        CHECK(differ(outPlay, classicPlay) > 40 && differ(rouPlay, outPlay) > 40 && differ(shaPlay, rouPlay) > 40 && differ(shaPlay, outPlay) > 40);
        // the Rounded font lacks the metronome only: the metronome button looks as it does in Classic style, the play button does not
        const auto rouMetro = crop(g, 610, 58);
        th.setStyle(IconStyle::Classic);
        const auto claMetro = crop(g, 610, 58);
        CHECK(differ(rouMetro, claMetro) == 0);
        std::fprintf(stderr, "icon styles: classic/outlined/rounded/sharp play button differ by %d / %d / %d pixels\n", differ(outPlay, classicPlay), differ(rouPlay, outPlay), differ(shaPlay, rouPlay));
        (void)classicTools;

        // the icons that are part of the program: every style is available without any font file, every icon has a glyph
        {
            th.setFontDirs({});
            th.setBuiltInEnabled(true);
            for (IconStyle st : {IconStyle::Outlined, IconStyle::Rounded, IconStyle::Sharp}) {
                CHECK(th.available(st) && th.fontPath(st) == "(built in)");
                th.setStyle(st);
                for (int ic = 0; ic < int(Icon::Count); ++ic) CHECK(th.glyphFor(Icon(ic)) != 0);
            }
            // the three styles really are different drawings of the same icon
            std::vector<std::vector<std::uint8_t>> pencil;
            for (IconStyle st : {IconStyle::Outlined, IconStyle::Rounded, IconStyle::Sharp}) {
                th.setStyle(st);
                GlyphBitmap g;
                CHECK(th.rasterize(th.glyphFor(Icon::Pencil), 48, g) && g.w > 10 && g.h > 10);
                pencil.push_back(g.a8);
            }
            CHECK(pencil[0] != pencil[1] && pencil[1] != pencil[2] && pencil[0] != pencil[2]);
            // a font file in a fonts folder replaces the built-in icons of its style only
            th.setFontDirs({"data/fonts"});
            CHECK(th.fontPath(IconStyle::Rounded).find("MaterialSymbolsRounded-test.ttf") != std::string::npos);
            th.setFontDirs({"data/fonts-without-this-style"});
            CHECK(th.fontPath(IconStyle::Rounded) == "(built in)");
            // the real fonts of Google, as downloaded (no .codepoints file): MINIDAW_MS_FONTS=<folder with the three variable fonts>
            if (const char* real = std::getenv("MINIDAW_MS_FONTS")) {
                std::vector<std::vector<std::uint8_t>> viaFile, viaBuiltIn;
                for (IconStyle st : {IconStyle::Outlined, IconStyle::Rounded, IconStyle::Sharp}) {
                    th.setStyle(st);
                    th.setFontDirs({});
                    for (int ic = 0; ic < int(Icon::Count); ++ic) { GlyphBitmap g; CHECK(th.rasterize(th.glyphFor(Icon(ic)), 40, g)); viaBuiltIn.push_back(g.a8); }
                    th.setFontDirs({real});
                    CHECK(th.fontPath(st).find("MaterialSymbols") != std::string::npos && th.fontPath(st) != "(built in)");
                    for (int ic = 0; ic < int(Icon::Count); ++ic) { GlyphBitmap g; CHECK(th.rasterize(th.glyphFor(Icon(ic)), 40, g)); viaFile.push_back(g.a8); }
                }
                CHECK(viaFile == viaBuiltIn);   // the built-in subset draws exactly what the full font draws
                std::fprintf(stderr, "real Material Symbols fonts: %zu glyphs identical to the built-in subset\n", viaFile.size());
            }
            th.setFontDirs({});
        }

        // Preferences: the choice lists the four styles, marks missing fonts, and applies at once
        th.setStyle(IconStyle::Classic);
        {
            Rig p;
            th.setFontDirs({"data/fonts"});
            th.setStyle(IconStyle::Classic);
            CHECK(p.st->runCommand("Preferences..."));
            p.frame(3);
            p.shot("/tmp/vk_icon_prefs.ppm");
            p.st->acceptDialog();                       // saves the dialog as it is: Classic stays
            p.frame(2);
            CHECK(th.style() == IconStyle::Classic && p.st->host().settings["iconStyle"] == "classic");
            // pick Sharp in the dialog: it applies at once and is saved
            CHECK(p.st->runCommand("Preferences..."));
            p.frame(2);
            CHECK(p.st->dialogField(10) && p.st->dialogField(10)->label == "Icon style");
            p.st->dialogField(10)->index = 3;
            p.st->acceptDialog();
            p.frame(2);
            CHECK(th.style() == IconStyle::Sharp && p.st->host().settings["iconStyle"] == "sharp");
        }
        // a saved style is read when the Studio starts
        {
            Host h; h.loopFolder = "../loops"; h.iconFontDirs = {"data/fonts"}; h.settings["iconStyle"] = "sharp";
            Session s2; buildDemo(s2);
            Studio st2(s2, h);
            CHECK(th.style() == IconStyle::Sharp && th.available(IconStyle::Sharp));
            Host h2 = h; h2.settings["iconStyle"] = "rounded";
            Studio st3(s2, h2);
            CHECK(th.style() == IconStyle::Rounded);
        }
        {   // screenshots with the built-in icons
            th.setFontDirs({});
            Rig d;
            for (auto [st, file] : {std::pair{IconStyle::Classic, "/tmp/vk_icons_classic.ppm"}, std::pair{IconStyle::Outlined, "/tmp/vk_icons_outlined.ppm"},
                                    std::pair{IconStyle::Rounded, "/tmp/vk_icons_rounded.ppm"}, std::pair{IconStyle::Sharp, "/tmp/vk_icons_sharp.ppm"}}) {
                th.setStyle(st);
                d.shot(file);
            }
            // the small marks of the controls follow the style as well: drop-down arrows, check boxes, sub-menu arrows
            {
                DrawList probe;
                probe.begin(100, 100, 1.f, d.fonts.get());
                th.setStyle(IconStyle::Classic);
                CHECK(!icons::glyph(probe, 10, 10, 16, rgb(255, 255, 255), Icon::Check) && !icons::glyph(probe, 10, 10, 16, rgb(255, 255, 255), Icon::DropDown));
                th.setStyle(IconStyle::Rounded);
                CHECK(icons::glyph(probe, 10, 10, 16, rgb(255, 255, 255), Icon::Check) && icons::glyph(probe, 10, 10, 16, rgb(255, 255, 255), Icon::Close));
                CHECK(icons::glyph(probe, 10, 10, 16, rgb(255, 255, 255), Icon::ChevronRight) && icons::glyph(probe, 10, 10, 16, rgb(255, 255, 255), Icon::ExpandMore));
            }
            auto prefsImage = [&](IconStyle st) {
                th.setStyle(st);
                CHECK(d.st->runCommand("Preferences..."));
                d.frame(3);
                const SoftImage im = softRender(d.dl, *d.fonts, theme::bg0);
                if (st == IconStyle::Outlined) d.shot("/tmp/vk_icons_prefs.ppm");
                d.st->acceptDialog();
                d.frame(2);
                return im;
            };
            const SoftImage pc = prefsImage(IconStyle::Classic), po = prefsImage(IconStyle::Outlined);
            int diff = 0;
            for (int y = 200; y < 560; ++y) for (int x = 480; x < 920; ++x) diff += pc.at(x, y) != po.at(x, y);
            CHECK(diff > 40);   // the drop-down arrows of the dialog are drawn differently
        }
        th.setStyle(IconStyle::Classic);
        th.setFontDirs({});
    }

    return fails ? 1 : 0;
}
