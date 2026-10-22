# Building and testing in a fresh Linux container (no device)

Everything below was used to build the runtime, run games under `qemu-arm`
and compare output, on Ubuntu 24.04 amd64. Paths assume `T=/root/tc`.

## 1. Packages

```bash
apt-get install -y gcc g++ gcc-multilib g++-multilib libc6-dev-i386 make file \
    squashfs-tools qemu-user python3-pil valgrind
# optional: gdb-multiarch (debug VM), fluidsynth (MIDI reference renders)
```

## 2. Toolchain

```bash
T=/root/tc; mkdir -p $T && cd $T
curl -LO https://github.com/FunKey-Project/FunKey-OS/releases/download/FunKey-OS-2.3.0/FunKey-sdk-2.3.0.tar.gz
tar xf FunKey-sdk-2.3.0.tar.gz && (cd FunKey-sdk-2.3.0 && ./relocate-sdk.sh)
curl -LO https://cdn.azul.com/zulu/bin/zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz
tar xf zulu7.24.0.1-ca-jdk7.0.191-linux_x64.tar.gz
```

Ubuntu 24.04's i386 static libm has no `fmod`/`fmodf`, so the host `romgen`
link fails. Build the shim and pass it in `HOST_LINK_FLAGS` (not needed on
Ubuntu 26):

```bash
gcc -m32 -O2 -c tools/cloud-test/fmodshim.c -o $T/fmodshim.o
```

## 3. Build runtime and OPK

```bash
cd phoneme-funkey-clean
HOST_LINK_FLAGS="-m32 $T/fmodshim.o" JDK_DIR=$T/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 \
FUNKEY_SDK_DIR=$T/FunKey-sdk-2.3.0 ./build-funkey-s.sh
cd ../phoneme-funkey-opk-wrapper
FUNKEY_PHONEME_ROOT=../phoneme-funkey-clean/phoneME-GP2X-SDL \
JDK_DIR=$T/zulu7.24.0.1-ca-jdk7.0.191-linux_x64 FUNKEY_SDK_DIR=$T/FunKey-sdk-2.3.0 \
FUNKEY_CC=$T/FunKey-sdk-2.3.0/bin/arm-funkey-linux-musleabihf-gcc ./package-funkey-opk.sh
```

Compiler flags are not tracked by make: after changing them delete
`phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s`.

## 4. ARM emulator

```bash
SYS=$T/FunKey-sdk-2.3.0/arm-funkey-linux-musleabihf/sysroot
mkdir -p $T/qroot && cp -a $SYS/lib $SYS/usr $T/qroot/
ln -sf libc.so $T/qroot/lib/ld-musl-armhf.so.1
mkdir -p $T/tools
$T/FunKey-sdk-2.3.0/bin/arm-funkey-linux-musleabihf-gcc -shared -fPIC -O2 \
    -I$SYS/usr/include/SDL tools/cloud-test/inject.c -o $T/tools/inject.so -lSDL -ldl
unsquashfs -d $T/run/home phoneme-funkey-clean/phoneME-GP2X-SDL/phoneme_feature/build_output_funkey_s/opk/pm.opk
cp tools/cloud-test/run.sh $T/run/
```

Run: `SHOT_DIR=/tmp/shots SHOT_MS=3000 $T/run/run.sh game.jar Main 60 "8000:13:d,8150:13:u"`
(Enter at 8 s). Frames land as BMP in `SHOT_DIR`; stdout/stderr carry
`System.out` and VM messages. `AOUT=out.raw` records the speaker output.

Notes:
- qemu is 5–10x slower than the device; timing and NEON speed are not
  representative, correctness is.
- The VM ignores SIGTERM, `run.sh` kills with SIGKILL. Do not `pkill -f`
  a pattern that matches your own shell.
- Uncaught/caught exceptions: product VM has `ENABLE_STACK_TRACE`, so
  `printStackTrace()` prints frames; obfuscated games can be patched to call
  it in their catch handler.
