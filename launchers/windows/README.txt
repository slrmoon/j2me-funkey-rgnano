FunKey J2ME runtime for Windows
===============================

The same phoneME runtime as on the FunKey S / RG Nano (M3G software 3D,
fkaudio/Sonivox sound, DoJa i-appli support), built for a PC window.
Windows 7 or newer, 32 or 64 bit (also Windows 11 on ARM). Nothing to
install: no Java, no WSL.

Place this folder at a path with Latin letters only (e.g. C:\j2me): the
Java VM opens files through 8-bit file names.

Starting games
--------------
Start FunKeyJ2ME.exe. The Games tab lists the games in the "games" folder
(sub-folders too; "Games folder..." picks another one, e.g. an existing
collection). Double-click a game or press Play. "Open a game..." starts
any .jar / .jam, and you can drop a .jar onto the window.

Dropping a .jar (or .jam) onto FunKeyJ2ME.exe itself starts the game right
away, with its settings, without the window.

i-appli (DoJa) games: game.jam (and game.sp, if any) next to game.jar.

Settings
--------
Display: window size (game screen times 1..8, shrunk to fit the monitor),
  full screen at start, how the picture fills the window, filter:
    Sharp     clean pixels at any size (default)
    Scale2x   rounded edges for pixel art
    Smooth    bilinear
    Pixel     plain nearest neighbour
  the phone's screen size (Auto = 240x240, i-appli: the .jam DrawArea),
  frame limit, Java memory, JIT, bytecode check, extra VM options.
  In the game: drag the window edges to resize; F11 or Alt+Enter = full
  screen.
Controls: which PC keys press which phone key (up to 3 each). Select a
  phone key, "Add key..." (or double-click), press the PC key. "Soft keys"
  picks the codes of the left/right soft keys, which differ by phone brand
  (Nokia -6/-7, Motorola -21/-22, ...): if a game ignores the soft keys,
  try another brand. [Runtime menu] (Esc) opens the runtime's own menu,
  [Full screen] (F11) switches full screen.
Sound & logs: sound on/off, volume, MIDI synth; log file on/off, a console
  window instead, key/event/display logging, FPS counter.

All settings apply to every game, unless "Own settings for the selected
game" is ticked: then that game keeps its own copy (data\games\<game>.ini),
handy for soft keys or the screen size of one game.

Default keys
------------
    Arrows, W A S D             directions
    Enter, Space                fire (select)
    F1, Tab, Q                  left soft key
    F2, Backspace, E            right soft key
    0-9, numpad (phone layout)  phone keys;  Num * / Z = *,  Num + / X = #
    Esc                         runtime menu
    F11, Alt+Enter              full screen
    Close the window to quit.

Files
-----
    settings.ini          settings (written by FunKeyJ2ME.exe)
    data\rms\<game>       saves (record stores)
    data\logs\<game>.log  runtime log of the last run
    data\games\<game>.ini own settings of a game
    data\run\<game>       temporary copies (games in folders with non-Latin
                          names, jars without a .jad)
When the folder is not writable (e.g. Program Files), settings and data go
to %LOCALAPPDATA%\funkey-j2me instead.

Not available on Windows (yet): network connections (games get an
IOException), the sampling profiler, the mass-test scripts of the Linux
build.
