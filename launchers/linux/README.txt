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

JPEG pictures (many i-appli) also need, optionally:

    sudo apt install libsdl-image1.2:i386

without it the runtime still starts, only JPEG images fail to load.

(On newer Ubuntu libsdl1.2debian is provided by sdl12-compat.)
unzip or python3 is used to read jar manifests.

Running
-------
    ./run.sh /path/to/game.jar

    SCALE=4 ./run.sh game.jar          bigger window (240x240 times 4)
    LCD=240x320 ./run.sh game.jar      screen size the game sees
    MODE=integer FILTER=pixel ./run.sh game.jar

Keys: the games get the keys of keys.conf (next to the scripts) in turn;
edit "keys = ok, 5, soft1" (names: ok 0-9 * # up down left right soft1
soft2, "down x3" repeats) and "first" / "every" (game seconds), or pass
--keys "down x2,ok" for one run. The report and every log.txt say which
keys were pressed and how often; games are listed by their own name
(MIDlet-Name / AppName) with the file below it.

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

Testing many games at once
--------------------------
    ./test-games.py '/games/*/*.jar' -o results -j 6 -t 15 --speed 3

Runs every jar (DoJa: with its .jam) for -t seconds, several at a time,
without windows; presses OK, 5 and the left soft key in turn every 1.5 s of game time (--keys,
--key-every), speeds the game clock up (--speed) and saves for each game
the console log and screenshots. results/index.html shows all games with
their last frame (OK / WARN / FAIL), results/summary.csv lists them.
Screenshots are PNG with python3-pil (sudo apt install python3-pil),
BMP otherwise. ./test-games.py -h lists all options.

i-appli (DoJa) games:
    ./test-doja.py /mnt/d/j2me-x86/appli -o results-doja -j 6 -t 15 --speed 3
runs every .jam with its jar (same name, PackageURL, or the only jar of
the folder that holds its AppClass); a .jam without a jar is SKIP. The
.sp scratchpad is copied for each run, the game folders stay untouched.
run.sh takes JAM=file.jam and SP=file.sp for the same purpose.
