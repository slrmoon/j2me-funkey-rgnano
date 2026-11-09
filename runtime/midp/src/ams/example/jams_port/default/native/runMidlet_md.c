/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <runMidlet.h>
#include <stdio.h>

/**
 * Runs a MIDlet from an installed MIDlet suite. This is an example of
 * how to use the public MIDP API.
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 *
 * IMPL_NOTE:determine if it is desirable for user targeted output
 *       messages to be sent via the log/trace service, or if
 *       they should remain as printf calls
 */
#ifdef __EMSCRIPTEN__
/*
 * Browser: the Emscripten 3.1.6 + clang 15 pair of Debian/Ubuntu never calls
 * a main(int, char**) (see cldc/src/vm/os/linux/emscripten_args.h), so the
 * arguments come from JavaScript (Module.arguments), joined by \x01.
 */
#include <stdlib.h>
#include <string.h>
char *emscripten_run_script_string(const char *script);

int
main(void) {
    char *s = strdup(emscripten_run_script_string(
        "(typeof arguments_!=='undefined'?arguments_:[]).join('\\x01')"));
    int n = 2, argc = 1;
    char *p;
    char **argv;
    for (p = s; *p; p++) if (*p == '\x01') n++;
    argv = (char **) malloc(sizeof(char *) * (size_t) (n + 1));
    argv[0] = (char *) "runMidlet";
    if (*s) {
        argv[argc++] = s;
        for (p = s; *p; p++) {
            if (*p == '\x01') { *p = 0; argv[argc++] = p + 1; }
        }
    }
    argv[argc] = NULL;
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    return runMidlet(argc, argv);
}
#else
int
main(int argc, char** commandlineArgs) {
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    return runMidlet(argc, commandlineArgs);
}
#endif
