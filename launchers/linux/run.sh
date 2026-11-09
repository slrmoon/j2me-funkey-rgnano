#!/bin/bash
# Run a J2ME (MIDP) or i-appli (DoJa) game with the desktop runtime.
#
#   ./run.sh game.jar
#
# MIDP: the main class comes from MIDlet-1 in the manifest (or a .jad next
# to the jar). DoJa: a .jam next to the jar (JAM=file.jam, game.jam, or the only .jam in
# the folder if its AppClass is in the jar); the scratchpad is game.sp.
#
# Environment (all optional):
#   SCALE=3         window size multiplier of the 240x240 screen (1..8)
#   LCD=240x320     screen size the game sees (default: runtime default,
#                   DoJa: the .jam DrawArea)
#   MODE=fit        fit | integer | crop | fill | center | zoom | auto
#   FILTER=sharp    sharp | smooth | pixel
#   FPS=30          frame limit;  HEAP_MB=16 (DoJa 32)
#   MUTE=1          no sound;     VMARGS="-int" extra VM options
#   SP=save.sp      DoJa scratchpad file (default: game.sp by the jar)
#   VERIFY=1        check bytecode like a phone does (off by default:
#                   modded games often fail it but run fine)
# Saves (RMS) go to ${XDG_DATA_HOME:-~/.local/share}/funkey-j2me/rms/<game>.
#
# Keys: arrows, Enter/Space = fire, F1/Tab = left soft key,
# F2/Esc/Backspace = right soft key, 0-9 / numpad, * #. Menu: see README.

set -e
HERE=$(cd "$(dirname "$0")" && pwd)

if [ $# -lt 1 ] || [ ! -f "$1" ]; then
    sed -n '2,24p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
fi

JAR=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
BASE=${JAR%.*}
NAME=$(basename "$BASE")
DIR=$(dirname "$JAR")
DATA=${XDG_DATA_HOME:-$HOME/.local/share}/funkey-j2me
# The VM cannot open a jar (or scratchpad) whose path has non-ASCII
# characters (BatSwingⅢ/, ギャング弾.JAR: ClassNotFoundException for the
# main class): such files are reached through ASCII links in $DATA/run.
nonascii() { printf '%s' "$1" | LC_ALL=C grep -q '[^ -~]'; }
if nonascii "$NAME"; then
    NAME=$(printf '%s' "$NAME" | LC_ALL=C tr -c 'A-Za-z0-9._-' '_')_$(printf '%s' "$JAR" | cksum | cut -d' ' -f1)
fi
mkdir -p "$DATA/rms/$NAME" "$DATA/run/$NAME"
ascii_link() {  # ascii_link <file> <link name>: prints a path the VM can open
    if nonascii "$1"; then
        ln -sfn "$1" "$DATA/run/$NAME/$2"
        printf '%s' "$DATA/run/$NAME/$2"
    else
        printf '%s' "$1"
    fi
}

jar_entry() {  # jar_entry <jar> <entry>: print one entry
    if command -v unzip >/dev/null 2>&1; then
        unzip -p "$1" "$2" 2>/dev/null
    else
        python3 -c 'import sys,zipfile; sys.stdout.buffer.write(zipfile.ZipFile(sys.argv[1]).read(sys.argv[2]))' "$1" "$2" 2>/dev/null
    fi
}
manifest() {  # manifest <jar>: the manifest with continuation lines joined
    # name in any letter case (Meta-inf/Manifest.mf works on phones), no
    # CR, no UTF-8 BOM; a line starting with a space continues the last one
    if command -v unzip >/dev/null 2>&1; then
        unzip -Cp "$1" META-INF/MANIFEST.MF 2>/dev/null
    else
        python3 -c 'import sys,zipfile
z=zipfile.ZipFile(sys.argv[1])
n=[i for i in z.namelist() if i.upper()=="META-INF/MANIFEST.MF"]
n and sys.stdout.buffer.write(z.read(n[0]))' "$1" 2>/dev/null
    fi | tr -d '\r' | sed '1s/^\xef\xbb\xbf//' |
        LC_ALL=C awk 'NR > 1 && /^ / { line = line substr($0, 2); next }
             NR > 1 { print line } { line = $0 } END { if (NR) print line }'
}
jar_has() {  # jar_has <jar> <entry>
    if command -v unzip >/dev/null 2>&1; then
        unzip -l "$1" "$2" >/dev/null 2>&1
    else
        python3 -c 'import sys,zipfile; zipfile.ZipFile(sys.argv[1]).getinfo(sys.argv[2])' "$1" "$2" 2>/dev/null
    fi
}
jam_value() {  # jam_value <jam> <key>
    tr -d '\r' < "$1" | grep -a "^$2 *=" | head -n 1 | sed "s/^$2 *= *//; s/ *\$//"
}

# DoJa: $JAM, game.jam, or the only .jam of the folder when its AppClass is in
# this jar (dumps often name the two differently); MIDP otherwise
APP_CLASS=""
if [ -n "${JAM:-}" ]; then        # JAM=path/game.jam ./run.sh game.jar
    [ -f "$JAM" ] || { echo "No such .jam: $JAM" >&2; exit 1; }
    APP_CLASS=$(jam_value "$JAM" AppClass)
elif [ -f "$BASE.jam" ] || [ -f "$BASE.JAM" ]; then
    JAM=$BASE.jam
    [ -f "$JAM" ] || JAM=$BASE.JAM
    APP_CLASS=$(jam_value "$JAM" AppClass)
else
    JAM=""
    shopt -s nullglob
    jams=("$DIR"/*.jam)
    shopt -u nullglob
    if [ ${#jams[@]} -eq 1 ]; then
        cls=$(jam_value "${jams[0]}" AppClass)
        if [ -n "$cls" ] && jar_has "$JAR" "$(printf '%s' "$cls" | tr . /).class"; then
            JAM=${jams[0]}
            APP_CLASS=$cls
        fi
    fi
fi

export MIDP_HOME=$HERE
# "Restart game" in the runtime menu leaves this file and exits; run_vm
# then starts the game again
export PHONEME_RESTART_FLAG=$DATA/run/$NAME/restart-request
run_vm() {
    local rc
    while :; do
        rm -f "$PHONEME_RESTART_FLAG"
        ./runMidlet "$@" && rc=0 || rc=$?
        [ -f "$PHONEME_RESTART_FLAG" ] || break
    done
    rm -f "$PHONEME_RESTART_FLAG"
    return "$rc"
}
[ "${VERIFY:-0}" = 1 ] || VMARGS="-UseVerifier ${VMARGS:-}"
export PHONEME_RMS_HOME=$DATA/rms/$NAME
export PHONEME_WINDOW_SCALE=${SCALE:-3}
export PHONEME_SCALE_MODE=${MODE:-fit}
[ -n "${FILTER:-}" ] && export PHONEME_SCALE_FILTER=$FILTER
[ -n "${FPS:-}" ] && export PHONEME_FPS_LIMIT=$FPS
[ -z "${MUTE:-}" ] && export PHONEME_ENABLE_AUDIO=1
if [ -n "${LCD:-}" ]; then
    export PHONEME_LCD_SIZE=$LCD PHONEME_SOURCE_SIZE=$LCD
fi

cd "$HERE/bin"
if [ -n "$APP_CLASS" ]; then
    export PHONEME_HEAP_MB=${HEAP_MB:-32}
    area=$(jam_value "$JAM" DrawArea)
    case "$area" in
        [0-9]*x[0-9]*) ;;
        *) area=240x240 ;;
    esac
    if [ -z "${LCD:-}" ]; then
        export PHONEME_LCD_SIZE=$area PHONEME_SOURCE_SIZE=$area
    fi
    hex=$(od -An -tx1 -v "$JAM" | tr -d ' \n')
    # scratchpad: SP=file, else game.sp (or .SP) next to the jar
    if [ -z "${SP:-}" ]; then
        SP=$BASE.sp
        [ ! -f "$SP" ] && [ -f "$BASE.SP" ] && SP=$BASE.SP
    fi
    # no scratchpad yet (a game without data): an empty one, as the FunKey
    # launcher does; the runtime writes its header
    [ -e "$SP" ] || : > "$SP" 2>/dev/null || true
    pics=""
    if [ -d "$DIR/mypicture" ]; then pics=$DIR/mypicture
    elif [ -d "$BASE.pictures" ]; then pics=$BASE.pictures; fi
    SP=$(ascii_link "$SP" "$NAME.sp")  # temp files are named after it
    [ -n "$pics" ] && pics=$(ascii_link "$pics" pictures)
    VMJAR=$(ascii_link "$JAR" game.jar)
    param=$(jam_value "$JAM" AppParam)
    ver=$(jam_value "$JAM" AppVer)
    spsize=$(jam_value "$JAM" SPsize)
    [ -n "${FKT_PRELOAD:-}" ] && export LD_PRELOAD=$FKT_PRELOAD
    run_vm $VMARGS =MaxMethodToCompile3000 \
        "-Ddoja.app.class=$APP_CLASS" \
        "-Ddoja.app.param=$param" \
        "-Ddoja.app.ver=$ver" \
        -Dmicroedition.encoding=SJIS \
        "-Ddoja.scratchpad.path=$SP" \
        "-Ddoja.spsize=$spsize" \
        "-Ddoja.jam.hex=$hex" \
        "-Ddoja.pictures=$pics" \
        -classpathext "$VMJAR:$HERE/lib/doja-support.jar" \
        internal com.funkey.doja.DoJaMIDlet
    exit $?
fi

export PHONEME_HEAP_MB=${HEAP_MB:-16}
# getAppProperty() reads a .jad next to the jar: use the game's own one,
# otherwise make one from the manifest beside a link to the jar.
RUNJAR=$JAR
if [ -f "$BASE.jad" ] && tr -d '\r' < "$BASE.jad" | grep -aq '^MIDlet-1:' && ! nonascii "$JAR"; then
    DESC=$BASE.jad
elif [ -f "$BASE.jad" ] && tr -d '\r' < "$BASE.jad" | grep -aq '^MIDlet-1:'; then
    cp "$BASE.jad" "$DATA/run/$NAME/game.jad"
    ln -sf "$JAR" "$DATA/run/$NAME/game.jar"
    RUNJAR=$DATA/run/$NAME/game.jar
    DESC=$DATA/run/$NAME/game.jad
else
    manifest "$JAR" > "$DATA/run/$NAME/game.jad"
    ln -sf "$JAR" "$DATA/run/$NAME/game.jar"
    RUNJAR=$DATA/run/$NAME/game.jar
    DESC=$DATA/run/$NAME/game.jad
fi
MAIN=$(tr -d '\r' < "$DESC" | grep -a '^MIDlet-1:' | head -n 1 | awk -F, '{print $3}' | tr -d ' ')
if [ -z "$MAIN" ]; then
    MAIN=$(manifest "$JAR" | grep -a '^MIDlet-1:' |
           head -n 1 | awk -F, '{print $3}' | tr -d ' ')
fi
if [ -z "$MAIN" ]; then
    echo "No MIDlet-1 entry in $JAR" >&2
    exit 1
fi
[ -n "${FKT_PRELOAD:-}" ] && export LD_PRELOAD=$FKT_PRELOAD
run_vm $VMARGS -classpathext "$RUNJAR" internal "$MAIN"
exit $?
