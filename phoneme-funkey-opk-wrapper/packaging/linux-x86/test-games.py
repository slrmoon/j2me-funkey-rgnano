#!/usr/bin/env python3
"""Run many J2ME / DoJa games one after another (several at a time) with
the desktop runtime, press OK and 5 now and then, and keep each game's log
and screenshots.

    ./test-games.py '/games/*/*.jar' -o results
    ./test-games.py /games -j 8 -t 20 --speed 3

Arguments: jar files, folders (searched recursively) or glob patterns.
Results go to the output folder:

    results/index.html      thumbnails of every game, colored by status
    results/summary.csv     one line per game
    results/<game>/log.txt  console output of the runtime
    results/<game>/*.png    screenshots (BMP when Pillow is not installed)

Status: OK (still running at the end, no errors in the log), WARN (still
running, but exceptions in the log or the screen stayed blank), FAIL (the
game stopped by itself: missing class, crash, ...).

Needs lib/fktest.so from the runtime package (it injects the keys, takes
the screenshots and speeds up the clock). Games run without a window
(SDL dummy driver) unless --show is given.
"""
import argparse
import csv
import glob
import html
import os
import re
import shutil
import signal
import subprocess
import sys
import threading
import struct
import time
import zipfile
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))

# SDL 1.2 key symbols
KEYS = {
    'ok': 13, 'enter': 13, 'fire': 13, 'space': 32,
    'soft1': 9, 'left_soft': 9, 'tab': 9, 'soft2': 27, 'right_soft': 27, 'esc': 27,
    'up': 273, 'down': 274, 'right': 275, 'left': 276,
    'star': 42, '*': 42, 'pound': 35, '#': 35,
}
for d in range(10):
    KEYS[str(d)] = 48 + d

ERROR_PATTERNS = [
    ('missing class', re.compile(r'NoClassDefFoundError|ClassNotFoundException')),
    ('out of memory', re.compile(r'OutOfMemoryError')),
    ('cannot run', re.compile(r'could not be run|cannot be launched|MIDlet suite could not')),
    ('start failed', re.compile(r'DoJa: application start failed|fatal error screen')),
    ('exception', re.compile(r'(\w+\.)*\w+(Exception|Error)\b')),
    ('m3g error', re.compile(r'\[native\] Error')),
]
IGNORE = re.compile(r'\[inject\]|ld\.so|ERROR: ld\.so')


def find_games(args):
    jars = []
    for a in args:
        if os.path.isdir(a):
            for root, _, files in os.walk(a):
                jars += [os.path.join(root, f) for f in files if f.lower().endswith('.jar')]
        elif any(c in a for c in '*?['):
            # .jar in any letter case: game dumps often have .JAR
            pat = re.sub(r'\.jar$', '.[jJ][aA][rR]', a, flags=re.I)
            found = [p for p in glob.glob(pat, recursive=True) if p.lower().endswith('.jar')]
            if not found:
                top = a.split('*')[0].split('?')[0].split('[')[0]
                top = top if os.path.isdir(top) else os.path.dirname(top)
                print('nothing matches %s%s' % (a, '' if not top or os.path.isdir(top)
                                                else ' (no folder %s)' % top), file=sys.stderr)
            jars += found
        elif os.path.isfile(a):
            jars.append(a)
        else:
            print('not found: %s (Windows D:\\games is /mnt/d/games in WSL)' % a, file=sys.stderr)
    seen = set()
    out = []
    for j in sorted(jars):
        p = os.path.abspath(j)
        if p not in seen:
            seen.add(p)
            out.append(p)
    return out


def jam_value(jam, key):
    """one key of a .jam (Shift_JIS text, key = value)"""
    try:
        with open(jam, 'rb') as f:
            for line in f.read().decode('shift_jis', 'replace').splitlines():
                k, _, v = line.partition('=')
                if k.strip().lower() == key.lower():
                    return v.strip()
    except OSError:
        pass
    return ''


def jar_for_jam(jam):
    """the jar of an i-appli: same name (any case), PackageURL, or the only
    jar of the folder"""
    d = os.path.dirname(jam)
    stem = os.path.splitext(os.path.basename(jam))[0].lower()
    jars = [f for f in os.listdir(d) if f.lower().endswith('.jar')]
    for f in jars:
        if os.path.splitext(f)[0].lower() == stem:
            return os.path.join(d, f)
    url = jam_value(jam, 'PackageURL').replace('\\', '/').split('/')[-1].lower()
    for f in jars:
        if url and f.lower() == url:
            return os.path.join(d, f)
    if len(jars) == 1:  # only when it holds the AppClass of this .jam
        cls = jam_value(jam, 'AppClass').replace('.', '/') + '.class'
        try:
            if cls in zipfile.ZipFile(os.path.join(d, jars[0])).namelist():
                return os.path.join(d, jars[0])
        except Exception:
            pass
    return None


def super_class(jar, cls):
    """the superclass name of a class in a jar, or None"""
    try:
        d = zipfile.ZipFile(jar).read(cls.replace('.', '/') + '.class')
        n = struct.unpack('>H', d[8:10])[0]
        cp = [None] * n
        i = 10
        k = 1
        while k < n:
            t = d[i]
            if t == 1:
                ln = struct.unpack('>H', d[i + 1:i + 3])[0]
                cp[k] = d[i + 3:i + 3 + ln].decode('utf-8', 'replace')
                i += 3 + ln
            elif t in (3, 4, 9, 10, 11, 12):
                i += 5
            elif t in (5, 6):
                i += 9
                k += 1
            elif t == 7:
                cp[k] = struct.unpack('>H', d[i + 1:i + 3])[0]
                i += 3
            elif t == 8:
                i += 3
            else:
                return None
            k += 1
        return cp[cp[struct.unpack('>H', d[i + 4:i + 6])[0]]]
    except Exception:
        return None


def find_doja(args):
    """(jar, jam, name) for every .jam under the arguments"""
    out = []
    seen = set()
    for a in args:
        if os.path.isdir(a):
            top = os.path.abspath(a)
            jams = [os.path.join(r, f) for r, _, fs in os.walk(a) for f in fs
                    if f.lower().endswith('.jam')]
        else:
            jams = [p for p in glob.glob(a, recursive=True) if p.lower().endswith('.jam')]
            if os.path.isfile(a) and a.lower().endswith('.jam'):
                jams = [a]
            top = None
            if not jams:
                print('nothing matches %s' % a, file=sys.stderr)
        for jam in sorted(jams):
            jam = os.path.abspath(jam)
            if jam in seen:
                continue
            seen.add(jam)
            rel = os.path.relpath(jam, top) if top else os.path.join(
                os.path.basename(os.path.dirname(jam)), os.path.basename(jam))
            out.append((jar_for_jam(jam), jam, os.path.splitext(rel)[0]))
    return out


def result_name(jar, used, lock, label=None):
    """folder name: <parent>_<jar> (DoJa: the .jam path), unique"""
    if label:
        base = label.replace(os.sep, '_')
    else:
        parent = os.path.basename(os.path.dirname(jar))
        stem = os.path.splitext(os.path.basename(jar))[0]
        base = (parent + '_' + stem) if parent else stem
    base = re.sub(r'[^\w.-]+', '_', base).strip('_')[:120] or 'game'
    with lock:
        name = base
        n = 2
        while name in used:
            name = '%s_%d' % (base, n)
            n += 1
        used.add(name)
    return name


def key_script(keys, start_ms, every_ms, total_ms, hold_ms=120):
    """press the keys in turn from start_ms, every every_ms (game time)"""
    ev = []
    t = start_ms
    i = 0
    while t < total_ms and len(ev) < 1000:
        sym = keys[i % len(keys)]
        ev.append('%d:%d:d' % (t, sym))
        ev.append('%d:%d:u' % (t + hold_ms, sym))
        t += every_ms
        i += 1
    return ','.join(ev)


def to_png(folder):
    """BMP -> PNG when Pillow is there; returns the image file names"""
    try:
        from PIL import Image
    except ImportError:
        Image = None
    names = []
    for f in sorted(os.listdir(folder)):
        if not f.endswith('.bmp'):
            continue
        p = os.path.join(folder, f)
        if Image is not None:
            try:
                q = p[:-4] + '.png'
                Image.open(p).save(q, optimize=True)
                os.remove(p)
                f = os.path.basename(q)
            except Exception:
                pass
        names.append(f)
    return names


def blank_image(path):
    """True when the picture is (nearly) one color"""
    try:
        from PIL import Image
    except ImportError:
        return None
    try:
        im = Image.open(path).convert('L').resize((64, 64))
        lo, hi = im.getextrema()
        return hi - lo < 8
    except Exception:
        return None


def run_game(game, opt, used, lock, counter):
    jar, jam, label = game
    name = result_name(jar or jam, used, lock, label)
    out = os.path.join(opt.output, name)
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    data = os.path.join(out, 'data')

    game_ms = int(opt.time * opt.speed * 1000)
    env = dict(os.environ)
    env.update({
        'FKT_PRELOAD': opt.shim,  # run.sh sets LD_PRELOAD for the VM only
        'FKT_KEYS': key_script(opt.keys, int(opt.first_key * 1000), int(opt.key_every * 1000), game_ms),
        'FKT_SHOT_DIR': out,
        # first shot at once, the last one shortly before the end
        'FKT_SHOT_MS': str(max(200, int(game_ms * 0.93) // max(1, opt.shots - 1))),
        'FKT_SPEED': str(opt.speed),
        'XDG_DATA_HOME': data,  # fresh saves for each run
        'SCALE': str(opt.scale),
        'MUTE': '1',
        'SDL_AUDIODRIVER': 'dummy',
    })
    if not opt.show:
        env['SDL_VIDEODRIVER'] = 'dummy'
    if opt.lcd:
        env['LCD'] = opt.lcd
    if opt.fps:
        env['FPS'] = str(opt.fps)
    env.pop('JAM', None)
    env.pop('SP', None)
    if jam:
        # the game's own scratchpad is copied: test runs never write to it
        os.makedirs(data, exist_ok=True)
        # named after the result folder: the runtime names its /tmp files
        # after the scratchpad, and parallel games must not share them
        sp = os.path.join(data, name + '.sp')
        stem = os.path.splitext(jar or jam)[0]
        for src in (stem + '.sp', stem + '.SP', os.path.splitext(jam)[0] + '.sp'):
            if os.path.isfile(src):
                shutil.copyfile(src, sp)
                break
        else:
            open(sp, 'wb').close()  # a new game: the runtime fills in the header
        env['JAM'] = jam
        env['SP'] = sp

    log_path = os.path.join(out, 'log.txt')
    t_start = time.time()
    skip = None
    if jam and jar is None:
        skip = ('no jar', 'no .jar for this .jam')
    elif jam:
        sup = super_class(jar, jam_value(jam, 'AppClass')) or ''
        if sup.startswith('com/docomostar/'):
            # docomo STAR, the API after DoJa: not implemented
            skip = ('STAR app', 'docomo STAR application (%s), not supported' % sup)
    if skip:
        with open(log_path, 'w') as log:
            log.write('# %s\n%s\n' % (jam, skip[1]))
        with lock:
            counter[0] += 1
            n = counter[0]
        print('[%d/%d] SKIP   0.0s  %s  %s' % (n, counter[1], name, skip[0]), flush=True)
        return {'name': name, 'jar': jam, 'status': 'SKIP', 'seconds': 0, 'exit_code': '',
                'problems': skip[0], 'first_error': skip[1],
                'screenshots': 0, 'last': '', 'shots': []}
    with open(log_path, 'wb') as log:
        log.write(('# %s\n' % (jam or jar)).encode())
        log.flush()
        p = subprocess.Popen([opt.run_sh, jar], cwd=os.path.dirname(opt.run_sh), env=env,
                             stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            rc = p.wait(timeout=opt.time)
            stopped = True
        except subprocess.TimeoutExpired:
            stopped = False
            try:
                os.killpg(p.pid, signal.SIGTERM)  # the shim saves last.bmp
                rc = p.wait(timeout=3)
            except (subprocess.TimeoutExpired, ProcessLookupError):
                try:
                    os.killpg(p.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                rc = p.wait()
    seconds = time.time() - t_start
    if not opt.keep_data:
        shutil.rmtree(data, ignore_errors=True)

    images = to_png(out)
    shots = [f for f in images if f.startswith('shot_')]
    last = next((f for f in images if f.startswith('last.')), shots[-1] if shots else '')

    with open(log_path, 'rb') as f:
        text = f.read().decode('utf-8', 'replace')
    lines = [l for l in text.splitlines() if not IGNORE.search(l)]
    problems = []
    first_error = ''
    for label, rx in ERROR_PATTERNS:
        for l in lines:
            if rx.search(l):
                if label not in problems:
                    problems.append(label)
                if not first_error:
                    first_error = l.strip()[:200]
                break

    blank = blank_image(os.path.join(out, last)) if last else True
    if stopped:
        status = 'FAIL'
    elif problems or blank:
        status = 'WARN'
    else:
        status = 'OK'
    if stopped and not first_error:
        first_error = 'exited, code %s' % rc
    if blank and not problems:
        problems.append('blank screen' if last else 'no screenshot')

    with lock:
        counter[0] += 1
        n = counter[0]
    print('[%d/%d] %-4s %5.1fs  %s  %s' % (n, counter[1], status, seconds, name,
                                         ', '.join(problems)), flush=True)
    return {
        'name': name, 'jar': jam or jar, 'status': status, 'seconds': round(seconds, 1),
        'exit_code': rc if stopped else '', 'problems': ', '.join(problems),
        'first_error': first_error, 'screenshots': len(shots), 'last': last,
        'shots': shots,
    }


def write_report(opt, results):
    order = {'FAIL': 0, 'WARN': 1, 'OK': 2, 'SKIP': 3}
    results.sort(key=lambda r: (order[r['status']], r['name'].lower()))
    with open(os.path.join(opt.output, 'summary.csv'), 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        cols = ['name', 'status', 'seconds', 'exit_code', 'problems', 'first_error', 'screenshots', 'jar']
        w.writerow(cols)
        for r in results:
            w.writerow([r[c] for c in cols])

    counts = {s: sum(1 for r in results if r['status'] == s) for s in order}
    cards = []
    for r in results:
        e = html.escape
        thumbs = ''.join('<img src="%s/%s" loading="lazy">' % (e(r['name']), e(s))
                         for s in r['shots'][-3:])
        last = ('<img class="big" src="%s/%s" loading="lazy">' % (e(r['name']), e(r['last']))
                if r['last'] else '<div class="none">no picture</div>')
        cards.append(
            '<div class="card %s" data-s="%s"><div class="hd"><b>%s</b><span>%s</span></div>'
            '%s<div class="th">%s</div><div class="pr">%s</div><div class="er">%s</div>'
            '<a href="%s/log.txt">log</a></div>' % (
                r['status'].lower(), r['status'], e(r['name']), r['status'], last, thumbs,
                e(r['problems']), e(r['first_error']), e(r['name'])))
    page = """<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width">
<title>Game test results</title><style>
body{font:14px system-ui,sans-serif;margin:16px;background:#f4f4f2;color:#222}
.bar button{margin-right:6px;padding:4px 10px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:12px;margin-top:12px}
.card{background:#fff;border-radius:6px;padding:8px;border-top:4px solid #999;overflow:hidden}
.card.skip{border-color:#888}.card.ok{border-color:#3a3}.card.warn{border-color:#d90}.card.fail{border-color:#c33}
.hd{display:flex;justify-content:space-between;gap:6px;word-break:break-all}
.big{width:100%;image-rendering:pixelated;background:#000;margin:6px 0}
.th img{width:32%;margin-right:1%;image-rendering:pixelated}
.pr{color:#a60}.er{font:12px monospace;color:#c33;word-break:break-all}
.none{height:120px;background:#000;color:#888;display:flex;align-items:center;justify-content:center;margin:6px 0}
</style></head><body><h2>Game test results</h2>
<div class="bar">@N@ games: <b style="color:#3a3">@OK@ OK</b>, <b style="color:#d90">@WARN@ WARN</b>,
<b style="color:#c33">@FAIL@ FAIL</b>, @SKIP@ SKIP &nbsp; @TIME@ s per game, speed x@SPEED@ &nbsp;
<button onclick="f('')">all</button><button onclick="f('FAIL')">FAIL</button>
<button onclick="f('WARN')">WARN</button><button onclick="f('OK')">OK</button><button onclick="f('SKIP')">SKIP</button></div>
<div class="grid">@CARDS@</div><script>
function f(s){document.querySelectorAll('.card').forEach(c=>c.style.display=!s||c.dataset.s==s?'':'none')}
</script></body></html>"""
    for k, v in (('@N@', len(results)), ('@OK@', counts['OK']), ('@WARN@', counts['WARN']),
                 ('@FAIL@', counts['FAIL']), ('@SKIP@', counts['SKIP']), ('@TIME@', '%g' % opt.time),
                 ('@SPEED@', '%g' % opt.speed), ('@CARDS@', '\n'.join(cards))):
        page = page.replace(k, str(v))
    with open(os.path.join(opt.output, 'index.html'), 'w', encoding='utf-8') as f:
        f.write(page)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter,
                                 epilog=__doc__.split('\n\n', 1)[1])
    ap.add_argument('games', nargs='+', help="jars, folders or patterns such as '/games/*/*.jar'")
    ap.add_argument('-o', '--output', default='test-results', help='result folder (default test-results)')
    ap.add_argument('-j', '--jobs', type=int, default=max(1, (os.cpu_count() or 2) // 2),
                    help='games at a time (default: half the CPU threads)')
    ap.add_argument('-t', '--time', type=float, default=15, help='seconds per game, 5..120 (default 15)')
    ap.add_argument('--speed', type=float, default=2,
                    help='game clock speed-up, 1 = real time (default 2)')
    ap.add_argument('--keys', default='ok,5,soft1',
                    help='keys pressed in turn: ok 0-9 * # up down left right soft1 soft2 '
                         '(default ok,5,soft1: soft1 answers "sound on?" screens)')
    ap.add_argument('--first-key', type=float, default=3, help='game seconds before the first press (default 3)')
    ap.add_argument('--key-every', type=float, default=1.5, help='game seconds between presses (default 1.5)')
    ap.add_argument('--shots', type=int, default=6, help='screenshots per game (default 6)')
    ap.add_argument('--scale', type=int, default=1, help='screenshot size: 240 x N (default 1)')
    ap.add_argument('--lcd', help='screen size the games see, e.g. 240x320')
    ap.add_argument('--fps', type=int, help='frame limit')
    ap.add_argument('--show', action='store_true', help='open windows instead of running hidden')
    ap.add_argument('--keep-data', action='store_true', help='keep the saves (data/) of each run')
    ap.add_argument('--doja', action='store_true',
                    help='i-appli (DoJa) games: every .jam with its jar (test-doja.py sets this)')
    ap.add_argument('--runtime', default=HERE, help='runtime folder with run.sh (default: this folder)')
    opt = ap.parse_args()

    opt.time = min(max(opt.time, 5), 120)
    opt.run_sh = os.path.join(os.path.abspath(opt.runtime), 'run.sh')
    opt.shim = os.path.join(os.path.abspath(opt.runtime), 'lib', 'fktest.so')
    for f in (opt.run_sh, opt.shim):
        if not os.path.isfile(f):
            sys.exit('missing %s (use --runtime <runtime folder>)' % f)
    try:
        opt.keys = [KEYS[k.strip().lower()] for k in opt.keys.split(',') if k.strip()]
    except KeyError as e:
        sys.exit('unknown key %s; known: %s' % (e, ' '.join(sorted(KEYS))))
    if not opt.keys:
        opt.keys = [13]
    opt.output = os.path.abspath(opt.output)
    os.makedirs(opt.output, exist_ok=True)

    if opt.doja:
        games = find_doja(opt.games)
        if not games:
            sys.exit('no .jam files found')
    else:
        games = [(j, None, None) for j in find_games(opt.games)]
        if not games:
            sys.exit('no .jar files found')
    print('%d games, %d at a time, %gs each (speed x%g) -> %s' % (
        len(games), opt.jobs, opt.time, opt.speed, opt.output), flush=True)

    used = set()
    lock = threading.Lock()
    counter = [0, len(games)]
    results = []
    try:
        with ThreadPoolExecutor(max_workers=opt.jobs) as pool:
            futures = [pool.submit(run_game, g, opt, used, lock, counter) for g in games]
            for fu in futures:
                try:
                    results.append(fu.result())
                except Exception as e:
                    print('error: %s' % e, file=sys.stderr)
    except KeyboardInterrupt:
        print('interrupted, writing the report for finished games', file=sys.stderr)
    write_report(opt, results)
    counts = {s: sum(1 for r in results if r['status'] == s) for s in ('OK', 'WARN', 'FAIL', 'SKIP')}
    print('OK %(OK)d  WARN %(WARN)d  FAIL %(FAIL)d  SKIP %(SKIP)d' % counts)
    print('report: ' + os.path.join(opt.output, 'index.html'))


if __name__ == '__main__':
    main()
