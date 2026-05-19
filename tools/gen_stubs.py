#!/usr/bin/env python3
"""Generate DoJa stub .java files from the missing symbols diff.

Reads the output of extract_refs.py --missing-only and creates minimal
stub .java files for all missing classes/methods.
"""

import os
import sys
import re
import argparse
from collections import defaultdict

STUBS_DIR = os.path.dirname(os.path.abspath(__file__))

DOJA_STUBS_BASE = STUBS_DIR  # will be overridden

UNSUPPORTED_PACKAGES = {
    # 3D graphics — complex rendering engine
    'com/nttdocomo/opt/ui/j3d',
    'com/nttdocomo/opt/ui/j3d2',
    'com/nttdocomo/opt/ui/ogl',
    'com/nttdocomo/ui/graphics3d',
    'com/nttdocomo/ui/ogl',
    'com/nttdocomo/ui/sound3d',
    'com/nttdocomo/ui/util3d',
    'com/nttdocomo/ui/Audio3D',
    # OpenGL ES wrapper
    'com/nttdocomo/ui/ogl',
    'com/nttdocomo/opt/ui/ogl',
    # Camera
    'com/nttdocomo/opt/ui/Camera',
    # Hardware devices
    'com/nttdocomo/opt/device',
    'com/nttdocomo/device',
    # Filesystem beyond scratchpad
    'com/nttdocomo/fs',
    # Bluetooth/OBEX/SPP
    'com/nttdocomo/io/ObexConnection',
    'com/nttdocomo/io/SPPConnection',
    'com/nttdocomo/io/ServerObexConnection',
    # Network
    'com/nttdocomo/io/HttpConnection',  # already exists but needs more
    # System services
    'com/nttdocomo/system',
    # Vendor-specific
    'com/nttdocomo/nec',
    # DoCoMo Star full API
    'com/docomostar',
    # Crypto
    'com/nttdocomo/util/MessageDigest',
    # 3D Eruption engine
    'com/docomostar/opt/ui/eruption',
    'com/docomostar/opt/ui/stereoscope',
}

SIMPLE_CLASSES = {
    'com/nttdocomo/lang/IllegalStateException': ('RuntimeException', True),
    'com/nttdocomo/lang/XString': ('Object', False),
    'com/nttdocomo/lang/MemoryManager': ('Object', False),
    'com/nttdocomo/util/JarFormatException': ('Exception', True),
    'com/nttdocomo/util/JarInflater': ('Object', False),
    'com/nttdocomo/util/Base64': ('Object', False),
    'com/nttdocomo/util/Phone': ('Object', False),
    'com/nttdocomo/util/ScheduleDate': ('Object', False),
    'com/nttdocomo/util/Timer': ('Object', False),
    'com/nttdocomo/util/MessageDigest': ('Object', False),
    'com/nttdocomo/net/URLDecoder': ('Object', False),
    'com/nttdocomo/ui/Label': ('com.nttdocomo.ui.Component', False),
    'com/nttdocomo/ui/TextBox': ('com.nttdocomo.ui.Component', False),
    'com/nttdocomo/ui/Button': ('com.nttdocomo.ui.Component', False),
    'com/nttdocomo/ui/ListBox': ('com.nttdocomo.ui.Component', False),
    'com/nttdocomo/ui/Ticker': ('com.nttdocomo.ui.Component', False),
    'com/nttdocomo/ui/AnchorButton': ('com.nttdocomo.ui.Component', False),
    'com/nttdocomo/ui/ImageLabel': ('com.nttdocomo.ui.Component', False),
    'com/nttdocomo/ui/ImageMap': ('Object', False),
    'com/nttdocomo/ui/ImageEncoder': ('Object', False),
    'com/nttdocomo/ui/EncodedImage': ('Object', False),
    'com/nttdocomo/ui/Sprite': ('Object', False),
    'com/nttdocomo/ui/SpriteSet': ('Object', False),
    'com/nttdocomo/ui/Palette': ('Object', False),
    'com/nttdocomo/ui/PalettedImage': ('Object', False),
    'com/nttdocomo/ui/VisualPresenter': ('Object', False),
    'com/nttdocomo/ui/Audio3D': ('Object', False),
    'com/nttdocomo/ui/MApplication': ('com.nttdocomo.ui.IApplication', False),
    'com/nttdocomo/io/FileEntity': ('Object', False),
    'com/nttdocomo/io/BufferedReader': ('Object', False),
    'com/nttdocomo/system/SoundStore': ('Object', False),
    'com/nttdocomo/system/StoreException': ('RuntimeException', True),
    'com/nttdocomo/system/InterruptedOperationException': ('RuntimeException', True),
    'com/nttdocomo/system/MailException': ('Exception', True),
    'com/nttdocomo/system/MailConstants': ('Object', False),
    'com/nttdocomo/system/ImageStore': ('Object', False),
    'com/nttdocomo/opt/ui/Canvas2': ('com.nttdocomo.ui.Canvas', False),
    'com/nttdocomo/opt/ui/Graphics2': ('com.nttdocomo.ui.Graphics', False),
    'com/nttdocomo/opt/ui/Sprite': ('Object', False),
    'com/nttdocomo/opt/ui/SpriteSet': ('Object', False),
    'com/nttdocomo/opt/ui/StereoScreen': ('Object', False),
    'com/nttdocomo/opt/ui/SubDisplay': ('Object', False),
    'com/nttdocomo/opt/ui/TransparentImage': ('Object', False),
    'com/nttdocomo/opt/ui/Palette': ('Object', False),
    'com/nttdocomo/opt/ui/PalettedImage': ('Object', False),
    'com/nttdocomo/opt/ui/PhoneSystem2': ('Object', False),
    'com/nttdocomo/opt/ui/PointingDevice': ('Object', False),
    'com/nttdocomo/opt/ui/AudioPresenter2': ('com.nttdocomo.ui.AudioPresenter', False),
    'com/nttdocomo/opt/ui/PhoneResource': ('Object', False),
}

INTERFACES = {
    'com/nttdocomo/util/TimerListener',
    'com/nttdocomo/util/EventListener',
    'com/nttdocomo/ui/Audio3DListener',
    'com/nttdocomo/ui/sound3d/SoundPosition',
    'com/nttdocomo/ui/graphics3d/DrawableObject3D',
    'com/nttdocomo/ui/graphics3d/collision/Shape',
    'com/nttdocomo/ui/graphics3d/collision/CollisionObserver',
    'com/nttdocomo/docomostar/ui/MediaListener',
    'com/nttdocomo/device/BTStateListener',
    'com/nttdocomo/opt/device/SpeechListener',
    'com/nttdocomo/system/MessageFolderListener',
    'com/docomostar/ui/MediaListener',
}

ABSTRACT_CLASSES = {
    'com/nttdocomo/ui/Audio3DLocalization',
    'com/nttdocomo/ui/graphics3d/collision/AbstractShape',
    'com/nttdocomo/ui/graphics3d/collision/BoundingVolume',
    'com/nttdocomo/ui/sound3d/SoundPosition',
}

# Classes from com.docomostar that mirror com.nttdocomo
DOCOMO_STAR_CLASSES = {
    'com/docomostar/StarApplication': 'Object',
    'com/docomostar/StarApplicationManager': 'Object',
    'com/docomostar/system/Launcher': 'Object',
    'com/docomostar/system/PhoneSystem': 'Object',
    'com/docomostar/opt/system/PhoneSystem2': 'Object',
    'com/docomostar/opt/ui/TouchDevice': 'Object',
}


def indent(n):
    return '    ' * n


def method_signature(name, descriptor, n=0):
    """Convert JVM descriptor to Java method stub."""
    if descriptor is None:
        descriptor = ''

    # Parse return type and parameters
    desc = descriptor.strip() or ''

    # Split params and return
    if ')' in desc:
        params_part, ret_part = desc.split(')', 1)
        params_part = params_part.lstrip('(')
    else:
        params_part = ''
        ret_part = 'V'

    # Parse parameter types
    params = []
    i = 0
    pp = params_part
    while i < len(pp):
        ch = pp[i]
        if ch == '[':
            j = i
            while j < len(pp) and pp[j] == '[':
                j += 1
            rest = pp[j:]
            if rest.startswith('L'):
                end = rest.find(';')
                if end < 0:
                    end = len(rest)
                params.append(pp[i:j+end+1])
                i = j + end + 1
            else:
                params.append(pp[i:j+1])
                i = j + 1
        elif ch == 'L':
            end = pp.find(';', i)
            if end < 0:
                end = len(pp)
            params.append(pp[i:end+1])
            i = end + 1
        else:
            params.append(pp[i:i+1])
            i += 1

    # Generate parameter names
    param_names = []
    for j, p in enumerate(params):
        param_names.append(f"p{j}")

    # Generate Java types
    java_params = []
    for j, p in enumerate(params):
        java_params.append((desc_to_java(p), param_names[j]))

    return_type = desc_to_java(ret_part)

    # Generate method signature
    params_str = ', '.join(f"{t} {n}" for t, n in java_params)

    return return_type, params_str


def desc_to_java(desc):
    """Convert a single type descriptor to Java type name."""
    if not desc:
        return 'void'

    depth = 0
    i = 0
    while i < len(desc) and desc[i] == '[':
        depth += 1
        i += 1

    base = desc[i:]
    arrays = '[]' * depth

    if base == 'V':
        return 'void'
    elif base == 'Z':
        return 'boolean' + arrays
    elif base == 'B':
        return 'byte' + arrays
    elif base == 'C':
        return 'char' + arrays
    elif base == 'S':
        return 'short' + arrays
    elif base == 'I':
        return 'int' + arrays
    elif base == 'J':
        return 'long' + arrays
    elif base == 'F':
        return 'float' + arrays
    elif base == 'D':
        return 'double' + arrays
    elif base.startswith('L') and base.endswith(';'):
        return base[1:-1].replace('/', '.') + arrays
    else:
        return base.replace('/', '.') + arrays


def generate_method_body(name, return_type, is_unsupported):
    """Generate appropriate method body."""
    if name == '<init>':
        # Constructor calls super
        return '    super();'

    if return_type == 'void':
        if is_unsupported:
            return '    throw new com.nttdocomo.lang.UnsupportedOperationException();'
        return '    // no-op'

    if return_type == 'boolean':
        return '    return false;'
    elif return_type == 'int' or return_type == 'long' or return_type == 'short' or return_type == 'byte':
        return '    return 0;'
    elif return_type == 'float':
        return '    return 0.0f;'
    elif return_type == 'double':
        return '    return 0.0;'
    elif return_type == 'char':
        return "    return '\\0';"
    else:
        return '    return null;'


def java_class_name(internal_name):
    """Convert com/foo/Bar to Bar"""
    return internal_name.split('/')[-1]


def java_package(internal_name):
    """Convert com/foo/Bar to com.foo"""
    parts = internal_name.split('/')
    return '.'.join(parts[:-1])


def is_unsupported(internal_name):
    """Check if a class is in unsupported category."""
    # Check full class path match
    for prefix in UNSUPPORTED_PACKAGES:
        if internal_name == prefix:
            return True
    # Check prefix match
    for prefix in UNSUPPORTED_PACKAGES:
        if '/' in prefix and internal_name.startswith(prefix + '/'):
            return True
        elif internal_name.startswith(prefix + '/'):
            return True
        elif internal_name.startswith(prefix):
            return True
    return False


def gen_field(name, is_final=True, field_type='int', init_value=None):
    """Generate a field declaration."""
    mods = 'public static final' if is_final else 'public'
    if init_value is not None:
        return f'    {mods} {field_type} {name} = {init_value};'
    return f'    {mods} {field_type} {name};'


def generate_class(class_name, methods, fields, is_missing_class=True):
    """Generate a complete stub .java file for a class."""
    pkg = java_package(class_name)
    cls = java_class_name(class_name)
    unsup = is_unsupported(class_name)

    # Determine parent class
    parent = None
    if class_name in SIMPLE_CLASSES:
        parent_info = SIMPLE_CLASSES[class_name]
        parent = parent_info[0]
    elif class_name in DOCOMO_STAR_CLASSES:
        parent = DOCOMO_STAR_CLASSES[class_name]
    elif class_name.endswith('Exception'):
        parent = 'RuntimeException'

    # Determine if interface
    if class_name in INTERFACES:
        class_decl = f'public interface {cls}'
        parent = None
    elif class_name in ABSTRACT_CLASSES:
        if parent:
            class_decl = f'public abstract class {cls} extends {parent}'
        else:
            class_decl = f'public abstract class {cls}'
    elif parent:
        class_decl = f'public class {cls} extends {parent}'
    else:
        class_decl = f'public class {cls}'

    lines = []
    lines.append(f'package {pkg};')
    lines.append('')

    # Check if we need additional imports
    if unsup:
        lines.append('// UNSUPPORTED: This API is device-dependent and not fully implemented')
        lines.append('import com.nttdocomo.lang.UnsupportedOperationException;')
    lines.append('')

    # Check for special imports
    needs_cls_map = False
    if parent and '/' in parent:
        needs_cls_map = True

    if class_name in INTERFACES:
        lines.append(class_decl + ' {')
        lines.append('')

        # Interface method stubs
        if methods:
            for name, desc in sorted(methods):
                if name == '<init>' or name == '<clinit>':
                    continue
                return_type, params_str = method_signature(name, desc)
                lines.append(f'    {return_type} {name}({params_str});')
                lines.append('')

        lines.append('}')
        return '\n'.join(lines)

    # For regular classes
    lines.append(class_decl + ' {')
    lines.append('')

    # Fields
    if fields:
        for fname, fdesc in sorted(fields):
            if fdesc:
                ftype = desc_to_java(fdesc)
            else:
                ftype = 'int'
            lines.append(f'    public {ftype} {fname};')
        lines.append('')

    # Methods
    if methods:
        for name, desc in sorted(methods):
            return_type, params_str = method_signature(name, desc)
            body = generate_method_body(name, return_type, unsup)

            if name == '<init>':
                lines.append(f'    public {cls}({params_str}) {{')
                lines.append(body)
                lines.append('    }')
            else:
                lines.append(f'    public {return_type} {name}({params_str}) {{')
                lines.append(body)
                lines.append('    }')
            lines.append('')

    lines.append('}')
    return '\n'.join(lines)


def parse_diff(diff_text):
    """Parse the diff output into structured data."""
    classes = defaultdict(lambda: {'methods': set(), 'fields': set(), 'missing': False})

    current_class = None
    current_status = None

    for line in diff_text.strip().split('\n'):
        line = line.strip()

        # Match [CLASS_MISSING] or [CLASS_EXISTS]
        m = re.match(r'^\[(CLASS_MISSING|CLASS_EXISTS)\]\s+(.+)$', line)
        if m:
            current_status = m.group(1)
            current_class = m.group(2).replace('.', '/')
            classes[current_class]['missing'] = (current_status == 'CLASS_MISSING')
            continue

        # Match METHOD / FIELD
        m = re.match(r'^\s*(METHOD|FIELD):\s+(.+)$', line)
        if m and current_class:
            member_type = m.group(1)
            member_sig = m.group(2)

            if member_type == 'METHOD':
                if '(' in member_sig:
                    # Split name and descriptor
                    paren = member_sig.index('(')
                    name = member_sig[:paren]
                    desc = member_sig[paren:]
                else:
                    name = member_sig
                    desc = ''
                classes[current_class]['methods'].add((name, desc))
            elif member_type == 'FIELD':
                # Field just has name
                name = member_sig.strip()
                classes[current_class]['fields'].add((name, ''))

    return dict(classes)


def main():
    parser = argparse.ArgumentParser(description='Generate DoJa stub Java files from diff')
    parser.add_argument('diff_file', help='Path to the missing symbols diff file')
    parser.add_argument('--stubs-dir', '-d', required=True,
                       help='Path to the doja-stubs base directory (contains com/)')
    parser.add_argument('--dry-run', '-n', action='store_true',
                       help='Print what would be generated without creating files')
    parser.add_argument('--only-missing-classes', action='store_true',
                       help='Only generate files for completely missing classes')
    parser.add_argument('--type', '-t', choices=['new', 'existing'],
                       help='Process only new classes or existing classes with missing methods')
    args = parser.parse_args()

    with open(args.diff_file) as f:
        diff_text = f.read()

    classes = parse_diff(diff_text)

    stubs_base = os.path.abspath(args.stubs_dir)

    new_files = 0
    updated_files = 0

    for class_name, info in sorted(classes.items()):
        pkg_path = class_name.replace('/', '.')
        cls_simple = java_class_name(class_name)
        pkg = java_package(class_name)
        pkg_dir = os.path.join(stubs_base, class_name[:class_name.rfind('/')].replace('/', os.sep))
        java_path = os.path.join(stubs_base, class_name.replace('/', os.sep) + '.java')

        if info['missing']:
            # New class stub
            if args.type == 'existing':
                continue

            if args.only_missing_classes and info.get('skip_methods'):
                continue

            print(f"  NEW: {pkg_path} ({len(info['methods'])} methods, {len(info['fields'])} fields)")

            java_code = generate_class(class_name, info['methods'], info['fields'], is_missing_class=True)

            if not args.dry_run:
                os.makedirs(pkg_dir, exist_ok=True)
                with open(java_path, 'w') as f:
                    f.write(java_code)

            new_files += 1
        else:
            # Existing class needs methods/fields added
            if args.type == 'new':
                continue

            if not info['methods'] and not info['fields']:
                continue

            if not os.path.exists(java_path):
                print(f"  SKIP (source not found): {pkg_path}")
                continue

            print(f"  UPDATE: {pkg_path} — adding {len(info['methods'])} methods, {len(info['fields'])} fields")

            if not args.dry_run:
                # Read existing source
                with open(java_path) as f:
                    existing = f.read()

                # Add missing methods and fields
                # Find the last closing brace
                last_brace = existing.rfind('}')
                if last_brace < 0:
                    print(f"    WARN: can't parse {java_path}")
                    continue

                new_members = []

                # Format new methods
                for name, desc in sorted(info['methods']):
                    if name == '<init>':
                        # Constructor
                        params_str = method_signature(name, desc)[1]
                        new_members.append(f'\n    public {cls_simple}({params_str}) {{')
                        new_members.append(f'        // no-op')
                        new_members.append(f'    }}')
                    elif name == '<clinit>':
                        continue
                    else:
                        return_type, params_str = method_signature(name, desc)
                        body = generate_method_body(name, return_type, False)
                        lines_m = [
                            f'',
                            f'    public {return_type} {name}({params_str}) {{',
                            f'        {body}',
                            f'    }}'
                        ]
                        new_members.append('\n'.join(lines_m))

                # Format new fields
                for fname, fdesc in sorted(info['fields']):
                    new_members.append(f'\n    public int {fname} = 0;')

                insertion = '\n'.join(new_members) + '\n'

                # Insert before last brace
                new_source = existing[:last_brace] + insertion + existing[last_brace:]

                with open(java_path, 'w') as f:
                    f.write(new_source)

            updated_files += 1

    print(f"\nTotal: {new_files} new files, {updated_files} updated files")
    if args.dry_run:
        print("DRY RUN — no files were modified")


if __name__ == '__main__':
    main()
