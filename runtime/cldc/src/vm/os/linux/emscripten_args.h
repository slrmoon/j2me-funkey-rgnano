/*
 * WebAssembly: main(void) that gets argc/argv from JavaScript.
 *
 * The Emscripten 3.1.6 + clang 15 pair of Debian/Ubuntu drops a
 * main(int, char**) (clang names it __main_argc_argv, which Emscripten does
 * not call), so on Emscripten the real main is renamed and this one passes
 * it the program arguments (arguments_: Module.arguments at start, or the
 * node command line).
 * No <emscripten.h>: its em_asm.h clashes with the VM headers.
 *
 * Use:  #include "emscripten_args.h"   then   EMSCRIPTEN_MAIN(real_main)
 */
#ifndef EMSCRIPTEN_ARGS_H
#define EMSCRIPTEN_ARGS_H
#ifdef __EMSCRIPTEN__
#include <stdlib.h>
#include <string.h>

extern "C" char *emscripten_run_script_string(const char *script);

/* the arguments, joined by \x01 */
static int fk_split_arguments(char ***out) {
  const char *js =
    "(typeof arguments_!=='undefined'?arguments_:[]).join('\\x01')";
  char *s = strdup(emscripten_run_script_string(js));
  int n = 2, i = 1;
  char *p;
  char **argv;
  for (p = s; *p; p++) if (*p == '\x01') n++;
  argv = (char **) malloc(sizeof(char *) * (size_t) (n + 1));
  argv[0] = (char *) "program";
  if (*s) {
    argv[i++] = s;
    for (p = s; *p; p++) {
      if (*p == '\x01') { *p = 0; argv[i++] = p + 1; }
    }
  }
  argv[i] = NULL;
  *out = argv;
  return i;
}

#define EMSCRIPTEN_MAIN(real_main)                 \
  int main(void) {                                 \
    char **fk_argv;                                \
    int fk_argc = fk_split_arguments(&fk_argv);    \
    return real_main(fk_argc, fk_argv);            \
  }
#endif
#endif
