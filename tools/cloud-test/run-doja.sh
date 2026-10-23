#!/bin/bash
# Run a DoJa i-appli (game.jar + game.jam) under qemu-arm, like run.sh does
# for MIDlets.
# usage: run-doja.sh <game.jam|game.jar> <seconds> "<keys>"
#   keys: "ms:sdl_keysym:d|u,..."  (see run.sh)
# env:  T=/root/tc  HOME_DIR=$T/run/home (unsquashed pm.opk)
#       SP_DIR   scratchpad directory (default $T/run/sp)
#       LCD=WxH  override the DrawArea from the .jam
#       other variables as in run.sh (SHOT_DIR, SHOT_MS, AOUT, VMARGS, FPS)
T=${T:-/root/tc}
HOME_DIR=${HOME_DIR:-$T/run/home}
SP_DIR=${SP_DIR:-$T/run/sp}
arg=$(readlink -f "$1")
base=${arg%.*}
jar=$base.jar
jam=$base.jam
[ -f "$jar" ] || { echo "no jar: $jar" >&2; exit 1; }
[ -f "$jam" ] || { echo "no jam: $jam" >&2; exit 1; }
# .jam files are Shift-JIS; the keys are ASCII
jamval() { iconv -f SHIFT_JIS -t UTF-8 "$jam" 2>/dev/null | tr -d '\r' |
           sed -n "s/^[[:space:]]*$1[[:space:]]*=[[:space:]]*//p" | head -1; }
APP_CLASS=$(jamval AppClass)
APP_PARAM=$(jamval AppParam)
APP_VER=$(jamval AppVer)
DRAW=$(jamval DrawArea)
LCD=${LCD:-${DRAW:-240x240}}
mkdir -p "$SP_DIR"
SP=$SP_DIR/$(basename "$base").sp
[ -f "$SP" ] || : > "$SP"
cd "$HOME_DIR/bin" || exit 1
export MIDP_HOME=$HOME_DIR PHONEME_RMS_HOME=${RMS:-$T/run/rms/doja} PHONEME_HEAP_MB=${HEAP_MB:-32}
mkdir -p "$PHONEME_RMS_HOME"
export PHONEME_ENABLE_GP2X_KEYS=1 PHONEME_KEY_PROFILE=${KEY_PROFILE:-doja}
export PHONEME_SCALE_MODE=fit PHONEME_FPS_LIMIT=${FPS:-30} PHONEME_SYNTH=${SYN:-hybrid}
export PHONEME_LCD_SIZE=$LCD PHONEME_SOURCE_SIZE=$LCD
export DOJA_APP_CLASS="$APP_CLASS" DOJA_APP_PARAM="$APP_PARAM" DOJA_APP_VER="$APP_VER"
export DOJA_JAM_PATH="$jam" DOJA_SCRATCHPAD_PATH="$SP"
export SDL_VIDEODRIVER=dummy
if [ -n "$AOUT" ]; then
    export PHONEME_ENABLE_AUDIO=1 SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=$AOUT SDL_DISKAUDIODELAY=46
else
    export SDL_AUDIODRIVER=dummy
fi
export SHOT_DIR=${SHOT_DIR:-} SHOT_MS=${SHOT_MS:-2000} INJECT_KEYS="$3"
export J2ME_GP2X_JOYU=-1 J2ME_GP2X_JOYD=-2 J2ME_GP2X_JOYL=-3 J2ME_GP2X_JOYR=-4 J2ME_GP2X_BUTA=-5 J2ME_GP2X_BUTB=53 J2ME_GP2X_BUTX=-15 J2ME_GP2X_BUTY=-16 J2ME_GP2X_LEFT=42 J2ME_GP2X_RIGHT=35 J2ME_GP2X_START=-7 J2ME_GP2X_SELECT=-6
echo "doja class=$APP_CLASS lcd=$LCD param=$APP_PARAM"
timeout -s KILL "$2" qemu-arm -cpu cortex-a7 -L "$T/qroot" -E LD_PRELOAD="$T/tools/inject.so" \
    ./runMidlet =MaxMethodToCompile3000 $VMARGS "-Ddoja.app.class=$APP_CLASS" "-Ddoja.app.param=$APP_PARAM" \
    "-Ddoja.app.ver=$APP_VER" -Dmicroedition.encoding=SJIS "-Ddoja.scratchpad.path=$SP" \
    "-Ddoja.spsize=$(jamval SPsize | tr -d ' ')" "-Ddoja.jam.hex=$(od -An -v -tx1 "$jam" | tr -d ' \n')" \
    -classpathext "$jar:$HOME_DIR/lib/doja-support.jar" internal com.funkey.doja.DoJaMIDlet
echo "exit=$?"
