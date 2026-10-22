#!/bin/bash
# Runs the pm launcher (unsquashed pm.opk in $T/run/home) under qemu-arm with games in /mnt/java.
# usage: run-launcher.sh <seconds> "<launcher keys>"; GAME_KEYS="..." are injected into the game.
# The first run installs the runtime into /mnt/FunKey/.pm; from the second run runMidlet is wrapped
# so the ARM child also runs under qemu (no binfmt_misc in the container).
# run the pm launcher under qemu; runMidlet is wrapped so the ARM child also runs in qemu
T=/root/tc
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SHOT_DIR=${SHOT_DIR:-/tmp/lshots} SHOT_MS=${SHOT_MS:-2000} INJECT_KEYS="$2"
mkdir -p $SHOT_DIR
# first start installs the runtime into /mnt/FunKey/.pm; then wrap runMidlet
if [ ! -f /mnt/FunKey/.pm/bin/runMidlet.arm ] && [ -f /mnt/FunKey/.pm/bin/runMidlet ]; then
  mv /mnt/FunKey/.pm/bin/runMidlet /mnt/FunKey/.pm/bin/runMidlet.arm
  cat > /mnt/FunKey/.pm/bin/runMidlet <<EOS
#!/bin/sh
echo "[wrapper] runMidlet \$*" | cut -c1-300 >&2
export INJECT_KEYS="\$GAME_KEYS"
exec qemu-arm -cpu cortex-a7 -L $T/qroot -E LD_PRELOAD=$T/tools/inject.so /mnt/FunKey/.pm/bin/runMidlet.arm "\$@"
EOS
  chmod +x /mnt/FunKey/.pm/bin/runMidlet
fi
cd $T/run/home && timeout -s KILL "$1" qemu-arm -cpu cortex-a7 -L $T/qroot -E LD_PRELOAD=$T/tools/inject.so ./pm
