# DoJa (i-appli) branch — status

## Done (2026-10-22)
- Merged `origin/main` into `doja` (M3G, fkaudio/EAS audio, scaling, menu, frame limiter). Old media-sdl/TiMidity path is gone; DoJa sound goes through MMAPI → fkaudio (MLD is converted to MIDI in Java by `MldPcmDecoder`).
- New DoJa runtime (hand-written, `packaging/funkey-s/doja-stubs/`):
  - `com.funkey.doja`: `DoJaMIDlet` (boot), `DoJaRuntime` (.jam properties, single DoJa event thread, keypad state, present/auto-present), `DoJaHost` (the only MIDP Canvas, key mapping), `BitmapFont` (Japanese bitmap fonts), `Stub` (logs first call of unimplemented API), `Render3D` (MascotCapsule placeholder).
  - `com.nttdocomo.ui`: Frame/Canvas/Display/Graphics/Image/Font/Panel/Dialog/Component/Label/Button/ListBox/TextBox/Ticker/ImageLabel/Sprite/SpriteSet/ImageMap/ShortTimer/IApplication/MApplication/PhoneSystem/UIException/MediaManager/AudioPresenter + interfaces. Correct DoJa constants (colors, flip modes, Font sizes, Dialog buttons, keys), drawString baseline, getKeypadState, lock/unlock, drawImage(img,dx,dy,sx,sy,w,h) arg order, drawScaledImage, affine drawImage, pixels get/set, copyArea, polygons, opt `Graphics2` (render modes ADD/SUB/alpha, syncUnlock).
  - `com.nttdocomo.util.Timer`, `com.nttdocomo.lang.XObject/XString`.
- Generated API skeletons for the whole `com.nttdocomo.*` DoJa 5.1 API: `packaging/funkey-s/doja-api/` by `tools/doja/gen_api_stubs.py` (API shape only — signatures/constants — taken from openDoJa sources, GPLv3; bodies are our defaults that log `DoJa: unimplemented X.m`). Regenerate: `python3 tools/doja/gen_api_stubs.py <openDoJa>/src/main/java packaging/funkey-s/doja-api --skip-dir packaging/funkey-s/doja-stubs`. Old game-reference stubs for com.nttdocomo removed (they had wrong types, e.g. Graphics3D as class).
- Fonts: Shinonome 12/16 px (public domain, Debian `xfonts-shinonome`, `pcf2bdf`) → `doja-stubs/com/funkey/doja/f12.bin, f16.bin` via `tools/doja/mkfont.py`. TINY=12, SMALL=16, MEDIUM=24 (2×12), LARGE=30 (2×16); DoJa<3: 12/12/16.
- Launcher (`pm-launch.c`): passes the whole .jam as `-Ddoja.jam.hex=` (runtime reads DrawArea, ProfileVer, AppParam, any key for getParameter); view preset Auto → LCD size = DrawArea (default 240x240).
- Packaging compiles doja-stubs + doja-api, copies *.bin; PHONEME_ROOT made absolute.
- Cloud test: `tools/cloud-test/run-doja.sh game.jam secs "keys"`; `inject.c` now also saves idle screens (first flip + last flipped frame).

## Tested
- Synthetic Hello app: paint, keypad, Japanese text OK (240x240 and 120x130).
- DoJa-1942 (github hectormoralespiloni/DoJa-1942, homebrew) boots to the intro screen; gameplay screenshots not yet checked.
- No real commercial games tested yet (user's games were not attached).

## Next
1. Run user's game corpus, compatibility table, fix most-hit APIs (grep log for `DoJa: unimplemented`, exceptions).
2. MLD natively in fkaudio (loops/cuepoints, sync events) instead of Java MLD→MIDI.
3. PalettedImage, animated MediaImage (drawNthImage), IME (imeOn returns initial text now), HTTP stub.
4. MascotCapsule v3 (`com.nttdocomo.opt.ui.j3d`) via `Render3D` — reuse M3G software rasterizer.
5. Debug: `VMARGS=-Ddoja.trace=1` prints events/present/host paint.
