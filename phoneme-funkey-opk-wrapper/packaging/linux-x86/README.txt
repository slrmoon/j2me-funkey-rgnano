FunKey J2ME runtime for desktop Linux (32-bit x86)
==================================================

The same phoneME runtime as on the FunKey S / RG Nano (M3G software 3D,
fkaudio/Sonivox sound, DoJa i-appli support), built for a PC window.

Requirements
------------
The runtime is a 32-bit program. On 64-bit Debian/Ubuntu install the
i386 libraries once:

    sudo dpkg --add-architecture i386
    sudo apt update
    sudo apt install libc6:i386 libstdc++6:i386 zlib1g:i386 \
        libsdl1.2debian:i386 libsdl-gfx1.2-5:i386

(On newer Ubuntu libsdl1.2debian is provided by sdl12-compat.)
unzip or python3 is used to read jar manifests.

Running
-------
    ./run.sh /path/to/game.jar

    SCALE=4 ./run.sh game.jar          bigger window (240x240 times 4)
    LCD=240x320 ./run.sh game.jar      screen size the game sees
    MODE=integer FILTER=pixel ./run.sh game.jar

i-appli (DoJa) games: put game.jam (and game.sp, if any) next to game.jar.
Saves are kept in ~/.local/share/funkey-j2me/rms/<game>.

Keys
----
    Arrows                      directions
    Enter / Space / Left Alt    fire (select)
    F1 / Tab / Left Ctrl        left soft key
    F2 / Esc / Backspace        right soft key
    0-9, numpad, * #            phone keys
    Close the window to quit.
