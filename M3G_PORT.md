# JSR-184 / M3G Port Notes

Goal: add real `javax.microedition.m3g` support for J2ME games that require
JSR-184 Mobile 3D Graphics, including classes such as
`javax.microedition.m3g.World`.

This must not be implemented as empty compatibility stubs. A stub jar fixes
`NoClassDefFoundError`, but M3G games immediately depend on native scene graph,
loader, image, transform, and rendering behavior.

## Current State

- The phoneME build system already has a JSR-184 switch:
  `USE_JSR_184=true`.
- The source tree now includes `phoneme_feature/jsr184`.
- `Verify.gmk` expects `JSR_184_DIR` and `SWERVE_DIR` when JSR-184 is enabled.
- The FunKey SDK sysroot has SDL, AGG, pixman, and zlib, but no EGL/GLES
  libraries.
- The current CLDC VM build has `ENABLE_DYNAMIC_NATIVE_METHODS=0`, so external
  native libraries cannot be used as a drop-in Java optional package.
- `build-ubuntu26-armv7.sh` passes the JSR-184 paths to the MIDP build. Use
  `FUNKEY_USE_JSR_184=true` to include the Java module. Use
  `FUNKEY_ENABLE_M3G_NATIVE=false` only for Java/preverify-only experiments.
- The FunKey KNI layer now has a real native handle table for the first object
  family: `Interface`, `Object3D`, `Node`, `Group`, `World`, `Background`, and
  `Camera`, plus the first `Graphics3D` target/render entry points. This is not
  yet the complete renderer, but it is no longer a class-only compatibility
  shim.
- A generated KNI coverage file supplies the remaining M3G native entrypoints
  so the full phoneME image links while the detailed backing state is ported
  class by class.
- The coverage layer has been promoted for the first real game-critical groups:
  `VertexArray`, `VertexBuffer`, `TriangleStripArray`, `Mesh`, `Image2D`,
  `Texture2D`, and core `Transform` matrix operations now keep native state
  instead of only returning conservative defaults. `Transform` now covers
  matrix get/set, multiply, scale, translate, transpose, inverse, axis rotation,
  quaternion rotation, and float-table transforms.
- Render binding now uses the phoneME LCDUI/putpixel `gxj_screen_buffer` target:
  `Graphics3D.clear` fills the bound RGB565 buffer and `render`/`renderNode`/
  `renderWorld` traverse Mesh/Group/World state. The software backend now has a
  basic filled RGB565 triangle rasterizer with a depth buffer and material
  diffuse color selection, with wireframe edges still drawn as a cheap debug
  outline.
- `Appearance`, `Material`, `CompositingMode`, `PolygonMode`, `Fog`, and
  `Light` now keep native getter/setter state for renderer use.

## Chosen Upstream Candidate

Primary source:

- `SymbianSource/oss.FCL.sf.app.JRT`
- Path: `javauis/m3g_qt`
- License in source headers: Eclipse Public License v1.0
- Contents:
  - complete public `javax.microedition.m3g` Java API
  - JNI native bindings
  - M3G core integration hooks
  - OpenGL ES 1.1 rendering path

Practical reference:

- `nikita36078/J2ME-Loader`
- Path: `app/src/main/java/javax/microedition/m3g` and
  `app/src/main/cpp/m3g`
- This is derived from the Symbian/Nokia source and has useful portability
  changes, but the Java side targets Android/FreeJ2ME-style graphics rather
  than phoneME LCDUI internals.

## Why This Is A Native Port

The M3G Java classes expose many native methods. In phoneME CLDC, unresolved
native methods are handled by VM-native/KNI linkage, not by ordinary desktop
Java JNI.

The upstream M3G binding exports functions like:

```c
Java_javax_microedition_m3g_World__1ctor(JNIEnv*, jclass, ...)
```

The current phoneME VM dynamic native path, when enabled, looks up JNI-style
symbol names but calls them through no-argument VM-native trampolines. In this
build it is disabled anyway. Therefore the upstream JNI layer cannot be loaded
as `libm3g.so` and used directly.

The complete port needs:

1. `phoneme_feature/jsr184/src/config/subsystem.gmk` integration.
2. `javax.microedition.m3g` Java classes compiled into the MIDP/CLDC image.
3. KNI/SNI wrappers for each native method, or a VM build mode that supports
   the upstream binding ABI safely.
4. A renderer backend for FunKey:
   - either software rasterization into LCDUI/SDL pixel buffers, or
   - a bundled software OpenGL ES implementation.
5. LCDUI target binding for `Graphics3D.bindTarget(Graphics)` and `Image2D`.
6. `microedition.m3g.version=1.1` only after the implementation is callable.

## Integration Strategy

Do not add a fake `m3g-support.jar`.

Recommended path:

1. Vendor the upstream M3G Java/native sources under
   `phoneme_feature/jsr184`. Done.
2. Make the Java classes buildable with the project JDK and preverifier.
   In progress: the imported FreeJ2ME Java API has been narrowed to
   `javax.microedition.m3g` and stripped of desktop/Android glue.
3. Port native wrappers from JNI to phoneME KNI incrementally. First milestone:
   - `Interface`
   - `Object3D`
   - `World`
   - `Group`
   - `Graphics3D`
   - `Background`
   - `Camera`
4. Continue the same KNI/state-port pattern for:
   - `Transform`
   - `VertexArray`
   - `VertexBuffer`
   - `TriangleStripArray`
   - `Mesh`
   - `Image2D`
   - `Loader`
5. Bring up a headless smoke test that creates a `World`, `Camera`,
   `VertexArray`, `VertexBuffer`, `TriangleStripArray`, and `Mesh`.
6. Only then wire `Graphics3D.render(...)` to an SDL/software backend.

## Port Files Added

- `src/config/subsystem.gmk`: phoneME subsystem entry.
- `src/config/properties_jsr184.xml`: `microedition.m3g.version`.
- `src/config/jsr184_rom.cfg`: ROM config placeholder.
- `src/classes/javax/microedition/m3g`: imported and CLDC-adjusted Java API.
- `src/native/upstream/m3g`: upstream Nokia/FreeJ2ME native M3G sources.
- `src/native/funkey/m3g_funkey_kni.c`: first KNI binding boundary and native
  method coverage for the basic scene/root render path.
- `src/native/funkey/m3g_funkey_soft.*`: native object table plus SDL/software
  renderer target state.
- `NOTICE-M3G.txt`: source provenance and license notes for the imported M3G
  files.

## Current Verification

- C syntax check passes for the FunKey KNI/software layer with `gcc -Wall
  -Wextra -fsyntax-only`.
- Java source scan is clean for removed desktop/Android dependencies:
  `java.awt`, `java.nio`, `org.recompile`, `System.loadLibrary`,
  `java.util.concurrent`, `java.lang.ref`, enhanced for-loops, and `@Override`.
- The bundled JDK at `phoneme-funkey-clean/zulu7.24.0.1-jdk7.0.191-linux_x64`
  builds the imported M3G Java sources.
- The FunKey SDK build path reaches M3G compile, preverify, ROM generation, and
  final MIDP link:

```sh
JAVA_HOME=$PWD/phoneme-funkey-clean/zulu7.24.0.1-jdk7.0.191-linux_x64 \
PATH=$PWD/phoneme-funkey-clean/zulu7.24.0.1-jdk7.0.191-linux_x64/bin:$PATH \
FUNKEY_USE_JSR_184=true \
FUNKEY_ENABLE_M3G_NATIVE=true \
./phoneme-funkey-clean/build-funkey-s.sh
```

- Current link status: the FunKey SDK build completes and produces ARM
  `runMidlet` with M3G enabled.
- Remaining implementation work is behavioral: the generated coverage layer
  must still be replaced class-by-class for animation, `.m3g` binary loader
  decoding, texture sampling, clipping, lighting, and perspective-correct
  projection.

## Acceptance Tests

Minimum:

- `Class.forName("javax.microedition.m3g.World")` succeeds.
- `System.getProperty("microedition.m3g.version")` returns `1.1`.
- `new World()` succeeds without `UnsatisfiedLinkError`.
- `Loader.load(...)` can parse a simple `.m3g` file.
- `Graphics3D.getInstance().bindTarget(Graphics)` and `render(World)` draw into
  the current LCDUI target.

Real-game:

- A JSR-184 game reaches its first rendered frame instead of failing in MIDlet
  initialization.
- Repeated bind/render/release cycles do not leak enough memory to crash a
  FunKey S session.
