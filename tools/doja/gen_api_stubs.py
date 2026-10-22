#!/usr/bin/env python3
"""Generate CLDC 1.1 skeletons of the DoJa API from an API reference source tree.

The reference is a tree of .java files that declare the public DoJa API
(com/nttdocomo/...). Only the API shape is used: type declarations,
public/protected member signatures and compile-time constants. Bodies are
replaced by defaults that log the first call (com.funkey.doja.Stub.log),
so games link and run, and the log shows which APIs they reach.

usage: gen_api_stubs.py <reference src root> <out dir> [--skip-dir <handwritten root>]...

Files present in a --skip-dir tree (hand-written runtime classes) are not
generated.
"""
import os
import re
import sys

JAVA_LANG = set('''Object String StringBuffer Integer Long Short Byte Character Boolean
Float Double Math System Thread Runnable Class Throwable Exception RuntimeException Error
IllegalArgumentException IllegalStateException NullPointerException
IndexOutOfBoundsException ArrayIndexOutOfBoundsException StringIndexOutOfBoundsException
ArithmeticException ClassCastException SecurityException InterruptedException
NumberFormatException OutOfMemoryError NegativeArraySizeException ClassNotFoundException
InstantiationException IllegalAccessException VirtualMachineError'''.split())
# CLDC 1.1 / MIDP 2.0 types usable from java.io, java.util, javax.microedition.io
CLDC_PKGS = {
    'java.io': set('''InputStream OutputStream IOException EOFException DataInput DataOutput
        DataInputStream DataOutputStream ByteArrayInputStream ByteArrayOutputStream Reader
        Writer InputStreamReader OutputStreamWriter PrintStream InterruptedIOException
        UnsupportedEncodingException UTFDataFormatException'''.split()),
    'java.util': set('''Vector Hashtable Enumeration Calendar Date TimeZone Random Stack
        NoSuchElementException EmptyStackException Timer TimerTask'''.split()),
    'javax.microedition.io': set('''Connection Connector ConnectionNotFoundException
        ContentConnection InputConnection OutputConnection StreamConnection
        StreamConnectionNotifier Datagram DatagramConnection HttpConnection'''.split()),
}
# provided by the phoneME runtime itself
RUNTIME_CLASSES = {'com/nttdocomo/util/JarInflater.java'}
FORBIDDEN_TOKENS = ('<', '...', '@', '->', '::')
MODIFIERS = {'public', 'protected', 'private', 'static', 'final', 'abstract',
             'synchronized', 'native', 'transient', 'volatile', 'strictfp', 'default',
             'sealed', 'non-sealed'}
PRIMS = {'int', 'long', 'short', 'byte', 'char', 'boolean', 'float', 'double', 'void'}


def strip_comments(src):
    out = []
    i = 0
    n = len(src)
    while i < n:
        c = src[i]
        if src.startswith('//', i):
            j = src.find('\n', i)
            i = n if j < 0 else j
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2)
            i = n if j < 0 else j + 2
        elif c == '"' or c == "'":
            j = i + 1
            while j < n and src[j] != c:
                if src[j] == '\\':
                    j += 1
                j += 1
            out.append(src[i:j + 1])
            i = j + 1
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def strip_annotations(s):
    # @Foo, @Foo(...)
    return re.sub(r'@\w+(\s*\([^()]*\))?', ' ', s)


def split_top(body):
    """Split a type body into member chunks at depth 0: returns list of
    (header, kind) where kind is ';' or '{' and header excludes the body."""
    items = []
    i = 0
    n = len(body)
    start = 0
    depth = 0
    paren = 0
    while i < n:
        c = body[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and body[j] != c:
                if body[j] == '\\':
                    j += 1
                j += 1
            i = j + 1
            continue
        if c == '(':
            paren += 1
        elif c == ')':
            paren -= 1
        elif c == '{' and paren == 0:
            header = body[start:i]
            # find matching brace
            d = 1
            j = i + 1
            while j < n and d:
                cj = body[j]
                if cj == '"' or cj == "'":
                    k = j + 1
                    while k < n and body[k] != cj:
                        if body[k] == '\\':
                            k += 1
                        k += 1
                    j = k + 1
                    continue
                if cj == '{':
                    d += 1
                elif cj == '}':
                    d -= 1
                j += 1
            items.append((header.strip(), '{', body[i + 1:j - 1]))
            i = j
            start = i
            continue
        elif c == ';' and paren == 0:
            items.append((body[start:i].strip(), ';', None))
            i += 1
            start = i
            continue
        i += 1
    return items


class TypeDecl:
    pass


def parse_type_header(h):
    h = ' '.join(strip_annotations(h).split())
    m = re.match(r'((?:\w+\s+)*?)(class|interface|enum|record)\s+(\w+)(.*)$', h)
    if not m:
        return None
    t = TypeDecl()
    t.mods = m.group(1).split()
    t.kind = m.group(2)
    t.name = m.group(3)
    rest = m.group(4)
    rest = re.sub(r'\bpermits\b.*$', '', rest)
    t.extends = []
    t.implements = []
    em = re.search(r'\bextends\s+([\w.,\s]+?)(?=\bimplements\b|$)', rest)
    if em:
        t.extends = [x.strip() for x in em.group(1).split(',') if x.strip()]
    im = re.search(r'\bimplements\s+([\w.,\s]+)$', rest)
    if im:
        t.implements = [x.strip() for x in im.group(1).split(',') if x.strip()]
    return t


def type_tokens(sig):
    return set(re.findall(r'\b([A-Z][\w]*)\b', sig))


class Ctx:
    def __init__(self, pkg, known_types, imports):
        self.pkg = pkg
        self.known = known_types  # fq names of generated + handwritten API types
        self.imports = imports


def resolve(name, ctx):
    """Returns the fully qualified name for a simple or dotted type name, or None."""
    if '.' in name:
        if name in ctx.known:
            return name
        for pkg, names in CLDC_PKGS.items():
            if name.startswith(pkg + '.') and name[len(pkg) + 1:] in names:
                return name
        if name.startswith('java.lang.') and name[10:] in JAVA_LANG:
            return name
        return None
    for imp in ctx.imports:
        if imp.endswith('.' + name):
            if imp in ctx.known:
                return imp
            pkg = imp[:-len(name) - 1]
            if pkg in CLDC_PKGS and name in CLDC_PKGS[pkg]:
                return imp
            return None
    if ctx.pkg + '.' + name in ctx.known:
        return ctx.pkg + '.' + name
    for imp in ctx.imports:
        if imp.endswith('.*'):
            pkg = imp[:-2]
            if pkg + '.' + name in ctx.known:
                return pkg + '.' + name
            if pkg in CLDC_PKGS and name in CLDC_PKGS[pkg]:
                return pkg + '.' + name
    if name in JAVA_LANG:
        return 'java.lang.' + name
    return None


def sig_ok(sig, ctx, own_names):
    if any(t in sig for t in FORBIDDEN_TOKENS):
        return False
    for tok in re.findall(r'\b([A-Za-z_][\w.]*)\b', sig):
        if tok in PRIMS or tok in MODIFIERS or tok in ('throws', 'extends'):
            continue
        if not tok[0].isupper() and '.' not in tok:
            continue  # parameter name
        base = tok
        if '.' in tok:
            parts = tok.split('.')
            # Outer.Inner or fq name
            if parts[0][0].islower():
                base = tok
            else:
                base = parts[0]
                if base in own_names:
                    continue
        if base in own_names:
            continue
        if resolve(base, ctx) is None:
            return False
    return True


DEFAULTS = {'int': '0', 'long': '0L', 'short': '(short) 0', 'byte': '(byte) 0',
            'char': "'\\0'", 'boolean': 'false', 'float': '0f', 'double': '0d'}


def default_value(rtype):
    rtype = rtype.strip()
    if rtype in DEFAULTS:
        return DEFAULTS[rtype]
    return 'null'


def is_exception(t, all_types):
    seen = set()
    cur = t
    while cur is not None:
        if not cur.extends:
            return False
        e = cur.extends[0].split('.')[-1]
        if e.endswith('Exception') or e.endswith('Error') or e == 'Throwable':
            return True
        if e in seen:
            return False
        seen.add(e)
        cur = all_types.get(e)
    return False


def gen_type(t, body, ctx, own_names, fq, all_types, indent=''):
    lines = []
    mods = [m for m in t.mods if m in ('public', 'abstract', 'final', 'static', 'protected')]
    kind = 'class' if t.kind in ('enum', 'record') else t.kind
    if t.kind == 'enum':
        return None
    head = ' '.join(mods + [kind, t.name])
    ext = [e for e in t.extends if resolve(e.split('.')[-1] if '.' not in e else e, ctx) or e in own_names]
    impl = [i for i in t.implements if (resolve(i, ctx) or i in own_names) and i != 'AutoCloseable']
    if kind == 'interface':
        if ext:
            head += ' extends ' + ', '.join(ext)
    else:
        if ext:
            head += ' extends ' + ext[0]
        if impl:
            head += ' implements ' + ', '.join(impl)
    lines.append(indent + head + ' {')
    members = split_top(body)
    exc = kind == 'class' and is_exception(t, all_types)
    has_noarg_ctor = False
    ctor_count = 0
    emitted_sigs = set()
    for header, k, mbody in members:
        h = ' '.join(strip_annotations(header).split())
        if not h:
            continue
        # nested types
        nt = re.match(r'((?:\w+\s+)*)(class|interface|enum|record)\s+\w+', h)
        if nt and k == '{':
            sub = parse_type_header(h)
            if sub and ('public' in sub.mods or 'protected' in sub.mods or kind == 'interface') \
                    and sub.kind in ('class', 'interface'):
                if kind == 'interface' and 'static' not in sub.mods:
                    sub.mods = ['public', 'static'] + sub.mods
                sub_lines = gen_type(sub, mbody, ctx, own_names, fq + '$' + sub.name, all_types,
                                     indent + '    ')
                if sub_lines:
                    lines.extend(sub_lines)
            continue
        if h.startswith('static') and k == '{' and '(' not in h:
            continue  # static initializer
        words = h.split()
        visible = ('public' in words or 'protected' in words or kind == 'interface')
        if 'private' in words:
            continue
        if not visible:
            continue
        if '(' in h:
            # method or constructor
            m = re.match(r'^(.*?)\b(\w+)\s*\((.*)\)\s*(throws\s+[\w.,\s]+)?\s*(default\s+.*)?$', h)
            if not m:
                continue
            pre, name, params, throws = m.group(1).strip(), m.group(2), m.group(3), m.group(4)
            if 'default' in pre.split() and kind == 'interface':
                pass
            prewords = pre.split()
            mods_m = [w for w in prewords if w in MODIFIERS]
            rtype = ' '.join(w for w in prewords if w not in MODIFIERS)
            if kind == 'interface':
                mods_m = [w for w in mods_m if w in ('static',)]
                if 'static' in mods_m:
                    continue  # CLDC: no static interface methods
            full = pre + ' ' + name + '(' + params + ')' + (' ' + throws if throws else '')
            if not sig_ok(full, ctx, own_names):
                continue
            ptypes = []
            pnames = []
            if params.strip():
                for p in params.split(','):
                    p = ' '.join(w for w in p.split() if w != 'final')
                    pt = p.rsplit(' ', 1)
                    if len(pt) != 2:
                        break
                    ptypes.append(pt[0].replace(' ', ''))
                    pnames.append(pt[1])
            key = name + '(' + ','.join(ptypes) + ')'
            if key in emitted_sigs:
                continue
            emitted_sigs.add(key)
            plist = ', '.join(a + ' ' + b for a, b in zip(ptypes, pnames))
            thr = (' ' + throws.strip()) if throws else ''
            is_ctor = rtype == '' and name == t.name
            if is_ctor:
                ctor_count += 1
                if not ptypes:
                    has_noarg_ctor = True
                vis = 'public' if 'public' in mods_m else 'protected'
                lines.append(indent + '    %s %s(%s)%s {' % (vis, name, plist, thr))
                if exc:
                    if ptypes == ['String']:
                        lines.append(indent + '        super(%s);' % pnames[0])
                    elif ptypes == ['int']:
                        lines.append(indent + '        this.fkStatus = %s;' % pnames[0])
                    elif ptypes == ['int', 'String']:
                        lines.append(indent + '        super(%s);' % pnames[1])
                        lines.append(indent + '        this.fkStatus = %s;' % pnames[0])
                lines.append(indent + '    }')
                continue
            if not rtype:
                continue
            vis = 'public' if ('public' in mods_m or kind == 'interface') else 'protected'
            mods_out = [vis] + [w for w in mods_m if w in ('static', 'final', 'abstract', 'synchronized')]
            if kind == 'interface':
                mods_out = ['public']
                lines.append(indent + '    %s %s %s(%s)%s;' % (' '.join(mods_out), rtype, name, plist, thr))
                continue
            if 'abstract' in mods_out:
                lines.append(indent + '    %s %s %s(%s)%s;' % (' '.join(mods_out), rtype, name, plist, thr))
                continue
            lines.append(indent + '    %s %s %s(%s)%s {' % (' '.join(mods_out), rtype, name, plist, thr))
            if exc and name == 'getStatus' and rtype == 'int' and not ptypes:
                lines.append(indent + '        return fkStatus;')
            else:
                lines.append(indent + '        com.funkey.doja.Stub.log("%s.%s");' % (fq.split('.')[-1], name))
                if rtype != 'void':
                    lines.append(indent + '        return %s;' % default_value(rtype))
            lines.append(indent + '    }')
        else:
            # field
            if k != ';':
                continue
            fm = re.match(r'^(.*?)\b([\w.\[\]]+)\s+(\w+)\s*(=\s*(.*))?$', h)
            if not fm:
                continue
            pre, ftype, fname, init = fm.group(1), fm.group(2), fm.group(3), fm.group(5)
            prewords = pre.split()
            if kind != 'interface' and not ('static' in prewords and 'final' in prewords):
                # plain public field (e.g. AffineTrans.m00, Vector3D.x)
                if 'public' in prewords and (ftype in PRIMS or ftype.rstrip('[]') in PRIMS
                                             or sig_ok(ftype, ctx, own_names)):
                    mods_f = ['public'] + [w for w in prewords if w in ('static',)]
                    lines.append(indent + '    %s %s %s;' % (' '.join(mods_f), ftype, fname))
                continue
            if init is None:
                continue
            init = init.strip()
            ok_literal = re.match(r'^[-+~()\w.\s|&<>*/"\'\\x]+$', init) and 'new ' not in init \
                and '(' not in init.replace('(int)', '').replace('(short)', '').replace('(byte)', '').replace('(char)', '').replace('(long)', '').replace('(float)', '')
            if not (ftype in PRIMS or ftype == 'String'):
                continue
            if not ok_literal and not (init.startswith('"') and init.endswith('"')):
                continue
            if not sig_ok(ftype + ' ' + init if not init.startswith('"') else ftype, ctx, own_names):
                continue
            if kind == 'interface':
                lines.append(indent + '    %s %s = %s;' % (ftype, fname, init))
            else:
                vis = 'public' if 'public' in prewords else 'protected'
                lines.append(indent + '    %s static final %s %s = %s;' % (vis, ftype, fname, init))
    if kind == 'class':
        if exc:
            lines.append(indent + '    private int fkStatus;')
        if not has_noarg_ctor:
            lines.append(indent + '    protected %s() {' % t.name)
            lines.append(indent + '    }')
    lines.append(indent + '}')
    return lines


def collect(ref_root, prefixes):
    files = []
    for dp, _, fs in os.walk(ref_root):
        for f in fs:
            if f.endswith('.java') and f != 'package-info.java':
                p = os.path.join(dp, f)
                rel = os.path.relpath(p, ref_root)
                if any(rel.startswith(px) for px in prefixes) and not os.path.basename(rel).startswith('_'):
                    files.append(rel)
    return sorted(files)


def main():
    args = sys.argv[1:]
    ref, out = args[0], args[1]
    skip_roots = []
    i = 2
    while i < len(args):
        if args[i] == '--skip-dir':
            skip_roots.append(args[i + 1])
            i += 2
        else:
            i += 1
    files = collect(ref, ['com/nttdocomo/'])
    known = set()
    for rel in files:
        known.add(rel[:-5].replace('/', '.'))
    handwritten = set()
    for root in skip_roots:
        for dp, _, fs in os.walk(root):
            for f in fs:
                if f.endswith('.java'):
                    rel = os.path.relpath(os.path.join(dp, f), root)
                    handwritten.add(rel)
                    known.add(rel[:-5].replace('/', '.'))
    # simple-name index for exception detection
    all_types = {}
    parsed = {}
    for rel in files:
        src = strip_comments(open(os.path.join(ref, rel), encoding='utf-8').read())
        parsed[rel] = src
        m = re.search(r'^\s*((?:public|abstract|final|sealed|non-sealed|\s)*)(class|interface|enum|record)\s+\w+[^{]*', src, re.M)
        if m:
            t = parse_type_header(m.group(0))
            if t:
                all_types[t.name] = t
    count = 0
    skipped = []
    for rel in files:
        if rel in handwritten or rel in RUNTIME_CLASSES:
            continue
        src = parsed[rel]
        pkg_m = re.search(r'^\s*package\s+([\w.]+)\s*;', src, re.M)
        pkg = pkg_m.group(1)
        imports = re.findall(r'^\s*import\s+([\w.*]+)\s*;', src, re.M)
        m = re.search(r'^\s*((?:public|abstract|final|sealed|non-sealed|\s)*)(class|interface|enum|record)\s+(\w+)[^{]*\{', src, re.M)
        if not m:
            skipped.append(rel)
            continue
        t = parse_type_header(m.group(0)[:-1])
        if t is None or 'public' not in t.mods or t.kind == 'enum':
            skipped.append(rel)
            continue
        # body: from the brace to the matching end
        start = m.end()
        depth = 1
        j = start
        while j < len(src) and depth:
            c = src[j]
            if c in '"\'':
                k = j + 1
                while k < len(src) and src[k] != c:
                    if src[k] == '\\':
                        k += 1
                    k += 1
                j = k + 1
                continue
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
            j += 1
        body = src[start:j - 1]
        own = {t.name}
        for sm in re.finditer(r'\b(class|interface)\s+(\w+)', body):
            own.add(sm.group(2))
        keep_imports = []
        for imp in imports:
            if imp.endswith('.*'):
                base = imp[:-2]
                if base in CLDC_PKGS or any(k.startswith(base + '.') for k in known):
                    keep_imports.append(imp)
                continue
            base, simple = imp.rsplit('.', 1)
            if imp in known or (base in CLDC_PKGS and simple in CLDC_PKGS[base]):
                keep_imports.append(imp)
        ctx = Ctx(pkg, known, keep_imports)
        lines = gen_type(t, body, ctx, own, pkg + '.' + t.name, all_types)
        if not lines:
            skipped.append(rel)
            continue
        text = 'package %s;\n\n' % pkg
        for imp in keep_imports:
            text += 'import %s;\n' % imp
        text += '\n/* generated by tools/doja/gen_api_stubs.py */\n'
        text += '\n'.join(lines) + '\n'
        op = os.path.join(out, rel)
        os.makedirs(os.path.dirname(op), exist_ok=True)
        open(op, 'w').write(text)
        count += 1
    print('generated', count, 'skipped', len(skipped))
    for s in skipped:
        print('  skipped', s)


main()
