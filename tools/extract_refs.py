#!/usr/bin/env python3
"""Extract external class/method/field references from game .jar files.

Parses .class constant pools, filters for target DoJa packages,
and compares with the symbols already present in doja-support.jar.
"""

import sys
import os
import zipfile
import struct
import argparse
from collections import defaultdict

TARGET_PACKAGES = [
    'com/nttdocomo/',
    'com/j_phone/',
    'com/docomostar/',
    'com/mascotcapsule/',
    'com/sonyericsson/',
    # javax.microedition is partly covered by phoneME — collect but flag
    'javax/microedition/',
]

KNOWN_INNER_CLASS_SEPARATORS = {'$', '#'}


def read_u1(data, pos):
    return data[pos], pos + 1


def read_u2(data, pos):
    return struct.unpack_from('>H', data, pos)[0], pos + 2


def read_u4(data, pos):
    return struct.unpack_from('>I', data, pos)[0], pos + 4


def parse_constant_pool(data):
    """Parse constant pool from class file data.
    Returns dict: cp_index -> {'tag': N, 'info': ...}
    """
    cp = {}
    pos = 8  # skip magic(4) + minor(2) + major(2)
    cp_count, pos = read_u2(data, pos)
    i = 1
    while i < cp_count:
        tag, pos = read_u1(data, pos)
        if tag == 1:  # CONSTANT_Utf8
            length, pos = read_u2(data, pos)
            value = data[pos:pos+length].decode('utf-8', errors='replace')
            pos += length
            cp[i] = {'tag': tag, 'value': value}
        elif tag == 3:  # CONSTANT_Integer
            val, pos = read_u4(data, pos)
            cp[i] = {'tag': tag, 'value': val}
        elif tag == 4:  # CONSTANT_Float
            val, pos = read_u4(data, pos)
            cp[i] = {'tag': tag, 'value': val}
        elif tag == 5:  # CONSTANT_Long
            val = (struct.unpack_from('>I', data, pos)[0] << 32) | struct.unpack_from('>I', data, pos+4)[0]
            pos += 8
            cp[i] = {'tag': tag, 'value': val}
            i += 1  # long takes 2 slots
        elif tag == 6:  # CONSTANT_Double
            # double takes 8 bytes
            pos += 8
            cp[i] = {'tag': tag}
            i += 1
        elif tag == 7:  # CONSTANT_Class
            name_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag, 'name_index': name_index}
        elif tag == 8:  # CONSTANT_String
            string_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag, 'string_index': string_index}
        elif tag == 9:  # CONSTANT_Fieldref
            class_index, pos = read_u2(data, pos)
            nat_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag, 'class_index': class_index, 'name_and_type_index': nat_index}
        elif tag == 10:  # CONSTANT_Methodref
            class_index, pos = read_u2(data, pos)
            nat_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag, 'class_index': class_index, 'name_and_type_index': nat_index}
        elif tag == 11:  # CONSTANT_InterfaceMethodref
            class_index, pos = read_u2(data, pos)
            nat_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag, 'class_index': class_index, 'name_and_type_index': nat_index}
        elif tag == 12:  # CONSTANT_NameAndType
            name_index, pos = read_u2(data, pos)
            desc_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag, 'name_index': name_index, 'descriptor_index': desc_index}
        elif tag == 15:  # CONSTANT_MethodHandle
            ref_kind, pos = read_u1(data, pos)
            ref_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag}
        elif tag == 16:  # CONSTANT_MethodType
            desc_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag}
        elif tag == 17:  # CONSTANT_Dynamic
            pos += 4
            cp[i] = {'tag': tag}
        elif tag == 18:  # CONSTANT_InvokeDynamic
            pos += 4
            cp[i] = {'tag': tag}
        elif tag == 19:  # CONSTANT_Module
            name_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag}
        elif tag == 20:  # CONSTANT_Package
            name_index, pos = read_u2(data, pos)
            cp[i] = {'tag': tag}
        else:
            # Unknown tag, can't continue
            raise ValueError(f"Unknown constant pool tag {tag} at index {i}")
        i += 1
    return cp


def resolve_utf8(cp, index):
    """Get UTF8 value from constant pool by index."""
    entry = cp.get(index)
    if entry and entry['tag'] == 1:
        return entry['value']
    return None


def resolve_class_name(cp, index):
    """Resolve a CONSTANT_Class reference to a class name."""
    entry = cp.get(index)
    if not entry or entry['tag'] != 7:
        return None
    return resolve_utf8(cp, entry['name_index'])


def resolve_ref(cp, entry):
    """Resolve a methodref/fieldref/interfacemethodref to (class_name, name, descriptor)."""
    class_name = resolve_class_name(cp, entry['class_index'])
    nat = cp.get(entry['name_and_type_index'])
    if not nat or nat['tag'] != 12:
        return class_name, None, None
    name = resolve_utf8(cp, nat['name_index'])
    desc = resolve_utf8(cp, nat['descriptor_index'])
    return class_name, name, desc


def extract_references_from_class(class_data):
    """Extract external references from a single .class file."""
    try:
        cp = parse_constant_pool(class_data)
    except (ValueError, struct.error, IndexError):
        return set(), set(), set()

    classes = set()
    methods = set()
    fields = set()

    for idx, entry in cp.items():
        if entry['tag'] == 7:  # CONSTANT_Class
            name = resolve_class_name(cp, idx)
            if name:
                # Skip arrays of primitives
                if not name.startswith('[') or (len(name) > 1 and name[1] == 'L'):
                    classes.add(_clean_array(name))

        elif entry['tag'] == 9:  # CONSTANT_Fieldref
            cls, name, desc = resolve_ref(cp, entry)
            if cls and name:
                cls = _clean_array(cls)
                fields.add((cls, name, desc or ''))

        elif entry['tag'] == 10:  # CONSTANT_Methodref
            cls, name, desc = resolve_ref(cp, entry)
            if cls and name:
                cls = _clean_array(cls)
                methods.add((cls, name, desc or ''))

        elif entry['tag'] == 11:  # CONSTANT_InterfaceMethodref
            cls, name, desc = resolve_ref(cp, entry)
            if cls and name:
                cls = _clean_array(cls)
                # Interface methods can be called as Methodref in some bytecode too,
                # but put them as methods anyway
                methods.add((cls, name, desc or ''))

    return classes, methods, fields


def _clean_array(name):
    """Convert array class names like [Ljava/lang/Object; to java/lang/Object."""
    if name.startswith('['):
        depth = 0
        for ch in name:
            if ch == '[':
                depth += 1
            else:
                break
        rest = name[depth:]
        if rest.startswith('L') and rest.endswith(';'):
            return rest[1:-1]
        return rest
    return name


def targets(name):
    """Check if a class name (using /) matches any target package."""
    for pkg in TARGET_PACKAGES:
        if name.startswith(pkg):
            return True
    return False


def strip_inner_class(name):
    """Strip inner class separator to get the outer class name.
    com/foo/Bar$Inner -> com/foo/Bar
    com/foo/Bar#1 -> com/foo/Bar
    """
    for sep in KNOWN_INNER_CLASS_SEPARATORS:
        idx = name.find(sep)
        if idx != -1:
            return name[:idx]
    return name


def extract_from_jar(jar_path):
    """Extract all target-package references from a jar file."""
    classes = set()
    methods = set()
    fields = set()

    try:
        with zipfile.ZipFile(jar_path, 'r') as zf:
            for name in zf.namelist():
                if name.endswith('.class') and not name.startswith('META-INF'):
                    try:
                        data = zf.read(name)
                        c, m, f = extract_references_from_class(data)
                        classes |= c
                        methods |= m
                        fields |= f
                    except Exception:
                        continue
    except (zipfile.BadZipFile, OSError) as e:
        print(f"WARN: {jar_path}: {e}", file=sys.stderr)

    # Filter for target packages
    classes = {c for c in classes if targets(c)}
    methods = {(c, n, d) for c, n, d in methods if targets(c)}
    fields = {(c, n, d) for c, n, d in fields if targets(c)}

    return classes, methods, fields


def extract_from_jar_constant_pool_only(jar_path):
    """Quick extraction using just the constant pool class names.
    Returns set of class names referenced."""
    return extract_from_jar(jar_path)


def load_support_symbols(support_jar_path):
    """Load all class/method/field symbols from doja-support.jar."""
    classes = set()
    methods = set()
    fields = set()

    try:
        with zipfile.ZipFile(support_jar_path, 'r') as zf:
            for name in zf.namelist():
                if not name.endswith('.class'):
                    continue
                if name.startswith('META-INF'):
                    continue
                # Convert path to class name
                cls_name = name[:-6]  # remove .class
                classes.add(cls_name)

                # Also extract methods/fields from support jar
                try:
                    data = zf.read(name)
                    c, m, f = extract_references_from_class(data)
                    # For support jar, we care about its own declarations
                    # Parse the class structure to get declared methods/fields
                    declared_m, declared_f = extract_declared_members(data)
                    for method_name, desc in declared_m:
                        methods.add((cls_name, method_name, desc))
                    for field_name, desc in declared_f:
                        fields.add((cls_name, field_name, desc))
                except Exception:
                    continue
    except (zipfile.BadZipFile, OSError) as e:
        print(f"WARN: {support_jar_path}: {e}", file=sys.stderr)

    return classes, methods, fields


def extract_declared_members(class_data):
    """Extract declared methods and fields from a .class file.
    Returns (set_of_methods, set_of_fields) where each is (name, descriptor).
    """
    methods = set()
    fields = set()

    try:
        cp = parse_constant_pool(class_data)
        pos = 8
        cp_count = struct.unpack_from('>H', class_data, 6)[0]
        # Skip constant pool
        pos = 8
        i = 1
        while i < cp_count:
            tag = class_data[pos]
            pos += 1
            if tag == 1:
                length = struct.unpack_from('>H', class_data, pos)[0]
                pos += 2 + length
                i += 1
            elif tag in (3, 4):
                pos += 4
                i += 1
            elif tag in (5, 6):
                pos += 8
                i += 2
            elif tag == 7:
                pos += 2
                i += 1
            elif tag == 8:
                pos += 2
                i += 1
            elif tag in (9, 10, 11, 12):
                pos += 4
                i += 1
            elif tag == 15:
                pos += 3
                i += 1
            elif tag == 16:
                pos += 2
                i += 1
            elif tag in (17, 18):
                pos += 4
                i += 1
            elif tag in (19, 20):
                pos += 2
                i += 1
            else:
                raise ValueError(f"Unknown tag {tag}")

        # Skip access_flags(2) + this_class(2) + super_class(2)
        pos += 6

        # Interfaces count
        iface_count = struct.unpack_from('>H', class_data, pos)[0]
        pos += 2 + iface_count * 2

        # Fields
        field_count = struct.unpack_from('>H', class_data, pos)[0]
        pos += 2
        for _ in range(field_count):
            pos += 2  # access_flags
            name_idx = struct.unpack_from('>H', class_data, pos)[0]
            pos += 2
            desc_idx = struct.unpack_from('>H', class_data, pos)[0]
            pos += 2
            attr_count = struct.unpack_from('>H', class_data, pos)[0]
            pos += 2
            for _ in range(attr_count):
                pos += 2  # attr_name_index
                attr_len = struct.unpack_from('>I', class_data, pos)[0]
                pos += 4 + attr_len
            name = resolve_utf8(cp, name_idx)
            desc = resolve_utf8(cp, desc_idx)
            if name:
                fields.add((name, desc or ''))

        # Methods
        method_count = struct.unpack_from('>H', class_data, pos)[0]
        pos += 2
        for _ in range(method_count):
            pos += 2  # access_flags
            name_idx = struct.unpack_from('>H', class_data, pos)[0]
            pos += 2
            desc_idx = struct.unpack_from('>H', class_data, pos)[0]
            pos += 2
            attr_count = struct.unpack_from('>H', class_data, pos)[0]
            pos += 2
            for _ in range(attr_count):
                pos += 2
                attr_len = struct.unpack_from('>I', class_data, pos)[0]
                pos += 4 + attr_len
            name = resolve_utf8(cp, name_idx)
            desc = resolve_utf8(cp, desc_idx)
            if name and name != '<init>' and name != '<clinit>':
                methods.add((name, desc or ''))
    except (ValueError, struct.error, IndexError):
        return set(), set()

    return methods, fields


def fmt_slash(typename):
    """Convert Java internal name to dotted: com/foo/Bar -> com.foo.Bar"""
    return typename.replace('/', '.')


def fmt_desc(desc):
    """Simple descriptor formatting (keep as-is but readable)."""
    return desc


def print_report(missing_classes, missing_methods, missing_fields, support_classes):
    """Print a differential report."""
    seen_root_classes = set()
    for cls in sorted(missing_classes):
        root = strip_inner_class(cls)
        if root not in seen_root_classes:
            seen_root_classes.add(root)
            if root not in support_classes:
                print(f"CLASS MISSING:   {fmt_slash(cls)}")
            else:
                inner = cls[len(root):]
                if inner:
                    print(f"  -> uses inner: {fmt_slash(inner)}")

    for cls, name, desc in sorted(missing_methods):
        root = strip_inner_class(cls)
        if root in support_classes:
            cls = fmt_slash(cls)
            print(f"METHOD MISSING:  {cls}.{name}{fmt_desc(desc)}")
        else:
            print(f"METHOD MISSING (class also missing): {fmt_slash(cls)}.{name}{fmt_desc(desc)}")

    for cls, name, desc in sorted(missing_fields):
        root = strip_inner_class(cls)
        cls_fmt = fmt_slash(cls)
        if root in support_classes:
            print(f"FIELD MISSING:   {cls_fmt}.{name}")
        else:
            print(f"FIELD MISSING (class also missing): {cls_fmt}.{name}")


def main():
    parser = argparse.ArgumentParser(description='Extract DoJa API references from game jars')
    parser.add_argument('game_jars', nargs='*', help='Game .jar files to analyze')
    parser.add_argument('--jar-list', '-l', help='File with list of jar paths (one per line)')
    parser.add_argument('--support-jar', '-s', required=True, help='Path to doja-support.jar')
    parser.add_argument('--missing-only', '-m', action='store_true', help='Only print missing symbols')
    parser.add_argument('--output-csv', '-o', help='Output CSV file for missing symbols')
    parser.add_argument('--verbose', '-v', action='store_true')
    args = parser.parse_args()

    # Load support symbols
    if args.verbose:
        print(f"Loading support symbols from {args.support_jar}...", file=sys.stderr)
    support_classes, support_methods, support_fields = load_support_symbols(args.support_jar)

    if args.verbose:
        print(f"Loaded {len(support_classes)} classes, {len(support_methods)} methods, "
              f"{len(support_fields)} fields from support jar", file=sys.stderr)

    # Collect game jars
    game_jars = list(args.game_jars)
    if args.jar_list:
        with open(args.jar_list) as f:
            game_jars.extend(line.strip() for line in f if line.strip())

    if not game_jars:
        print("No game jars specified. Use positional args or -l", file=sys.stderr)
        sys.exit(1)

    if args.verbose:
        print(f"Analyzing {len(game_jars)} game jars...", file=sys.stderr)

    # Extract references from all games
    all_classes = set()
    all_methods = set()
    all_fields = set()

    for i, jar_path in enumerate(game_jars):
        if args.verbose and (i % 100 == 0):
            print(f"  Progress: {i}/{len(game_jars)}", file=sys.stderr)
        classes, methods, fields = extract_from_jar(jar_path)
        all_classes |= classes
        all_methods |= methods
        all_fields |= fields

    if args.verbose:
        print(f"Total unique references: {len(all_classes)} classes, "
              f"{len(all_methods)} methods, {len(all_fields)} fields", file=sys.stderr)

    # Compute diff
    missing_classes = all_classes - support_classes

    # For methods/fields: missing if class known but method/field not declared
    missing_methods = set()
    missing_fields = set()

    for cls, name, desc in all_methods:
        root = strip_inner_class(cls)
        if root in support_classes and (cls, name, desc) not in support_methods:
            missing_methods.add((cls, name, desc))
        elif root not in support_classes:
            missing_methods.add((cls, name, desc))

    for cls, name, desc in all_fields:
        root = strip_inner_class(cls)
        if root in support_classes and (name, desc) not in support_fields:
            missing_fields.add((cls, name, desc))
        elif root not in support_classes:
            missing_fields.add((cls, name, desc))

    if args.verbose:
        print(f"Missing: {len(missing_classes)} classes, {len(missing_methods)} methods, "
              f"{len(missing_fields)} fields", file=sys.stderr)

    if args.missing_only:
        # Group by root class
        by_class = defaultdict(lambda: {'methods': set(), 'fields': set(), 'is_missing': False})
        for cls in missing_classes:
            root = strip_inner_class(cls)
            by_class[root]['is_missing'] = True
            by_class[root]['cls_name'] = cls

        for cls, name, desc in missing_methods:
            root = strip_inner_class(cls)
            by_class[root]['methods'].add((name, desc))

        for cls, name, desc in missing_fields:
            root = strip_inner_class(cls)
            by_class[root]['fields'].add((name, desc))

        for root in sorted(by_class.keys()):
            info = by_class[root]
            status = "CLASS_MISSING" if info['is_missing'] else "CLASS_EXISTS"
            print(f"\n[{status}] {fmt_slash(root)}")
            if info['methods']:
                for name, desc in sorted(info['methods']):
                    print(f"  METHOD: {name}{fmt_desc(desc)}")
            if info['fields']:
                for name, desc in sorted(info['fields']):
                    print(f"  FIELD: {name}")

    if args.output_csv:
        with open(args.output_csv, 'w') as f:
            f.write("type,package,class,member,descriptor\n")
            for cls in sorted(missing_classes):
                f.write(f"CLASS,{_pkg(cls)},{fmt_slash(cls)},,\n")
            for cls, name, desc in sorted(missing_methods):
                f.write(f"METHOD,{_pkg(cls)},{fmt_slash(cls)},{name},\"{desc}\"\n")
            for cls, name, desc in sorted(missing_fields):
                f.write(f"FIELD,{_pkg(cls)},{fmt_slash(cls)},{name},\n")


def _pkg(cls_name):
    """Get the outer package prefix."""
    name = cls_name.replace('/', '.')
    for pfx in ['com.nttdocomo', 'com.j_phone', 'com.docomostar', 'com.mascotcapsule',
                'com.sonyericsson', 'javax.microedition']:
        if name.startswith(pfx):
            return pfx
    last = name.rfind('.')
    if last > 0:
        return name[:last]
    return ''


if __name__ == '__main__':
    main()
