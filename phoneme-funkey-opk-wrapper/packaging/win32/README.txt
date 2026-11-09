FunKey J2ME runtime for Windows
===============================

The same phoneME runtime as on the FunKey S / RG Nano (M3G software 3D,
fkaudio/Sonivox sound, DoJa i-appli support), built for a PC window.
Windows 7 or newer, 32 or 64 bit (also Windows 11 on ARM). Nothing to
install: no Java, no WSL.

Running
-------
Drag a game's .jar onto FunKeyJ2ME.exe, or start FunKeyJ2ME.exe and pick
the .jar in the dialog. From a command prompt:

    FunKeyJ2ME.exe C:\games\game.jar

Tip: right-click a .jar > Open with > Choose another app > FunKeyJ2ME.exe
to start games with a double click.

i-appli (DoJa) games: put game.jam (and game.sp, if any) next to game.jar.
Dropping the .jam itself also works.

Settings (window size, screen size, scaling, sound...) are in settings.ini.

Keys
----
    Arrows                      directions
    Enter / Space / Left Alt    fire (select)
    F1 / Tab / Left Ctrl        left soft key
    F2 / Esc / Backspace        right soft key
    0-9, numpad, * #            phone keys
    Close the window to quit.

Files
-----
    data\rms\<game>     saves (record stores)
    data\logs\<game>.log  runtime log of the last run
    data\run\<game>     temporary copies (games in folders with non-Latin
                        names, jars without a .jad)
When the runtime folder is not writable (e.g. Program Files), data goes to
%LOCALAPPDATA%\funkey-j2me instead.

The runtime folder itself is best placed at a path with Latin letters only
(e.g. C:\j2me): the Java VM opens files through 8-bit file names.

Not available on Windows (yet): network connections (games get an
IOException), the sampling profiler, the mass-test scripts of the Linux
build.
