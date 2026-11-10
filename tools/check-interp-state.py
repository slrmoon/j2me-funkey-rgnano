#!/usr/bin/env python3
"""Check the INTERP_LOCAL_STATE rule of the C interpreter.

runtime/cldc/src/vm/cpu/c/Interpreter_c.cpp: the "fast" bytecode handlers
(BYTECODE_FAST) and the state-passing helpers (IS_INLINE / IS_FN) get the
interpreter state as local copies. They must not call code that reads or
writes the global state unless the call is wrapped in VM_CALL(...), which
writes the locals out first and reads them back afterwards.

    tools/check-interp-state.py [Interpreter_c.cpp]

Exit status 1 and a list of offending lines when the rule is broken.
"""
import re
import sys

PATH = sys.argv[1] if len(sys.argv) > 1 else \
    'runtime/cldc/src/vm/cpu/c/Interpreter_c.cpp'

# functions that work on the global state (g_jfp, g_jsp, g_jpc, g_jlocals)
GLOBAL_USERS = re.compile(r'''\b(
    interpreter_call_vm\w* | shared_call_vm_internal | shared_entry |
    invoke_java_method | return_internal |
    interpreter_method_entry | quick_native_method_entry | resume_thread |
    interpreter_throw_\w+ | lock_object | unlock_object | monitor_\w+_internal |
    lock_synchronized_method_internal | unlock_\w+ | allocate_monitor |
    interpreter_grow_stack_internal | bc_impl_\w+ | interpreter_dispatch_table |
    method_transition | timer_tick | update_java_pointers
)\s*[\(\[]''', re.X)


def strip_vm_calls(text):
    """Remove the arguments of every VM_CALL(...) (balanced parentheses)."""
    out, i = [], 0
    while True:
        j = text.find('VM_CALL(', i)
        if j < 0:
            out.append(text[i:])
            return ''.join(out)
        out.append(text[i:j])
        depth, k = 0, j + len('VM_CALL')
        while k < len(text):
            if text[k] == '(':
                depth += 1
            elif text[k] == ')':
                depth -= 1
                if depth == 0:
                    break
            k += 1
        out.append('VM_CALL(...)' + '\n' * text.count('\n', j, k))
        i = k + 1


def bodies(src):
    """Yield (name, first line, body) of fast handlers and IS_ helpers."""
    for m in re.finditer(r'BYTECODE_FAST\((\w+)\)(.*?)BYTECODE_IMPL_END',
                         src, re.S):
        yield m.group(1), src.count('\n', 0, m.start()) + 1, m.group(2)
    for m in re.finditer(r'^\s*IS_(?:INLINE|FN)\s+[\w\s\*]+?\b(\w+)\s*\(', src,
                         re.M):
        start = src.index('{', m.end())
        if src.rfind(';', m.end(), start) >= 0:
            continue  # a declaration
        depth, k = 0, start
        while True:
            if src[k] == '{':
                depth += 1
            elif src[k] == '}':
                depth -= 1
                if depth == 0:
                    break
            k += 1
        yield m.group(1), src.count('\n', 0, start) + 1, src[start:k]


def main():
    src = open(PATH).read()
    # the macro definitions themselves are not code to check
    errors = 0
    for name, line, body in bodies(src):
        if name.startswith('bc_run_'):
            continue  # the generated wrappers
        clean = strip_vm_calls(body)
        for n, text in enumerate(clean.split('\n')):
            m = GLOBAL_USERS.search(text)
            if m and not text.lstrip().startswith(('//', '*', '#define')):
                print('%s:%d: %s: %s() works on the global state; wrap it '
                      'in VM_CALL(...)' % (PATH, line + n, name, m.group(1)))
                errors += 1
    if errors:
        sys.exit(1)
    print('ok')


if __name__ == '__main__':
    main()
