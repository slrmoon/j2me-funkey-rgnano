#!/usr/bin/env python3
"""Same as test-games.py, for i-appli (DoJa) games: every .jam under the
given folders is run with its jar, OK / 5 / left soft key are pressed in
turn, and the log and screenshots of each game are kept.

    ./test-doja.py /mnt/d/j2me-x86/appli -o results-doja -j 6 -t 15 --speed 3

The jar of a .jam: the same name (any letter case), the jar named in its
PackageURL, or the only jar of the folder. The game's .sp scratchpad is
copied for each run, so the games' own files are never written.
Options: ./test-games.py -h
"""
import os
import runpy
import sys

args = sys.argv[1:]
if '-h' in args or '--help' in args:
    print(__doc__)
if not any(a in ('-o', '--output') or a.startswith('--output=') for a in args):
    args = ['-o', 'test-results-doja'] + args
sys.argv = [os.path.join(os.path.dirname(os.path.abspath(__file__)), 'test-games.py'), '--doja'] + args
runpy.run_path(sys.argv[0], run_name='__main__')
