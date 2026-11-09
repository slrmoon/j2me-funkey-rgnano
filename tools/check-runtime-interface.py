#!/usr/bin/env python3
"""Checks that the runtime's launcher interface is listed and documented.

- every PhoneMEConfig("NAME") / FKA_CONFIG("NAME") in runtime/ is in the
  table of runtime/midp/src/core/phoneme_config/reference/native/phoneme_config.c
- every name of the table is used somewhere and documented in
  docs/runtime-interface.md
- our runtime modules do not call getenv() directly

Run from anywhere: python3 tools/check-runtime-interface.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RUNTIME = os.path.join(ROOT, 'runtime')
TABLE = os.path.join(RUNTIME, 'midp/src/core/phoneme_config/reference/native/phoneme_config.c')
DOC = os.path.join(ROOT, 'docs', 'runtime-interface.md')

# our native code (the rest of runtime/ is upstream phoneME)
OURS = [
    'midp/src/highlevelui/lfjport/sdl',
    'midp/src/events/mastermode_port/sdl',
    'midp/src/media-sdl/reference/native/fkaudio',
    'midp/src/ams/example/jams/native/runMidlet.c',
    'midp/src/ams/suitestore/recordstore_api/reference/native/suitestore_rms.c',
    'midp/src/ams/example/ams_common_port/default/native/commandLineUtil_md.c',
    'jsr184/src/native/funkey',
]
# names passed through a variable (EnvKey("J2ME_GP2X_...") in midp_msgQueue_md.c)
INDIRECT = re.compile(r'EnvKey\("([A-Z0-9_]+)"')


def sources(paths):
    for p in paths:
        full = os.path.join(RUNTIME, p)
        if os.path.isfile(full):
            yield full
            continue
        for d, _, files in os.walk(full):
            for f in files:
                if f.endswith(('.c', '.h', '.cpp')):
                    yield os.path.join(d, f)


def main():
    errors = []
    table = set(re.findall(r'\{\s*"([A-Z0-9_]+)",', open(TABLE).read()))
    used = set()
    for path in sources(['midp/src', 'jsr184/src', 'jsr120/src']):
        text = open(path, encoding='utf-8', errors='replace').read()
        used |= set(re.findall(r'(?:PhoneMEConfig(?:IsSet|Int)?|FKA_CONFIG)\("([A-Z0-9_]+)"', text))
        used |= set(INDIRECT.findall(text))
    for path in sources(OURS):
        rel = os.path.relpath(path, ROOT)
        for n, line in enumerate(open(path, encoding='utf-8', errors='replace'), 1):
            code = line.strip()
            if code.startswith(('*', '/*', '//')):
                continue
            if re.search(r'\bgetenv\s*\(', code) and 'FKA_CONFIG(name) getenv' not in code:
                errors.append('%s:%d: getenv() instead of PhoneMEConfig()' % (rel, n))
    doc = open(DOC, encoding='utf-8').read()
    documented = set(re.findall(r'`([A-Z][A-Z0-9_]{3,})`', doc))
    for name in sorted(used - table):
        errors.append('%s is read by the runtime but not listed in phoneme_config.c' % name)
    for name in sorted(table - used):
        errors.append('%s is listed in phoneme_config.c but never read' % name)
    for name in sorted(table - documented):
        errors.append('%s is not documented in docs/runtime-interface.md' % name)
    if errors:
        print('\n'.join(errors))
        return 1
    print('runtime interface: %d settings, all listed and documented' % len(table))
    return 0


if __name__ == '__main__':
    sys.exit(main())
