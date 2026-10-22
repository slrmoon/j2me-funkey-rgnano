#!/bin/bash
# Run a MIDlet under qemu-arm with SDL dummy video (and optional disk audio).
# usage: run.sh <jar> <main class> <seconds> "<keys>"
#   keys: "ms:sdl_keysym:d|u,..."   e.g. "8000:13:d,8150:13:u" (Enter at 8 s)
# env:  T=/root/tc  HOME_DIR=$T/run/home (unsquashed pm.opk)  FPS=30
#       SHOT_DIR/SHOT_MS  frame dumps (BMP) every SHOT_MS ms
#       AOUT=file.raw     record audio (22050 Hz s16 stereo) instead of muting
#       VMARGS="-int"     interpreter only
# A sibling <jar minus .jar>.jad is read for getAppProperty(); the device
# launcher creates it from the manifest, do the same here
# (unzip -p x.jar META-INF/MANIFEST.MF > x.jad).
T=${T:-/root/tc}
HOME_DIR=${HOME_DIR:-$T/run/home}
RMS=${RMS:-$T/run/rms/default}
mkdir -p "$RMS"
cd "$HOME_DIR/bin" || exit 1
export MIDP_HOME=$HOME_DIR PHONEME_RMS_HOME=$RMS PHONEME_HEAP_MB=${HEAP_MB:-16}
export PHONEME_ENABLE_GP2X_KEYS=1 PHONEME_KEY_PROFILE=${KEY_PROFILE:-nokia-s40}
export PHONEME_SCALE_MODE=fit PHONEME_FPS_LIMIT=${FPS:-30} PHONEME_SYNTH=${SYN:-hybrid}
export SDL_VIDEODRIVER=dummy
if [ -n "$AOUT" ]; then
    export PHONEME_ENABLE_AUDIO=1 SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=$AOUT SDL_DISKAUDIODELAY=46
else
    export SDL_AUDIODRIVER=dummy
fi
export SHOT_DIR=${SHOT_DIR:-} SHOT_MS=${SHOT_MS:-2000} INJECT_KEYS="$4"
export J2ME_GP2X_JOYU=-1 J2ME_GP2X_JOYD=-2 J2ME_GP2X_JOYL=-3 J2ME_GP2X_JOYR=-4 J2ME_GP2X_BUTA=-5 J2ME_GP2X_BUTB=53 J2ME_GP2X_BUTX=-15 J2ME_GP2X_BUTY=-16 J2ME_GP2X_LEFT=42 J2ME_GP2X_RIGHT=35 J2ME_GP2X_START=-7 J2ME_GP2X_SELECT=-6
# the VM ignores SIGTERM: kill hard
timeout -s KILL "$3" qemu-arm -cpu cortex-a7 -L "$T/qroot" -E LD_PRELOAD="$T/tools/inject.so" \
    ./runMidlet $VMARGS -classpathext "$1" internal "$2"
echo "exit=$?"
