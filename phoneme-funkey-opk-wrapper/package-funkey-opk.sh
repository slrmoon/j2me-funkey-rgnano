#!/bin/sh

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PHONEME_ROOT_DEFAULT=$ROOT/../phoneme-funkey-clean/phoneME-GP2X-SDL
if [ -d "$ROOT/phoneME-GP2X-SDL/phoneme_feature" ]; then
    PHONEME_ROOT=${FUNKEY_PHONEME_ROOT:-$ROOT/phoneME-GP2X-SDL}
else
    PHONEME_ROOT=${FUNKEY_PHONEME_ROOT:-$PHONEME_ROOT_DEFAULT}
fi
MEHOME=$PHONEME_ROOT/phoneme_feature
MIDP_BIN=$MEHOME/build_output_funkey_s/midp/bin/arm
MIDP_CLASSES=$MEHOME/build_output_funkey_s/midp/classes.zip
TARGET_CLDC_BIN=$MEHOME/build_output_funkey_s/cldc/linux_arm_vfp/dist/bin
CLDC_DIST=$MEHOME/build_output_funkey_s/cldc/linux_i386/dist
PREVERIFY=$CLDC_DIST/bin/preverify
RUNMIDLET_FALLBACK=${RUNMIDLET_FALLBACK:-$MEHOME/build_output_funkey_s/opk/phoneme-funkey-s/bin/runMidlet}
CLDC_VM_FALLBACK=${CLDC_VM_FALLBACK:-}
JDK_DIR=${JDK_DIR:-$ROOT/zulu7.24.0.1-jdk7.0.191-linux_x64}
PACKAGE_ROOT=$MEHOME/build_output_funkey_s/opk
STAGE=$PACKAGE_ROOT/pm
OPK=$PACKAGE_ROOT/pm.opk
HELLO_SRC=$ROOT/packaging/funkey-s/HelloMidlet.java
NOKIA_STUB_SRC=$ROOT/packaging/funkey-s/nokia-stubs
HELLO_BUILD=$PACKAGE_ROOT/hello-build
FUNKEY_CC=${FUNKEY_CC:-}
APP_JAR=${FUNKEY_MIDLET_JAR:-${1:-}}
APP_JAD=${FUNKEY_MIDLET_JAD:-${2:-}}
APP_MAIN=${FUNKEY_MIDLET_MAIN:-}
APP_TITLE=${FUNKEY_APP_TITLE:-J2ME}
APP_COMMENT=${FUNKEY_APP_COMMENT:-Java ME runtime}
RUNMIDLET_HEAP_ARG=${FUNKEY_RUNMIDLET_HEAP_ARG:-}
APP_ICON_SRC=$ROOT/packaging/funkey-s/java-runtime-icon.png

abs_path() {
    case "$1" in
        /*) printf '%s\n' "$1" ;;
        *) printf '%s/%s\n' "$PWD" "$1" ;;
    esac
}

copy_nonempty_file() {
    src=$1
    dst=$2
    fallback=${3:-}

    if [ -s "$src" ]; then
        cp "$src" "$dst"
        return
    fi

    if [ -n "$fallback" ] && [ -s "$fallback" ]; then
        cp "$fallback" "$dst"
        return
    fi

    echo "Missing non-empty source file: $src" >&2
    if [ -n "$fallback" ]; then
        echo "Fallback was also unavailable: $fallback" >&2
    fi
    exit 1
}

midlet_main_from_text() {
    tr -d '\r' < "$1" |
        sed -n 's/^MIDlet-1:[[:space:]]*//Ip' |
        head -n 1 |
        awk -F, '{ value=$NF; gsub(/^[ \t]+|[ \t]+$/, "", value); print value }'
}

manifest_from_jar() {
    mkdir -p "$HELLO_BUILD/manifest"
    (
        cd "$HELLO_BUILD/manifest"
        "$JDK_DIR/bin/jar" xf "$1" META-INF/MANIFEST.MF
    )
    if [ -f "$HELLO_BUILD/manifest/META-INF/MANIFEST.MF" ]; then
        printf '%s\n' "$HELLO_BUILD/manifest/META-INF/MANIFEST.MF"
    fi
}

jar_has_entry() {
    "$JDK_DIR/bin/jar" tf "$1" | grep -q "^$2$"
}

inject_nokia_stubs() {
    if [ "${FUNKEY_NOKIA_STUBS:-auto}" = "0" ]; then
        return
    fi
    if [ ! -d "$NOKIA_STUB_SRC" ]; then
        return
    fi
    if jar_has_entry "$STAGE/midlets/a.jar" "com/nokia/mid/sound/Sound.class"; then
        return
    fi

    echo "Injecting Nokia compatibility stubs"
    mkdir -p "$HELLO_BUILD/nokia-stubs/classes" "$HELLO_BUILD/nokia-stubs/verified"
    "$JDK_DIR/bin/javac" \
        -source 1.4 -target 1.4 \
        -bootclasspath "$MIDP_CLASSES" \
        -classpath "$MIDP_CLASSES" \
        -d "$HELLO_BUILD/nokia-stubs/classes" \
        "$NOKIA_STUB_SRC/com/nokia/mid/sound/Sound.java" \
        "$NOKIA_STUB_SRC/com/nokia/mid/sound/SoundListener.java" \
        "$NOKIA_STUB_SRC/com/nokia/mid/ui/DeviceControl.java" \
        "$NOKIA_STUB_SRC/com/nokia/mid/ui/DirectGraphics.java" \
        "$NOKIA_STUB_SRC/com/nokia/mid/ui/DirectUtils.java"

    "$PREVERIFY" \
        -classpath "$MIDP_CLASSES:$CLDC_DIST/lib/cldc_classes.zip" \
        -d "$HELLO_BUILD/nokia-stubs/verified" \
        "$HELLO_BUILD/nokia-stubs/classes"

    (
        cd "$HELLO_BUILD/nokia-stubs/verified"
        "$JDK_DIR/bin/jar" uf "$STAGE/midlets/a.jar" \
            com/nokia/mid/sound/Sound.class \
            com/nokia/mid/sound/SoundListener.class \
            com/nokia/mid/ui/DeviceControl.class \
            com/nokia/mid/ui/DirectGraphics.class \
            com/nokia/mid/ui/DirectUtils.class \
            com/nokia/mid/ui/DirectUtils\$DirectGraphicsImpl.class
    )
}

if [ -z "$FUNKEY_CC" ] && command -v arm-funkey-linux-musleabihf-gcc >/dev/null 2>&1; then
    FUNKEY_CC=$(command -v arm-funkey-linux-musleabihf-gcc)
fi

if [ -z "$FUNKEY_CC" ] && [ -x "${FUNKEY_SDK_DIR:-/home/pq/FunKey-sdk-2.3.0}/bin/arm-funkey-linux-musleabihf-gcc" ]; then
    FUNKEY_CC=${FUNKEY_SDK_DIR:-/home/pq/FunKey-sdk-2.3.0}/bin/arm-funkey-linux-musleabihf-gcc
fi

if [ ! -x "$MIDP_BIN/runMidlet" ] && [ ! -s "$RUNMIDLET_FALLBACK" ]; then
    echo "Missing FunKey runMidlet: $MIDP_BIN/runMidlet" >&2
    echo "Run build-funkey-s.sh first." >&2
    exit 1
fi

if [ ! -x "$JDK_DIR/bin/javac" ]; then
    echo "Missing JDK 7 javac: $JDK_DIR/bin/javac" >&2
    exit 1
fi

if [ -z "$FUNKEY_CC" ] || [ ! -x "$FUNKEY_CC" ]; then
    echo "Missing FunKey C compiler. Set FUNKEY_CC or FUNKEY_SDK_DIR." >&2
    exit 1
fi

rm -rf "$STAGE" "$HELLO_BUILD" "$OPK"
mkdir -p "$STAGE/bin" "$STAGE/midlets" "$HELLO_BUILD/classes" "$HELLO_BUILD/meta"
mkdir -p "$HELLO_BUILD/verified"

if [ -n "$APP_JAR" ]; then
    APP_JAR=$(abs_path "$APP_JAR")
    if [ ! -f "$APP_JAR" ]; then
        echo "Missing MIDlet jar: $APP_JAR" >&2
        exit 1
    fi
    cp "$APP_JAR" "$STAGE/midlets/a.jar"
    if [ -n "$APP_JAD" ]; then
        APP_JAD=$(abs_path "$APP_JAD")
        if [ ! -f "$APP_JAD" ]; then
            echo "Missing MIDlet jad: $APP_JAD" >&2
            exit 1
        fi
        cp "$APP_JAD" "$STAGE/midlets/a.jad"
    else
        MANIFEST=$(manifest_from_jar "$APP_JAR" || true)
        if [ -n "${MANIFEST:-}" ]; then
            cp "$MANIFEST" "$STAGE/midlets/a.jad"
        else
            : > "$STAGE/midlets/a.jad"
        fi
    fi
    if [ -z "$APP_MAIN" ]; then
        APP_MAIN=$(midlet_main_from_text "$STAGE/midlets/a.jad")
    fi
    if [ -z "$APP_MAIN" ]; then
        echo "Could not infer MIDlet class. Set FUNKEY_MIDLET_MAIN=YourMidlet." >&2
        exit 1
    fi
    inject_nokia_stubs
else
    BUILTIN_SRC=$HELLO_SRC
    APP_MAIN=HelloMidlet
    "$JDK_DIR/bin/javac" \
        -source 1.4 -target 1.4 \
        -bootclasspath "$MIDP_CLASSES" \
        -classpath "$MIDP_CLASSES" \
        -d "$HELLO_BUILD/classes" \
        "$BUILTIN_SRC"

    "$PREVERIFY" \
        -classpath "$MIDP_CLASSES:$CLDC_DIST/lib/cldc_classes.zip" \
        -d "$HELLO_BUILD/verified" \
        "$HELLO_BUILD/classes"

cat > "$HELLO_BUILD/meta/MANIFEST.MF" <<EOF
Manifest-Version: 1.0
MIDlet-Name: phoneME FunKey Smoke Test
MIDlet-Version: 1.0.0
MIDlet-Vendor: phoneME
MIDlet-1: phoneME, , $APP_MAIN
MicroEdition-Configuration: CLDC-1.1
MicroEdition-Profile: MIDP-2.0

EOF

    "$JDK_DIR/bin/jar" cfm "$STAGE/midlets/a.jar" \
        "$HELLO_BUILD/meta/MANIFEST.MF" \
        -C "$HELLO_BUILD/verified" .

    JAR_SIZE=$(wc -c < "$STAGE/midlets/a.jar" | tr -d ' ')
cat > "$STAGE/midlets/a.jad" <<EOF
MIDlet-Name: phoneME FunKey Smoke Test
MIDlet-Version: 1.0.0
MIDlet-Vendor: phoneME
MIDlet-1: phoneME, , $APP_MAIN
MIDlet-Jar-URL: a.jar
MIDlet-Jar-Size: $JAR_SIZE
MicroEdition-Configuration: CLDC-1.1
MicroEdition-Profile: MIDP-2.0

EOF
fi

JAR_SIZE=$(wc -c < "$STAGE/midlets/a.jar" | tr -d ' ')
sed '/^[Mm][Ii][Dd][Ll][Ee][Tt]-[Jj][Aa][Rr]-[Uu][Rr][Ll]:/d; /^[Mm][Ii][Dd][Ll][Ee][Tt]-[Jj][Aa][Rr]-[Ss][Ii][Zz][Ee]:/d' \
    "$STAGE/midlets/a.jad" > "$HELLO_BUILD/meta/a.jad"
{
    cat "$HELLO_BUILD/meta/a.jad"
    printf 'MIDlet-Jar-URL: a.jar\n'
    printf 'MIDlet-Jar-Size: %s\n\n' "$JAR_SIZE"
} > "$STAGE/midlets/a.jad"
printf '%s\n' "$APP_MAIN" > "$STAGE/midlets/main-class"
echo "Packaging MIDlet class: $APP_MAIN"

copy_nonempty_file "$MIDP_BIN/runMidlet" "$STAGE/bin/runMidlet" "$RUNMIDLET_FALLBACK"
cp "$MIDP_BIN/installMidlet" "$STAGE/bin/"
cp "$MIDP_BIN/listMidlets.sh" "$STAGE/bin/"
cp "$MIDP_BIN/preverify" "$STAGE/bin/"
cp "$MIDP_BIN/j2se_test_keystore.bin" "$STAGE/bin/" 2>/dev/null || true
rm -f "$STAGE/bin/cldc_vm"
if [ -s "$TARGET_CLDC_BIN/cldc_vm" ] || { [ -n "$CLDC_VM_FALLBACK" ] && [ -s "$CLDC_VM_FALLBACK" ]; }; then
    copy_nonempty_file "$TARGET_CLDC_BIN/cldc_vm" "$STAGE/bin/cldc_vm" "$CLDC_VM_FALLBACK"
fi
cp -R "$MEHOME/build_output_funkey_s/midp/lib" "$STAGE/lib"
rm -rf "$STAGE/lib/freepats"
cp -R "$MEHOME/build_output_funkey_s/midp/appdb" "$STAGE/appdb"
echo "Compiling SDL launcher (browser mode)"

FUNKEY_SDK_DIR=${FUNKEY_SDK_DIR:-$(dirname "$(dirname "$FUNKEY_CC")")}
SYSROOT=$FUNKEY_SDK_DIR/arm-funkey-linux-musleabihf/sysroot
LAUNCH_SRC=$ROOT/packaging/funkey-s/pm-launch.c
if [ -f "$LAUNCH_SRC" ]; then
    "$FUNKEY_CC" -Os -s \
        -I"$SYSROOT/usr/include/SDL" \
        "$LAUNCH_SRC" \
        -o "$STAGE/pm" \
        -lSDL -lSDL_gfx -lm
    echo "SDL launcher built: $(file "$STAGE/pm")"
else
    echo "WARNING: pm-launch.c not found, falling back to stub"
    cp "$MIDP_BIN/runMidlet" "$STAGE/pm" 2>/dev/null || {
        echo "No launcher available" >&2
        exit 1
    }
fi

{
    printf 'pm-runtime-v1\n'
    date +%s
    wc -c "$STAGE/bin/runMidlet" "$STAGE/bin/cldc_vm" "$STAGE/lib/soundfont/default.sf2" 2>/dev/null || true
} > "$STAGE/runtime-version"

cat > "$STAGE/r.sh" <<'EOF'
#!/bin/sh
APP_DIR=/mnt/FunKey/.pm
mkdir -p "$APP_DIR"
EARLY_LOG="$APP_DIR/r-early.log"
echo "r.sh v16 enter" > "$EARLY_LOG"
echo "argv0=$0 pwd=$(pwd)" >> "$EARLY_LOG"

case "$0" in
    */*) OPK_DIR=${0%/*} ;;
    *) OPK_DIR=. ;;
esac
cd "$OPK_DIR" || {
    echo "cd OPK_DIR failed: $OPK_DIR" >> "$EARLY_LOG"
    exit 126
}
OPK_DIR=$(pwd)
echo "OPK_DIR=$OPK_DIR" >> "$EARLY_LOG"

echo "mkdir app dirs" >> "$EARLY_LOG"
mkdir -p "$APP_DIR/bin" "$APP_DIR/midlets" || {
    echo "mkdir app dirs failed" >> "$EARLY_LOG"
    exit 126
}
RUNTIME_CURRENT=0
if [ -f "$OPK_DIR/runtime-version" ] && [ -f "$APP_DIR/runtime-version" ] && cmp -s "$OPK_DIR/runtime-version" "$APP_DIR/runtime-version"; then
    RUNTIME_CURRENT=1
fi
if [ "$RUNTIME_CURRENT" != "1" ]; then
    echo "runtime update begin" >> "$EARLY_LOG"
    rm -rf "$APP_DIR/lib" "$APP_DIR/bin" || {
        echo "remove old runtime failed" >> "$EARLY_LOG"
        exit 126
    }
    mkdir -p "$APP_DIR/bin" || {
        echo "mkdir bin failed" >> "$EARLY_LOG"
        exit 126
    }
    cp -R "$OPK_DIR/lib" "$APP_DIR/lib" || {
        echo "copy lib failed from $OPK_DIR/lib" >> "$EARLY_LOG"
        exit 126
    }
    cp "$OPK_DIR/runtime-version" "$APP_DIR/runtime-version" 2>/dev/null || true
    echo "runtime update lib ok" >> "$EARLY_LOG"
else
    echo "runtime current, skip lib/bin copy" >> "$EARLY_LOG"
fi
echo "init appdb (first launch only)" >> "$EARLY_LOG"
if [ ! -e "$APP_DIR/appdb" ]; then
    cp -R "$OPK_DIR/appdb" "$APP_DIR/appdb" || {
        echo "copy appdb failed from $OPK_DIR/appdb" >> "$EARLY_LOG"
    }
    echo "appdb initialized" >> "$EARLY_LOG"
else
    echo "appdb already exists, preserving" >> "$EARLY_LOG"
fi
export MIDP_HOME="$APP_DIR"
export PHONEME_TIMIDITY_SYNTHETIC="${PHONEME_TIMIDITY_SYNTHETIC:-1}"
export PHONEME_ENABLE_GP2X_KEYS="${PHONEME_ENABLE_GP2X_KEYS:-1}"
if [ "${PHONEME_DEBUG_LOGS:-0}" = "1" ]; then
    export PHONEME_KEY_DEBUG="${PHONEME_KEY_DEBUG:-1}"
    export PHONEME_EVENT_DEBUG="${PHONEME_EVENT_DEBUG:-1}"
    export PHONEME_REFRESH_DEBUG="${PHONEME_REFRESH_DEBUG:-1}"
fi
export PHONEME_KEY_PROFILE="${PHONEME_KEY_PROFILE:-game}"
DEFAULT_RUNMIDLET_HEAP_ARG='__RUNMIDLET_HEAP_ARG__'
RUNMIDLET_HEAP_ARG=${PHONEME_RUNMIDLET_HEAP_ARG:-$DEFAULT_RUNMIDLET_HEAP_ARG}
LOG="$APP_DIR/l"
echo "copy bin begin" >> "$EARLY_LOG"
if [ "$RUNTIME_CURRENT" != "1" ]; then
    cp "$OPK_DIR/bin/runMidlet" "$APP_DIR/bin/runMidlet" || {
        echo "copy runMidlet failed" >> "$EARLY_LOG"
        exit 126
    }
    cp "$OPK_DIR/bin/installMidlet" "$APP_DIR/bin/installMidlet" || {
        echo "copy installMidlet failed" >> "$EARLY_LOG"
        exit 126
    }
    cp "$OPK_DIR/bin/listMidlets.sh" "$APP_DIR/bin/listMidlets.sh" || {
        echo "copy listMidlets failed" >> "$EARLY_LOG"
        exit 126
    }
    cp "$OPK_DIR/bin/preverify" "$APP_DIR/bin/preverify" || {
        echo "copy preverify failed" >> "$EARLY_LOG"
        exit 126
    }
    cp "$OPK_DIR/bin/cldc_vm" "$APP_DIR/bin/cldc_vm" 2>/dev/null || true
    cp "$OPK_DIR/bin/j2se_test_keystore.bin" "$APP_DIR/bin/j2se_test_keystore.bin" 2>/dev/null || true
fi
echo "copy bin ok" >> "$EARLY_LOG"
cp "$OPK_DIR/midlets/a.jar" "$APP_DIR/midlets/a.jar" || {
    echo "copy jar failed" >> "$EARLY_LOG"
    exit 126
}
cp "$OPK_DIR/midlets/a.jad" "$APP_DIR/midlets/a.jad" || {
    echo "copy jad failed" >> "$EARLY_LOG"
    exit 126
}
cp "$OPK_DIR/midlets/main-class" "$APP_DIR/midlets/main-class" || {
    echo "copy main class failed" >> "$EARLY_LOG"
    exit 126
}
echo "write jad" >> "$EARLY_LOG"
JAR_SIZE=$(wc -c < "$APP_DIR/midlets/a.jar" | tr -d ' ')
sed '/^[Mm][Ii][Dd][Ll][Ee][Tt]-[Jj][Aa][Rr]-[Uu][Rr][Ll]:/d; /^[Mm][Ii][Dd][Ll][Ee][Tt]-[Jj][Aa][Rr]-[Ss][Ii][Zz][Ee]:/d' \
    "$OPK_DIR/midlets/a.jad" > "$APP_DIR/midlets/a.jad"
{
    printf 'MIDlet-Jar-URL: file://%s/midlets/a.jar\n' "$APP_DIR"
    printf 'MIDlet-Jar-Size: %s\n\n' "$JAR_SIZE"
} >> "$APP_DIR/midlets/a.jad"
echo "chmod bin" >> "$EARLY_LOG"
chmod +x "$APP_DIR/bin/runMidlet" "$APP_DIR/bin/installMidlet" "$APP_DIR/bin/listMidlets.sh" "$APP_DIR/bin/preverify"
if [ "$RUNTIME_CURRENT" != "1" ]; then
    chmod +x "$APP_DIR/bin/cldc_vm" 2>/dev/null || true
fi

echo "cd app bin" >> "$EARLY_LOG"
cd "$APP_DIR/bin"
echo "create main log" >> "$EARLY_LOG"
: > "$LOG"
echo "redirect main log" >> "$EARLY_LOG"
exec >> "$LOG" 2>&1

child_pid=

stop_midlet() {
    if [ -n "${child_pid:-}" ] && kill -0 "$child_pid" 2>/dev/null; then
        echo "launcher trap: forwarding signal to runMidlet pid=$child_pid"
        kill "$child_pid" 2>/dev/null || true
        wait "$child_pid" 2>/dev/null || true
    fi
}

trap 'stop_midlet; exit 0' INT TERM HUP

echo "pm launcher v19 signal-safe"
echo "Starting phoneME FunKey runtime"
date
echo "APP_DIR=$APP_DIR"
echo "MIDP_HOME=$MIDP_HOME"
echo "PHONEME_ENABLE_GP2X_KEYS=$PHONEME_ENABLE_GP2X_KEYS"
echo "PHONEME_KEY_PROFILE=$PHONEME_KEY_PROFILE"
echo "RUNMIDLET_HEAP_ARG=${RUNMIDLET_HEAP_ARG:-}"
file ./runMidlet 2>/dev/null || true
if [ -x ./cldc_vm ]; then
    echo "CLDC VM sanity"
    ./cldc_vm -version || true
fi
echo "Running direct classpath MIDlet"
MIDLET_CLASS=$(cat "$APP_DIR/midlets/main-class")
echo "MIDLET_CLASS=$MIDLET_CLASS"
if [ -n "${RUNMIDLET_HEAP_ARG:-}" ]; then
    # Expand VM args into separate argv entries for runMidlet/JVM_ParseOneArg.
    # shellcheck disable=SC2086
    set -- $RUNMIDLET_HEAP_ARG
    ./runMidlet "$@" -classpathext "$APP_DIR/midlets/a.jar" internal "$MIDLET_CLASS" &
else
    ./runMidlet -classpathext "$APP_DIR/midlets/a.jar" internal "$MIDLET_CLASS" &
fi
child_pid=$!
echo "runMidlet pid: $child_pid"
wait "$child_pid"
status=$?
child_pid=
echo "runMidlet exit status: $status"
echo "$status" > "$APP_DIR/s"
exit "$(cat "$APP_DIR/s" 2>/dev/null || echo 1)"
EOF
escaped_heap_arg=$(printf '%s' "$RUNMIDLET_HEAP_ARG" | sed "s/'/'\\\\''/g")
sed -i "s|__RUNMIDLET_HEAP_ARG__|$escaped_heap_arg|" "$STAGE/r.sh"
chmod +x "$STAGE/r.sh"

cat > "$STAGE/pm.funkey-s.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$APP_TITLE
Comment=$APP_COMMENT
Exec=pm
Icon=pm
Categories=emulators

EOF

if [ -f "$APP_ICON_SRC" ]; then
    cp "$APP_ICON_SRC" "$STAGE/pm.png"
else
    printf '%s' \
        'iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAVUlEQVR4nO2WQQoAIAgE8/9P' \
        'vWgQpEgtQvYhDoyrs4IkkwAAgHl1bQF4pykA5hJQAHMJqD2s8QfQnU6YAHMJqD0S' \
        'kN9A9zphAswloPb+f4DmEtAAcwmoPQAAwOlXB9+MJPz57QAAAABJRU5ErkJggg==' |
        base64 -d > "$STAGE/pm.png"
fi

mksquashfs "$STAGE" "$OPK" -all-root -noappend -no-exports -no-xattrs

echo "$OPK"
