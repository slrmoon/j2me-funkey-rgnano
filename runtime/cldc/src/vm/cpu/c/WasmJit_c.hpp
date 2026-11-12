/*
 * WasmJit_c.hpp: a just-in-time compiler from Java bytecode to WebAssembly
 * for the C interpreter of the browser build. Included by Interpreter_c.cpp
 * (it needs the interpreter's state and helpers), only for Emscripten.
 *
 * DESIGN
 *
 * A hot method gets a WebAssembly module of its own, made here in C, then
 * compiled and instantiated by the browser (fk_jit_load, JavaScript). The
 * module's two functions are added to the indirect function table:
 *   entry()      - its index becomes the method's execution entry, so the
 *                  interpreter calls it like any other method entry
 *                  (invoke_java_method); it builds the Java frame with
 *                  interpreter_method_entry() and runs the method from 0;
 *   run(block)   - runs the method from a basic block; the interpreter
 *                  calls it on a backward branch to a loop header (OSR).
 *
 * Compiled code works on the SAME Java frames as the interpreter (locals,
 * expression stack and frame fields in memory). While it runs it keeps the
 * locals and the stack slots in WebAssembly locals (one i32 per 32-bit
 * slot, as in memory); memory is brought up to date only when control
 * leaves it. Nothing in the VM can see the frame in between: the garbage
 * collector, exceptions and thread switches happen only inside calls, and
 * calls are made only after the frame was written back.
 *
 * Leaving the compiled code ("exit at bci"): everything is written back and
 * the interpreter's state is set to that bytecode, so the interpreter goes
 * on from there. This is how the compiled code handles anything it does not
 * do itself: an unsupported bytecode, an exception (null pointer, array
 * index, division by zero: the interpreter executes the bytecode again and
 * throws), a timer tick on a backward branch (thread switch, end of the time
 * slice), a return it cannot do the short way.
 *
 * Calls (invoke*, new, checkcast...): the frame is written back and the
 * interpreter's own handler for that bytecode is called (jit_exec_bytecode).
 * If afterwards the state is right after the bytecode in the same frame -
 * the callee was compiled and ran to its end, or it was a fast accessor,
 * quick native, allocation... - the compiled code reloads its locals (the
 * GC may have moved objects) and goes on. Otherwise (the callee is
 * interpreted, an exception was thrown, another thread runs now) it returns
 * at once without touching anything: the interpreter takes over.
 *
 * Nothing compiled is ever on the native stack when the interpreter or the
 * VM looks at Java frames, so GC, exceptions, green threads and the slave
 * mode work as before. Methods are never "uncompiled": the compiled code is
 * always allowed to give up and exit to the interpreter.
 *
 * Calls between compiled methods (invokestatic/special/virtual/final of a
 * compiled, not synchronized method) skip the interpreter: the caller
 * builds the callee's frame and calls its run() through the function
 * table. At most MAX_NESTING compiled calls are nested on the native stack;
 * deeper, the call is left to the interpreter.
 *
 * A method becomes hot after PHONEME_JIT_THRESHOLD invocations + backward
 * branches. Bytecodes not quickened yet (never executed) are exits; when
 * such a part starts to run, the method is compiled again.
 *
 * Settings (environment; on the page ?env=NAME=value, ?jit=0):
 *   PHONEME_JIT=0            interpreter only
 *   PHONEME_JIT_THRESHOLD=n  hotness threshold (400)
 *   PHONEME_JIT_TRACE=1      log compilations and statistics
 *   PHONEME_JIT_ROM=0        do not compile the system (ROM) classes
 *   PHONEME_JIT_SKIP=n       debugging: do not compile the n-th method
 *   PHONEME_JIT_DUMP=1       debugging: print each module (base64)
 */

#if ENABLE_WASM_JIT && WASM_JIT_PART == 1

/* Part 1 (included before the bytecode helpers): settings, counters and
 * the table of compiled methods, used by the interpreter's hooks. */

namespace wasm_jit {

static int  jit_on = -1;            // -1: settings not read yet
static int  jit_threshold = 400;    // invocations + backward branches
static int  jit_trace = 0;
static int  jit_rom = 1;            // compile methods of the ROM (system)
static int  jit_skip = -1;          // debugging: do not compile the n-th
static int  first_fn = 0x7fffffff;  // lowest table index of compiled code

struct Counter { address method; int count; int attempts; };
enum { COUNTERS = 8192, COUNTER_WAYS = 4 };
static Counter counters[COUNTERS + COUNTER_WAYS];

/* The counter of a method. Four places per method: with one, two hot
 * methods that fell on the same place reset each other's count for ever
 * and neither was compiled (String.equals called from a loop that calls
 * something else, for one). A newcomer takes a free place or that of the
 * coldest method. */
static Counter* counter_miss(address method, Counter* c) {
  Counter* victim = c;
  for (int i = 0; i < COUNTER_WAYS; i++) {
    if (c[i].method == method) return &c[i];
    if (c[i].method == NULL) { victim = &c[i]; break; }
    if ((unsigned)c[i].count < (unsigned)victim->count) victim = &c[i];
  }
  victim->method = method;
  victim->count = 0;
  victim->attempts = 0;
  return victim;
}

static inline Counter* counter_for(address method) {
  unsigned h = ((unsigned)(uintptr_t)method * 2654435761u) >> 19;
  Counter* c = &counters[h & (COUNTERS - 1)];
  if (c->method == method) return c;
  return counter_miss(method, c);
}

static void read_settings();
static void hot_invoke(address method);
static void backedge(address method, int bci);
static void reset_nesting();

}  // namespace wasm_jit

static inline address method_execution_entry(address method) {
  return **(address**)(method + Method::variable_part_offset());
}

#endif

#if ENABLE_WASM_JIT && WASM_JIT_PART == 2

static void jit_exec_bytecode_entry();
static void jit_stale_exit_entry();

extern "C" {
  extern void os_emscripten_poll_ticks();
}

/* the JavaScript side: compile and instantiate a module, put its exports
 * entry() and run() into the function table; returns entry()'s index (run
 * is the next one) or 0 */
EM_JS(int, fk_jit_load, (const unsigned char* bytes, int len,
                         const int* imports, int nimports), {
  try {
    var imp = { e: { m: wasmMemory, t: wasmTable } };
    for (var i = 0; i < nimports; i++) {
      imp.e['f' + i] = wasmTable.get(HEAP32[(imports >> 2) + i]);
    }
    var b = HEAPU8.slice(bytes, bytes + len);
    if (typeof ENV !== 'undefined' && ENV['PHONEME_JIT_DUMP']) {
      var t = ''; for (var k = 0; k < b.length; k++) t += String.fromCharCode(b[k]);
      out('JITMOD ' + btoa(t));
    }
    var mod = new WebAssembly.Module(b);
    var inst = new WebAssembly.Instance(mod, imp);
    var base = wasmTable.grow(2);
    wasmTable.set(base, inst.exports.entry);
    wasmTable.set(base + 1, inst.exports.run);
    return base;
  } catch (e) {
    err('jit: ' + e);
    return 0;
  }
});

namespace wasm_jit {

enum { MAX_CODE = 12000, MAX_LOCALS = 250, MAX_STACK = 64,
       MAX_NESTING = 40 };

/* ------------------------------------------------- compiled method info */

struct Entry { int bci; int block; };
struct Info {
  int entry_fn, run_fn;
  address plain_entry;     // the interpreter entry it replaced
  enum { MAX_STALE = 16 };
  int n_stale;             // compiled code stops at these bytecodes: they
  int stale[MAX_STALE];    // were not yet quickened then
  int n_entries;
  Entry* entries;
};
enum { INFOS = 4096 };
static Info* infos[INFOS];     // by entry_fn
static int   compiled_count = 0;

/* the nesting of compiled methods on the native stack (direct calls) */
static int nesting = 0;

/* statistics (PHONEME_JIT_TRACE=1): runs entered, exits to the
 * interpreter, calls that went on in compiled code, calls that left it */
static int stats[8];
enum { ST_RUNS, ST_EXITS, ST_CALLS, ST_CALL_LEFT, ST_RETURNS, ST_LAST_EXIT,
       ST_DIRECT };

static Info* info_for(int entry_fn) {
  for (int i = 0; i < INFOS; i++) {
    Info* in = infos[(entry_fn + i) & (INFOS - 1)];
    if (in == NULL) return NULL;
    if (in->entry_fn == entry_fn) return in;
  }
  return NULL;
}

static void add_info(Info* in) {
  for (int i = 0; i < INFOS; i++) {
    Info** p = &infos[(in->entry_fn + i) & (INFOS - 1)];
    if (*p == NULL) { *p = in; return; }
  }
}

/* ---------------------------------------------------------- byte buffer */

struct Buf {
  unsigned char* p;
  int n, cap;
  Buf() : p(NULL), n(0), cap(0) {}
  ~Buf() { free(p); }
  void byte(int b) {
    if (n == cap) {
      cap = cap ? cap * 2 : 4096;
      p = (unsigned char*)realloc(p, cap);
    }
    p[n++] = (unsigned char)b;
  }
  void uleb(unsigned v) {
    do { int b = v & 0x7f; v >>= 7; if (v) b |= 0x80; byte(b); } while (v);
  }
  void sleb(int v) {
    for (;;) {
      int b = v & 0x7f; v >>= 7;
      if ((v == 0 && !(b & 0x40)) || (v == -1 && (b & 0x40))) { byte(b); return; }
      byte(b | 0x80);
    }
  }
  void sleb64(jlong v) {
    for (;;) {
      int b = (int)(v & 0x7f); v >>= 7;
      if ((v == 0 && !(b & 0x40)) || (v == -1 && (b & 0x40))) { byte(b); return; }
      byte(b | 0x80);
    }
  }
  void bytes(const Buf& o) { for (int i = 0; i < o.n; i++) byte(o.p[i]); }
  void section(int id, const Buf& body) { byte(id); uleb(body.n); bytes(body); }
};

/* WebAssembly opcodes */
enum {
  W_UNREACHABLE = 0x00, W_BLOCK = 0x02, W_LOOP = 0x03, W_IF = 0x04,
  W_ELSE = 0x05, W_END = 0x0b, W_BR = 0x0c, W_BR_IF = 0x0d,
  W_BR_TABLE = 0x0e, W_RETURN = 0x0f, W_CALL = 0x10, W_DROP = 0x1a,
  W_SELECT = 0x1b, W_LGET = 0x20, W_LSET = 0x21, W_LTEE = 0x22,
  W_I32LOAD = 0x28, W_I64LOAD = 0x29, W_I32LOAD8S = 0x2c,
  W_I32LOAD16S = 0x2e, W_I32LOAD16U = 0x2f, W_I32STORE = 0x36,
  W_I64STORE = 0x37, W_I32STORE8 = 0x3a, W_I32STORE16 = 0x3b,
  W_I32CONST = 0x41, W_I64CONST = 0x42,
  W_I32EQZ = 0x45, W_I32EQ = 0x46, W_I32NE = 0x47, W_I32LTS = 0x48,
  W_I32LTU = 0x49, W_I32GTS = 0x4a, W_I32GTU = 0x4b, W_I32LES = 0x4c,
  W_I32GES = 0x4e, W_I32GEU = 0x4f,
  W_I64EQZ = 0x50, W_I64EQ = 0x51, W_I64LTS = 0x53, W_I64GTS = 0x55,
  W_F32EQ = 0x5b, W_F32LT = 0x5d, W_F32GT = 0x5e, W_F32LE = 0x5f,
  W_F32GE = 0x60,
  W_F64EQ = 0x61, W_F64LT = 0x63, W_F64GT = 0x64, W_F64LE = 0x65,
  W_F64GE = 0x66,
  W_I32ADD = 0x6a, W_I32SUB = 0x6b, W_I32MUL = 0x6c, W_I32DIVS = 0x6d,
  W_I32REMS = 0x6f, W_I32AND = 0x71, W_I32OR = 0x72, W_I32XOR = 0x73,
  W_I32SHL = 0x74, W_I32SHRS = 0x75, W_I32SHRU = 0x76,
  W_I64ADD = 0x7c, W_I64SUB = 0x7d, W_I64MUL = 0x7e, W_I64DIVS = 0x7f,
  W_I64REMS = 0x81, W_I64AND = 0x83, W_I64OR = 0x84, W_I64XOR = 0x85,
  W_I64SHL = 0x86, W_I64SHRS = 0x87, W_I64SHRU = 0x88,
  W_F32NEG = 0x8c, W_F32ADD = 0x92, W_F32SUB = 0x93, W_F32MUL = 0x94,
  W_F32DIV = 0x95,
  W_F64NEG = 0x9a, W_F64ADD = 0xa0, W_F64SUB = 0xa1, W_F64MUL = 0xa2,
  W_F64DIV = 0xa3,
  W_I32WRAP = 0xa7, W_I64EXTS = 0xac, W_I64EXTU = 0xad,
  W_F32CVTI32 = 0xb2, W_F32CVTI64 = 0xb4, W_F32DEMOTE = 0xb6,
  W_F64CVTI32 = 0xb7, W_F64CVTI64 = 0xb9, W_F64PROMOTE = 0xbb,
  W_I32REINTF = 0xbc, W_I64REINTF = 0xbd, W_F32REINTI = 0xbe,
  W_F64REINTI = 0xbf, W_I32EXT8S = 0xc0, W_I32EXT16S = 0xc1,
  W_PREFIX_FC = 0xfc
};
enum { T_VOIDBLOCK = 0x40, VT_I32 = 0x7f, VT_I64 = 0x7e, VT_F32 = 0x7d,
       VT_F64 = 0x7c };
/* imported functions */
enum { F_METHOD_ENTRY, F_POLL_TICKS, F_EXEC_BYTECODE, F_STALE_EXIT, F_IMPORTS,
       F_ENTRY = F_IMPORTS, F_RUN };

/* -------------------------------------------- reading the method's code */

static inline int u1(address bcp, int bci) { return bcp[bci]; }
static inline int s1(address bcp, int bci) { return (jbyte)bcp[bci]; }
static inline int s2(address bcp, int bci) {
  return (jshort)((bcp[bci] << 8) | bcp[bci + 1]);
}
static inline int u2(address bcp, int bci) {
  return (jushort)((bcp[bci] << 8) | bcp[bci + 1]);
}
static inline int s4(address bcp, int bci) {
  return (jint)(((juint)bcp[bci] << 24) | (bcp[bci + 1] << 16) |
                (bcp[bci + 2] << 8) | bcp[bci + 3]);
}
static inline int u2_native(address bcp, int bci) {
#if HARDWARE_LITTLE_ENDIAN && ENABLE_NATIVE_ORDER_REWRITING
  return (jushort)(bcp[bci] | (bcp[bci + 1] << 8));
#else
  return u2(bcp, bci);
#endif
}
static inline int switch_int(address bcp, int pos) {
  return int_from_addr(bcp + pos);   // as the interpreter reads it
}

/* What one bytecode does, for the analysis and the code generator. */
enum Kind {
  K_SIMPLE,     // straight-line code
  K_BRANCH,     // conditional branch to target
  K_GOTO,       // unconditional jump
  K_SWITCH,     // tableswitch / lookupswitch
  K_RETURN,     // xreturn
  K_CALL,       // done by the interpreter's handler (jit_exec_bytecode)
  K_EXIT        // not compiled: leave to the interpreter here
};

struct Op {
  int code;
  int len;
  int pops, pushes;   // words
  Kind kind;
  int target;         // K_BRANCH / K_GOTO
};

/* words of a result, from Method::return_type() (a storage kind) */
static int return_words(int storage) {
  switch (storage) {
  case Method::NO_RESULT: return 0;
  case Method::FP_SINGLE: case Method::SINGLE: return 1;
  default: return 2;
  }
}

/* parameter and result words of an invoke at bci, or false */
static bool invoke_shape(address method, address bcp, int bci, int code,
                         int* params, int* result) {
  address cpool = get_cpool(method);
  // the interpreter reads the index with GET_SHORT (big endian) for invokes
  int index = u2(bcp, bci + 1);
  address callee = NULL;
  switch (code) {
  case Bytecodes::_fast_invokestatic:
  case Bytecodes::_fast_init_invokestatic:
  case Bytecodes::_fast_invokevirtual_final:
    callee = (address)get_from_cpool(cpool, index);
    break;
  case Bytecodes::_fast_invokevirtual:
  case Bytecodes::_fast_invokespecial: {
    jushort vindex = first_ushort_from_cpool(cpool, index);
    jushort klazz_id = second_ushort_from_cpool(cpool, index);
    address klazz = get_class_by_id(klazz_id);
    address ci = *(address*)(klazz + JavaClass::class_info_offset());
    callee = get_method_from_ci(ci, vindex);
    break;
  }
  case Bytecodes::_fast_invokeinterface: {
    jushort method_index = first_ushort_from_cpool(cpool, index);
    jushort class_id = second_ushort_from_cpool(cpool, index);
    InstanceClass::Raw ic = Universe::class_from_id(class_id);
    ObjArray::Raw methods = ic().methods();
    if (method_index >= methods().length()) return false;
    callee = (address)methods().obj_at(method_index);
    if (callee == NULL) return false;
    if (get_num_params(callee) != u1(bcp, bci + 3)) return false;
    break;
  }
  default:
    return false;
  }
  if (callee == NULL) return false;
  Method::Raw m = (OopDesc*)callee;
  *params = m().size_of_parameters();
  *result = return_words(m().return_type());
  return true;
}

/* Decode the bytecode at bci; false when the analysis cannot go on. */
static bool decode(address method, address bcp, int code_size, int bci,
                   Op* op) {
  const int c = bcp[bci];
  op->code = c;
  op->kind = K_SIMPLE;
  op->target = -1;
  op->pops = op->pushes = 0;
  {
    Method::Raw m = (OopDesc*)method;
    op->len = Bytecodes::length_for(&m(), bci);
  }
  if (op->len <= 0 || bci + op->len > code_size) return false;

#define E(p, q) op->pops = (p); op->pushes = (q); break
  switch (c) {
  case Bytecodes::_nop: E(0, 0);
  case Bytecodes::_aconst_null:
  case Bytecodes::_iconst_m1: case Bytecodes::_iconst_0:
  case Bytecodes::_iconst_1: case Bytecodes::_iconst_2:
  case Bytecodes::_iconst_3: case Bytecodes::_iconst_4:
  case Bytecodes::_iconst_5: case Bytecodes::_fconst_0:
  case Bytecodes::_fconst_1: case Bytecodes::_fconst_2:
  case Bytecodes::_bipush: case Bytecodes::_sipush:
  case Bytecodes::_fast_1_ldc: case Bytecodes::_fast_1_ldc_w:
  case Bytecodes::_iload: case Bytecodes::_fload: case Bytecodes::_aload:
  case Bytecodes::_iload_0: case Bytecodes::_iload_1:
  case Bytecodes::_iload_2: case Bytecodes::_iload_3:
  case Bytecodes::_fload_0: case Bytecodes::_fload_1:
  case Bytecodes::_fload_2: case Bytecodes::_fload_3:
  case Bytecodes::_aload_0: case Bytecodes::_aload_1:
  case Bytecodes::_aload_2: case Bytecodes::_aload_3:
  case Bytecodes::_aload_0_fast_igetfield_1:
  case Bytecodes::_aload_0_fast_agetfield_1:
  case Bytecodes::_aload_0_fast_igetfield_4:
  case Bytecodes::_aload_0_fast_agetfield_4:
  case Bytecodes::_aload_0_fast_igetfield_8:
  case Bytecodes::_aload_0_fast_agetfield_8:
  case Bytecodes::_fast_1_getstatic: case Bytecodes::_fast_init_1_getstatic:
    E(0, 1);
  case Bytecodes::_lconst_0: case Bytecodes::_lconst_1:
  case Bytecodes::_dconst_0: case Bytecodes::_dconst_1:
  case Bytecodes::_fast_2_ldc_w:
  case Bytecodes::_lload: case Bytecodes::_dload:
  case Bytecodes::_lload_0: case Bytecodes::_lload_1:
  case Bytecodes::_lload_2: case Bytecodes::_lload_3:
  case Bytecodes::_dload_0: case Bytecodes::_dload_1:
  case Bytecodes::_dload_2: case Bytecodes::_dload_3:
  case Bytecodes::_fast_2_getstatic: case Bytecodes::_fast_init_2_getstatic:
    E(0, 2);
  case Bytecodes::_iaload: case Bytecodes::_faload: case Bytecodes::_aaload:
  case Bytecodes::_baload: case Bytecodes::_caload: case Bytecodes::_saload:
    E(2, 1);
  case Bytecodes::_laload: case Bytecodes::_daload: E(2, 2);
  case Bytecodes::_istore: case Bytecodes::_fstore: case Bytecodes::_astore:
  case Bytecodes::_istore_0: case Bytecodes::_istore_1:
  case Bytecodes::_istore_2: case Bytecodes::_istore_3:
  case Bytecodes::_fstore_0: case Bytecodes::_fstore_1:
  case Bytecodes::_fstore_2: case Bytecodes::_fstore_3:
  case Bytecodes::_astore_0: case Bytecodes::_astore_1:
  case Bytecodes::_astore_2: case Bytecodes::_astore_3:
  case Bytecodes::_pop:
  case Bytecodes::_fast_1_putstatic: case Bytecodes::_fast_a_putstatic:
  case Bytecodes::_fast_init_1_putstatic:
  case Bytecodes::_fast_init_a_putstatic:
    E(1, 0);
  case Bytecodes::_lstore: case Bytecodes::_dstore:
  case Bytecodes::_lstore_0: case Bytecodes::_lstore_1:
  case Bytecodes::_lstore_2: case Bytecodes::_lstore_3:
  case Bytecodes::_dstore_0: case Bytecodes::_dstore_1:
  case Bytecodes::_dstore_2: case Bytecodes::_dstore_3:
  case Bytecodes::_pop2:
  case Bytecodes::_fast_2_putstatic: case Bytecodes::_fast_init_2_putstatic:
    E(2, 0);
  case Bytecodes::_iastore: case Bytecodes::_fastore: case Bytecodes::_bastore:
  case Bytecodes::_castore: case Bytecodes::_sastore:
    E(3, 0);
  case Bytecodes::_lastore: case Bytecodes::_dastore: E(4, 0);
  case Bytecodes::_dup: E(1, 2);
  case Bytecodes::_dup_x1: E(2, 3);
  case Bytecodes::_dup_x2: E(3, 4);
  case Bytecodes::_dup2: E(2, 4);
  case Bytecodes::_dup2_x1: E(3, 5);
  case Bytecodes::_dup2_x2: E(4, 6);
  case Bytecodes::_swap: E(2, 2);
  case Bytecodes::_iadd: case Bytecodes::_isub: case Bytecodes::_imul:
  case Bytecodes::_iand: case Bytecodes::_ior: case Bytecodes::_ixor:
  case Bytecodes::_ishl: case Bytecodes::_ishr: case Bytecodes::_iushr:
  case Bytecodes::_idiv: case Bytecodes::_irem:
  case Bytecodes::_fadd: case Bytecodes::_fsub: case Bytecodes::_fmul:
  case Bytecodes::_fdiv: case Bytecodes::_fcmpl: case Bytecodes::_fcmpg:
    E(2, 1);
  case Bytecodes::_ladd: case Bytecodes::_lsub: case Bytecodes::_lmul:
  case Bytecodes::_land: case Bytecodes::_lor: case Bytecodes::_lxor:
  case Bytecodes::_ldiv: case Bytecodes::_lrem:
  case Bytecodes::_dadd: case Bytecodes::_dsub: case Bytecodes::_dmul:
  case Bytecodes::_ddiv:
    E(4, 2);
  case Bytecodes::_lshl: case Bytecodes::_lshr: case Bytecodes::_lushr:
    E(3, 2);
  case Bytecodes::_lcmp: case Bytecodes::_dcmpl: case Bytecodes::_dcmpg:
    E(4, 1);
  case Bytecodes::_ineg: case Bytecodes::_fneg:
  case Bytecodes::_i2f: case Bytecodes::_f2i:
  case Bytecodes::_i2b: case Bytecodes::_i2c: case Bytecodes::_i2s:
  case Bytecodes::_arraylength:
  case Bytecodes::_fast_igetfield: case Bytecodes::_fast_fgetfield:
  case Bytecodes::_fast_agetfield: case Bytecodes::_fast_bgetfield:
  case Bytecodes::_fast_sgetfield: case Bytecodes::_fast_cgetfield:
  case Bytecodes::_fast_igetfield_1: case Bytecodes::_fast_agetfield_1:
    E(1, 1);
  case Bytecodes::_iinc: E(0, 0);
  case Bytecodes::_lneg: case Bytecodes::_dneg:
  case Bytecodes::_l2d: case Bytecodes::_d2l:
    E(2, 2);
  case Bytecodes::_i2l: case Bytecodes::_i2d: case Bytecodes::_f2l:
  case Bytecodes::_f2d:
  case Bytecodes::_fast_lgetfield: case Bytecodes::_fast_dgetfield:
    E(1, 2);
  case Bytecodes::_l2i: case Bytecodes::_l2f: case Bytecodes::_d2i:
  case Bytecodes::_d2f:
    E(2, 1);
  case Bytecodes::_fast_iputfield: case Bytecodes::_fast_fputfield:
  case Bytecodes::_fast_aputfield: case Bytecodes::_fast_bputfield:
  case Bytecodes::_fast_sputfield:
    E(2, 0);
  case Bytecodes::_fast_lputfield: case Bytecodes::_fast_dputfield:
    E(3, 0);
  case Bytecodes::_pop_and_npe_if_null: E(1, 0);

  case Bytecodes::_ifeq: case Bytecodes::_ifne: case Bytecodes::_iflt:
  case Bytecodes::_ifge: case Bytecodes::_ifgt: case Bytecodes::_ifle:
  case Bytecodes::_ifnull: case Bytecodes::_ifnonnull:
    op->kind = K_BRANCH; op->target = bci + s2(bcp, bci + 1); E(1, 0);
  case Bytecodes::_if_icmpeq: case Bytecodes::_if_icmpne:
  case Bytecodes::_if_icmplt: case Bytecodes::_if_icmpge:
  case Bytecodes::_if_icmpgt: case Bytecodes::_if_icmple:
  case Bytecodes::_if_acmpeq: case Bytecodes::_if_acmpne:
    op->kind = K_BRANCH; op->target = bci + s2(bcp, bci + 1); E(2, 0);
  case Bytecodes::_goto:
    op->kind = K_GOTO; op->target = bci + s2(bcp, bci + 1); E(0, 0);
  case Bytecodes::_goto_w:
    op->kind = K_GOTO; op->target = bci + s4(bcp, bci + 1); E(0, 0);
  case Bytecodes::_tableswitch: case Bytecodes::_lookupswitch:
    op->kind = K_SWITCH; E(1, 0);
  case Bytecodes::_ireturn: case Bytecodes::_freturn:
  case Bytecodes::_areturn:
    op->kind = K_RETURN; E(1, 0);
  case Bytecodes::_lreturn: case Bytecodes::_dreturn:
    op->kind = K_RETURN; E(2, 0);
  case Bytecodes::_return:
    op->kind = K_RETURN; E(0, 0);

  // done by the interpreter's handler
  case Bytecodes::_fast_invokevirtual: case Bytecodes::_fast_invokestatic:
  case Bytecodes::_fast_init_invokestatic:
  case Bytecodes::_fast_invokevirtual_final:
  case Bytecodes::_fast_invokespecial:
  case Bytecodes::_fast_invokeinterface: {
    int params, result;
    if (!invoke_shape(method, bcp, bci, c, &params, &result)) {
      op->kind = K_EXIT; E(0, 0);
    }
    op->kind = K_CALL; E(params, result);
  }
  case Bytecodes::_new: case Bytecodes::_fast_new:
  case Bytecodes::_fast_init_new:
    op->kind = K_CALL; E(0, 1);
  case Bytecodes::_newarray: case Bytecodes::_anewarray:
  case Bytecodes::_fast_anewarray:
  case Bytecodes::_checkcast: case Bytecodes::_fast_checkcast:
  case Bytecodes::_instanceof: case Bytecodes::_fast_instanceof:
    op->kind = K_CALL; E(1, 1);
  case Bytecodes::_multianewarray:
    op->kind = K_CALL; E(u1(bcp, bci + 3), 1);
  case Bytecodes::_monitorenter: case Bytecodes::_monitorexit:
    op->kind = K_CALL; E(1, 0);
  case Bytecodes::_aastore:
    op->kind = K_CALL; E(3, 0);
  case Bytecodes::_frem: op->kind = K_CALL; E(2, 1);
  case Bytecodes::_drem: op->kind = K_CALL; E(4, 2);
  case Bytecodes::_init_static_array: op->kind = K_CALL; E(1, 1);

  default:
    // athrow, wide, jsr/ret, not yet quickened bytecodes...
    op->kind = K_EXIT; E(0, 0);
  }
#undef E
  return true;
}

/* ------------------------------------------------------------- analysis */

struct Method_ {
  address method, bcp;
  int code_size, max_locals, max_stack;
  signed char* depth;      // stack depth before each bci, -1 unknown
  unsigned char* leader;   // basic block starts
  unsigned char* is_insn;  // instruction starts reached
  unsigned char* backward; // targets of backward branches
  int* block_of;           // bci -> block id
  int* block_bci;          // block id -> bci
  int n_blocks;
  int unquickened;
  unsigned char* written;  // locals that may be stored to
};

static bool is_unquickened(int c) {
  switch (c) {
  case Bytecodes::_ldc: case Bytecodes::_ldc_w: case Bytecodes::_ldc2_w:
  case Bytecodes::_getstatic: case Bytecodes::_putstatic:
  case Bytecodes::_getfield: case Bytecodes::_putfield:
  case Bytecodes::_invokevirtual: case Bytecodes::_invokespecial:
  case Bytecodes::_invokestatic: case Bytecodes::_invokeinterface:
    return true;
  }
  return false;
}

static int local_index_written(address bcp, int bci, int c, int* words) {
  *words = 1;
  switch (c) {
  case Bytecodes::_istore: case Bytecodes::_fstore: case Bytecodes::_astore:
    return u1(bcp, bci + 1);
  case Bytecodes::_lstore: case Bytecodes::_dstore:
    *words = 2; return u1(bcp, bci + 1);
  case Bytecodes::_istore_0: case Bytecodes::_fstore_0:
  case Bytecodes::_astore_0: return 0;
  case Bytecodes::_istore_1: case Bytecodes::_fstore_1:
  case Bytecodes::_astore_1: return 1;
  case Bytecodes::_istore_2: case Bytecodes::_fstore_2:
  case Bytecodes::_astore_2: return 2;
  case Bytecodes::_istore_3: case Bytecodes::_fstore_3:
  case Bytecodes::_astore_3: return 3;
  case Bytecodes::_lstore_0: case Bytecodes::_dstore_0: *words = 2; return 0;
  case Bytecodes::_lstore_1: case Bytecodes::_dstore_1: *words = 2; return 1;
  case Bytecodes::_lstore_2: case Bytecodes::_dstore_2: *words = 2; return 2;
  case Bytecodes::_lstore_3: case Bytecodes::_dstore_3: *words = 2; return 3;
  case Bytecodes::_iinc: return u1(bcp, bci + 1);
  }
  return -1;
}

static void switch_targets(address bcp, int bci, int c, int* deflt,
                           int* n, int* keys, int* targets, int max) {
  int a = (bci + 4) & ~3;
  *deflt = bci + switch_int(bcp, a);
  if (c == Bytecodes::_tableswitch) {
    int lo = switch_int(bcp, a + 4), hi = switch_int(bcp, a + 8);
    int count = hi - lo + 1;
    *n = 0;
    for (int i = 0; i < count && *n < max; i++) {
      keys[*n] = lo + i;
      targets[(*n)++] = bci + switch_int(bcp, a + 12 + i * 4);
    }
    if (count > max) *n = -1;
  } else {
    int npairs = switch_int(bcp, a + 4);
    *n = 0;
    for (int i = 0; i < npairs && *n < max; i++) {
      keys[*n] = switch_int(bcp, a + 8 + i * 8);
      targets[(*n)++] = bci + switch_int(bcp, a + 12 + i * 8);
    }
    if (npairs > max) *n = -1;
  }
}

enum { MAX_SWITCH = 256 };

static int fail_bci, fail_line;
#define FAIL_AT(b) (fail_bci = (b), fail_line = __LINE__, false)

static bool analyze(Method_* M) {
  int size = M->code_size;
  int* work = (int*)malloc(sizeof(int) * (size + 1));
  int nwork = 0;
  bool ok = true;
  memset(M->depth, -1, size);
  M->depth[0] = 0;
  M->leader[0] = 1;
  work[nwork++] = 0;
  static int keys[MAX_SWITCH], targets[MAX_SWITCH];

#define REACH(b, d)                                                    \
  do {                                                                 \
    int b_ = (b), d_ = (d);                                            \
    if (b_ < 0 || b_ >= size) { ok = FAIL_AT(b_); break; }             \
    if (M->depth[b_] == -1) {                                          \
      M->depth[b_] = (signed char)d_; work[nwork++] = b_;              \
    } else if (M->depth[b_] != d_) { ok = FAIL_AT(b_); }               \
  } while (0)

  while (ok && nwork > 0) {
    int bci = work[--nwork];
    // follow straight-line code from bci
    for (;;) {
      Op op;
      int d = M->depth[bci];
      if (!decode(M->method, M->bcp, size, bci, &op)) { ok = FAIL_AT(bci); break; }
      M->is_insn[bci] = 1;
      if (is_unquickened(op.code)) M->unquickened++;
      int words;
      int w = local_index_written(M->bcp, bci, op.code, &words);
      if (w >= 0) {
        if (w + words > M->max_locals) { ok = FAIL_AT(bci); break; }
        M->written[w] = 1;
        if (words == 2) M->written[w + 1] = 1;
      }
      if (op.kind == K_EXIT) break;
      if (d < op.pops) { ok = FAIL_AT(bci); break; }
      int nd = d - op.pops + op.pushes;
      if (nd > M->max_stack || nd > MAX_STACK) { ok = FAIL_AT(bci); break; }
      if (op.kind == K_RETURN) break;
      if (op.kind == K_BRANCH || op.kind == K_GOTO) {
        M->leader[op.target] = 1;
        if (op.target <= bci) M->backward[op.target] = 1;
        REACH(op.target, nd);
        if (op.kind == K_GOTO) break;
        M->leader[bci + op.len] = 1;
      }
      if (op.kind == K_SWITCH) {
        int deflt, n;
        switch_targets(M->bcp, bci, op.code, &deflt, &n, keys, targets,
                       MAX_SWITCH);
        if (n < 0) { ok = FAIL_AT(bci); break; }
        M->leader[deflt] = 1;
        if (deflt <= bci) M->backward[deflt] = 1;
        REACH(deflt, nd);
        for (int i = 0; i < n; i++) {
          M->leader[targets[i]] = 1;
          if (targets[i] <= bci) M->backward[targets[i]] = 1;
          REACH(targets[i], nd);
        }
        break;
      }
      int next = bci + op.len;
      if (next >= size) { ok = FAIL_AT(bci); break; }
      if (M->depth[next] == -1) {
        M->depth[next] = (signed char)nd;
        bci = next;
        continue;
      }
      if (M->depth[next] != nd) ok = FAIL_AT(next);
      break;
    }
  }
#undef REACH
  free(work);
  if (!ok) return false;

  // blocks: leaders that are reached instructions
  M->n_blocks = 0;
  for (int bci = 0; bci < size; bci++) {
    if (M->leader[bci] && M->depth[bci] >= 0) {
      if (!M->is_insn[bci]) return FAIL_AT(bci);
      M->block_of[bci] = M->n_blocks;
      M->block_bci[M->n_blocks++] = bci;
    } else {
      M->block_of[bci] = -1;
    }
  }
  return true;
}

/* -------------------------------------------------------- code generator */

struct Gen {
  Method_* M;
  Buf c;              // the body of run()
  int nest;           // structured nesting level
  int cur_block;
  // wasm locals of run(): 0 = param (block)
  enum { P_BLOCK = 0, L_JFP, L_JLOCALS, L_SBASE, L_CP, L_NEXT, L_XBCI,
         L_XD, L_XSTALE, L_T0, L_T1, L_T2, L_T3, L_T4, L_T5, L_I64A, L_I64B, L_F32,
         L_F64, L_FIRST };
  int L(int n) { return L_FIRST + n; }
  int S(int i) { return L_FIRST + M->max_locals + i; }

  // label levels: done = 1, exit = 2, dispatch = 3, block j at 3 + N - j
  int rel_done()       { return nest - 1; }
  int rel_exit()       { return nest - 2; }
  int rel_dispatch()   { return nest - 3; }
  int rel_block(int j) { return nest - (3 + M->n_blocks - j); }
  int helper_level;   // level of block "handler" of the current call
  int rel_helper()     { return nest - helper_level; }

  void op(int o) { c.byte(o); }
  void lget(int l) { c.byte(W_LGET); c.uleb(l); }
  void lset(int l) { c.byte(W_LSET); c.uleb(l); }
  void ltee(int l) { c.byte(W_LTEE); c.uleb(l); }
  void i32(int v) { c.byte(W_I32CONST); c.sleb(v); }
  void i64(jlong v) { c.byte(W_I64CONST); c.sleb64(v); }
  void mem(int o, int align, int offset) {
    c.byte(o); c.uleb(align); c.uleb(offset);
  }
  void load32(int offset = 0) { mem(W_I32LOAD, 2, offset); }
  void store32(int offset = 0) { mem(W_I32STORE, 2, offset); }
  void load64(int offset = 0) { mem(W_I64LOAD, 3, offset); }
  void store64(int offset = 0) { mem(W_I64STORE, 3, offset); }
  void br(int rel) { c.byte(W_BR); c.uleb(rel); }
  void br_if(int rel) { c.byte(W_BR_IF); c.uleb(rel); }
  void if_() { c.byte(W_IF); c.byte(T_VOIDBLOCK); nest++; }
  void else_() { c.byte(W_ELSE); }
  void end() { c.byte(W_END); nest--; }
  void block() { c.byte(W_BLOCK); c.byte(T_VOIDBLOCK); nest++; }
  void loop() { c.byte(W_LOOP); c.byte(T_VOIDBLOCK); nest++; }
  void call(int f) { c.byte(W_CALL); c.uleb(f); }
  void fc(int sub) { c.byte(W_PREFIX_FC); c.uleb(sub); }

  // address helpers: base + constant (any sign)
  void addr(int base_local, int off) {
    lget(base_local);
    if (off != 0) { i32(off); op(W_I32ADD); }
  }
  void global_addr(const void* p) { i32((int)(uintptr_t)p); }
  void frame_load(int off) { addr(L_JFP, off); load32(); }
  void method_base() {        // the method's bytecode base, from the frame
    frame_load(JavaFrame::method_offset());
    i32(Method::base_offset());
    op(W_I32ADD);
  }

  // slots as long / double
  void get_long(int hi) {     // hi at slot hi, lo at hi + 1
    lget(hi + 1); op(W_I64EXTU);
    lget(hi); op(W_I64EXTU); i64(32); op(W_I64SHL);
    op(W_I64OR);
  }
  void set_long(int hi) {     // i64 on the stack
    ltee(L_I64A); op(W_I32WRAP); lset(hi + 1);
    lget(L_I64A); i64(32); op(W_I64SHRU); op(W_I32WRAP); lset(hi);
  }
  void get_double(int hi) { get_long(hi); op(W_F64REINTI); }
  void set_double(int hi) { op(W_I64REINTF); set_long(hi); }
  void get_float(int l) { lget(l); op(W_F32REINTI); }
  void set_float(int l) { op(W_I32REINTF); lset(l); }

  // leave to the interpreter at bci with depth d (condition on the stack)
  void exit_if(int bci, int d) {
    if_();
    i32(bci); lset(L_XBCI);
    i32(d); lset(L_XD);
    br(rel_exit());
    end();
  }
  void exit_now(int bci, int d) {
    i32(bci); lset(L_XBCI);
    i32(d); lset(L_XD);
    br(rel_exit());
  }
  void null_check(int slot, int bci, int d) {
    lget(slot); op(W_I32EQZ); exit_if(bci, d);
  }

  // write the locals that may differ from memory, and the stack below d
  void write_back(int d) {
    for (int n = 0; n < M->max_locals; n++) {
      if (!M->written[n]) continue;
      lget(L_JLOCALS); i32(4 * n); op(W_I32SUB);
      lget(L(n)); store32();
    }
    for (int i = 0; i < d; i++) {
      lget(L_SBASE); i32(4 * (i + 1)); op(W_I32SUB);
      lget(S(i)); store32();
    }
  }
  void reload(int d) {
    for (int n = 0; n < M->max_locals; n++) {
      lget(L_JLOCALS); i32(4 * n); op(W_I32SUB); load32(); lset(L(n));
    }
    for (int i = 0; i < d; i++) {
      lget(L_SBASE); i32(4 * (i + 1)); op(W_I32SUB); load32(); lset(S(i));
    }
    frame_load(JavaFrame::cpool_offset()); lset(L_CP);
  }
  // the interpreter's globals as the frame is at bci with depth d
  void set_globals(int bci, int d) {
    global_addr(&g_jfp); lget(L_JFP); store32();
    global_addr(&g_jlocals); lget(L_JLOCALS); store32();
    global_addr(&g_jsp); lget(L_SBASE); i32(4 * d); op(W_I32SUB); store32();
    global_addr(&g_jpc); method_base(); i32(bci); op(W_I32ADD); store32();
  }

  // a backward branch: timer tick, then through the dispatcher
  void back_jump(int target) {
    int d = M->depth[target];
    // poll the clock now and then, as check_timer_tick() does
    global_addr(&poll_ticks_countdown);
    global_addr(&poll_ticks_countdown); load32(); i32(1); op(W_I32SUB);
    ltee(L_T0); store32();
    lget(L_T0); i32(0); op(W_I32LTS);
    if_();
      global_addr(&poll_ticks_countdown); i32(1024); store32();
      call(F_POLL_TICKS);
    end();
    global_addr(&_rt_timer_ticks); load32(); i32(0); op(W_I32GTS);
    exit_if(target, d);
    i32(M->block_of[target]); lset(L_NEXT);
    br(rel_dispatch());
  }
  void jump(int bci, int target) {
    if (target <= bci) {
      back_jump(target);
    } else {
      br(rel_block(M->block_of[target]));
    }
  }
  // conditional: the condition is on the stack
  void cond_jump(int bci, int target) {
    if (target <= bci) {
      if_(); back_jump(target); end();
    } else {
      br_if(rel_block(M->block_of[target]));
    }
  }

  void write_barrier_at(int addr_local) {
    // ObjectHeap::set_bit_for(): bit (addr >> 2) of the bit vector
    lget(addr_local); i32(LogBytesPerWord); op(W_I32SHRU); lset(L_T2);
    global_addr(&_bitvector_base); load32();
    lget(L_T2); i32(LogBitsPerWord); op(W_I32SHRU); i32(2); op(W_I32SHL);
    op(W_I32ADD); ltee(L_T1);
    lget(L_T1); load32();
    i32(1); lget(L_T2); i32(BitsPerWord - 1); op(W_I32AND); op(W_I32SHL);
    op(W_I32OR);
    store32();
  }

  // static field address into L_T0, or exit (class not initialized)
  bool static_address(int bci, int d) {
    address cpool = get_cpool(M->method);
    int idx = u2(M->bcp, bci + 1);
    jushort class_id = first_ushort_from_cpool(cpool, idx);
    jushort offset = second_ushort_from_cpool(cpool, idx);
#if ENABLE_ISOLATES
    global_addr(&_mirror_list_base); load32();
    load32(class_id * 4);
    ltee(L_T0);
    global_addr(&_task_class_init_marker); load32();
    op(W_I32EQ);
    exit_if(bci, d);
    lget(L_T0); i32(offset); op(W_I32ADD); lset(L_T0);
    return true;
#else
    (void)class_id; (void)offset;
    return false;
#endif
  }

  void bump(int k) {
    if (!jit_trace) return;
    global_addr(&stats[k]); global_addr(&stats[k]); load32(); i32(1);
    op(W_I32ADD); store32();
  }

  bool gen_insn(int bci, Op& o);
  bool gen_switch(int bci, Op& o, int d);
  void gen_call(int bci, Op& o, int d);
  bool gen_direct_call(int bci, Op& o, int d);
  bool generate();
};

bool Gen::gen_switch(int bci, Op& o, int d) {
  static int keys[MAX_SWITCH], targets[MAX_SWITCH];
  int deflt, n;
  switch_targets(M->bcp, bci, o.code, &deflt, &n, keys, targets, MAX_SWITCH);
  if (n < 0) return false;
  lget(S(d - 1)); lset(L_T0);
  for (int i = 0; i < n; i++) {
    lget(L_T0); i32(keys[i]); op(W_I32EQ);
    cond_jump(bci, targets[i]);
  }
  jump(bci, deflt);
  return true;
}

bool Gen::gen_insn(int bci, Op& o) {
  const int d = M->depth[bci];
  address bcp = M->bcp;
  const int c = o.code;
  const int top = d - 1;     // slot of the top of stack

  switch (c) {
  case Bytecodes::_nop: break;
  case Bytecodes::_aconst_null: i32(0); lset(S(d)); break;
  case Bytecodes::_iconst_m1: case Bytecodes::_iconst_0:
  case Bytecodes::_iconst_1: case Bytecodes::_iconst_2:
  case Bytecodes::_iconst_3: case Bytecodes::_iconst_4:
  case Bytecodes::_iconst_5:
    i32(c - Bytecodes::_iconst_0); lset(S(d)); break;
  case Bytecodes::_fconst_0: i32(0); lset(S(d)); break;
  case Bytecodes::_fconst_1: i32(0x3f800000); lset(S(d)); break;
  case Bytecodes::_fconst_2: i32(0x40000000); lset(S(d)); break;
  case Bytecodes::_lconst_0: case Bytecodes::_lconst_1:
    i32(0); lset(S(d)); i32(c - Bytecodes::_lconst_0); lset(S(d + 1)); break;
  case Bytecodes::_dconst_0:
    i32(0); lset(S(d)); i32(0); lset(S(d + 1)); break;
  case Bytecodes::_dconst_1:
    i32(0x3ff00000); lset(S(d)); i32(0); lset(S(d + 1)); break;
  case Bytecodes::_bipush: i32(s1(bcp, bci + 1)); lset(S(d)); break;
  case Bytecodes::_sipush: i32(s2(bcp, bci + 1)); lset(S(d)); break;
  case Bytecodes::_fast_1_ldc:
    lget(L_CP); load32(u1(bcp, bci + 1) * 4); lset(S(d)); break;
  case Bytecodes::_fast_1_ldc_w:
    lget(L_CP); load32(u2(bcp, bci + 1) * 4); lset(S(d)); break;
  case Bytecodes::_fast_2_ldc_w:
    lget(L_CP); load64(u2(bcp, bci + 1) * 4); set_long(S(d)); break;

  case Bytecodes::_iload: case Bytecodes::_fload: case Bytecodes::_aload:
    lget(L(u1(bcp, bci + 1))); lset(S(d)); break;
  case Bytecodes::_iload_0: case Bytecodes::_iload_1:
  case Bytecodes::_iload_2: case Bytecodes::_iload_3:
    lget(L(c - Bytecodes::_iload_0)); lset(S(d)); break;
  case Bytecodes::_fload_0: case Bytecodes::_fload_1:
  case Bytecodes::_fload_2: case Bytecodes::_fload_3:
    lget(L(c - Bytecodes::_fload_0)); lset(S(d)); break;
  case Bytecodes::_aload_0: case Bytecodes::_aload_1:
  case Bytecodes::_aload_2: case Bytecodes::_aload_3:
    lget(L(c - Bytecodes::_aload_0)); lset(S(d)); break;
  case Bytecodes::_lload: case Bytecodes::_dload: {
    int n = u1(bcp, bci + 1);
    lget(L(n)); lset(S(d)); lget(L(n + 1)); lset(S(d + 1)); break;
  }
  case Bytecodes::_lload_0: case Bytecodes::_lload_1:
  case Bytecodes::_lload_2: case Bytecodes::_lload_3:
  case Bytecodes::_dload_0: case Bytecodes::_dload_1:
  case Bytecodes::_dload_2: case Bytecodes::_dload_3: {
    int n = (c >= Bytecodes::_dload_0) ? c - Bytecodes::_dload_0
                                       : c - Bytecodes::_lload_0;
    lget(L(n)); lset(S(d)); lget(L(n + 1)); lset(S(d + 1)); break;
  }
  case Bytecodes::_istore: case Bytecodes::_fstore: case Bytecodes::_astore:
    lget(S(top)); lset(L(u1(bcp, bci + 1))); break;
  case Bytecodes::_istore_0: case Bytecodes::_istore_1:
  case Bytecodes::_istore_2: case Bytecodes::_istore_3:
    lget(S(top)); lset(L(c - Bytecodes::_istore_0)); break;
  case Bytecodes::_fstore_0: case Bytecodes::_fstore_1:
  case Bytecodes::_fstore_2: case Bytecodes::_fstore_3:
    lget(S(top)); lset(L(c - Bytecodes::_fstore_0)); break;
  case Bytecodes::_astore_0: case Bytecodes::_astore_1:
  case Bytecodes::_astore_2: case Bytecodes::_astore_3:
    lget(S(top)); lset(L(c - Bytecodes::_astore_0)); break;
  case Bytecodes::_lstore: case Bytecodes::_dstore: {
    int n = u1(bcp, bci + 1);
    lget(S(d - 2)); lset(L(n)); lget(S(d - 1)); lset(L(n + 1)); break;
  }
  case Bytecodes::_lstore_0: case Bytecodes::_lstore_1:
  case Bytecodes::_lstore_2: case Bytecodes::_lstore_3:
  case Bytecodes::_dstore_0: case Bytecodes::_dstore_1:
  case Bytecodes::_dstore_2: case Bytecodes::_dstore_3: {
    int n = (c >= Bytecodes::_dstore_0) ? c - Bytecodes::_dstore_0
                                        : c - Bytecodes::_lstore_0;
    lget(S(d - 2)); lset(L(n)); lget(S(d - 1)); lset(L(n + 1)); break;
  }
  case Bytecodes::_iinc: {
    int n = u1(bcp, bci + 1);
    lget(L(n)); i32(s1(bcp, bci + 2)); op(W_I32ADD); lset(L(n)); break;
  }

  // arrays
  case Bytecodes::_iaload: case Bytecodes::_faload: case Bytecodes::_aaload:
  case Bytecodes::_baload: case Bytecodes::_caload: case Bytecodes::_saload:
  case Bytecodes::_laload: case Bytecodes::_daload: {
    int ref = S(d - 2), idx = S(d - 1);
    null_check(ref, bci, d);
    lget(idx); lget(ref); load32(Array::length_offset()); op(W_I32GEU);
    exit_if(bci, d);
    int shift = (c == Bytecodes::_baload) ? 0 :
                (c == Bytecodes::_caload || c == Bytecodes::_saload) ? 1 :
                (c == Bytecodes::_laload || c == Bytecodes::_daload) ? 3 : 2;
    lget(ref); lget(idx);
    if (shift) { i32(shift); op(W_I32SHL); }
    op(W_I32ADD);
    switch (c) {
    case Bytecodes::_baload: mem(W_I32LOAD8S, 0, Array::base_offset()); break;
    case Bytecodes::_caload: mem(W_I32LOAD16U, 1, Array::base_offset()); break;
    case Bytecodes::_saload: mem(W_I32LOAD16S, 1, Array::base_offset()); break;
    case Bytecodes::_laload: case Bytecodes::_daload:
      load64(Array::base_offset()); set_long(S(d - 2)); goto done;
    default: load32(Array::base_offset()); break;
    }
    lset(S(d - 2));
    break;
  }
  case Bytecodes::_iastore: case Bytecodes::_fastore: case Bytecodes::_bastore:
  case Bytecodes::_castore: case Bytecodes::_sastore:
  case Bytecodes::_lastore: case Bytecodes::_dastore: {
    bool wide = (c == Bytecodes::_lastore || c == Bytecodes::_dastore);
    int ref = S(d - (wide ? 4 : 3)), idx = S(d - (wide ? 3 : 2));
    null_check(ref, bci, d);
    lget(idx); lget(ref); load32(Array::length_offset()); op(W_I32GEU);
    exit_if(bci, d);
    int shift = (c == Bytecodes::_bastore) ? 0 :
                (c == Bytecodes::_castore || c == Bytecodes::_sastore) ? 1 :
                wide ? 3 : 2;
    lget(ref); lget(idx);
    if (shift) { i32(shift); op(W_I32SHL); }
    op(W_I32ADD);
    switch (c) {
    case Bytecodes::_bastore:
      lget(S(d - 1)); mem(W_I32STORE8, 0, Array::base_offset()); break;
    case Bytecodes::_castore: case Bytecodes::_sastore:
      lget(S(d - 1)); mem(W_I32STORE16, 1, Array::base_offset()); break;
    case Bytecodes::_lastore: case Bytecodes::_dastore:
      get_long(S(d - 2)); store64(Array::base_offset()); break;
    default:
      lget(S(d - 1)); store32(Array::base_offset()); break;
    }
    break;
  }
  case Bytecodes::_arraylength:
    null_check(S(top), bci, d);
    lget(S(top)); load32(Array::length_offset()); lset(S(top)); break;

  // stack
  case Bytecodes::_pop: case Bytecodes::_pop2: break;
  case Bytecodes::_dup: lget(S(top)); lset(S(d)); break;
  case Bytecodes::_dup_x1:   // a b -> b a b
    lget(S(d - 1)); lset(S(d));
    lget(S(d - 2)); lset(S(d - 1));
    lget(S(d)); lset(S(d - 2));
    break;
  case Bytecodes::_dup_x2:   // a b c -> c a b c
    lget(S(d - 1)); lset(S(d));
    lget(S(d - 2)); lset(S(d - 1));
    lget(S(d - 3)); lset(S(d - 2));
    lget(S(d)); lset(S(d - 3));
    break;
  case Bytecodes::_dup2:     // a b -> a b a b
    lget(S(d - 2)); lset(S(d));
    lget(S(d - 1)); lset(S(d + 1));
    break;
  case Bytecodes::_dup2_x1:  // a b c -> b c a b c
    lget(S(d - 1)); lset(S(d + 1));
    lget(S(d - 2)); lset(S(d));
    lget(S(d - 3)); lset(S(d - 1));
    lget(S(d + 1)); lset(S(d - 2));
    lget(S(d)); lset(S(d - 3));
    break;
  case Bytecodes::_dup2_x2:  // a b c d -> c d a b c d
    lget(S(d - 1)); lset(S(d + 1));
    lget(S(d - 2)); lset(S(d));
    lget(S(d - 3)); lset(S(d - 1));
    lget(S(d - 4)); lset(S(d - 2));
    lget(S(d + 1)); lset(S(d - 3));
    lget(S(d)); lset(S(d - 4));
    break;
  case Bytecodes::_swap:
    lget(S(d - 1)); lset(L_T0);
    lget(S(d - 2)); lset(S(d - 1));
    lget(L_T0); lset(S(d - 2));
    break;

  // int arithmetic
  case Bytecodes::_iadd: case Bytecodes::_isub: case Bytecodes::_imul:
  case Bytecodes::_iand: case Bytecodes::_ior: case Bytecodes::_ixor:
  case Bytecodes::_ishl: case Bytecodes::_ishr: case Bytecodes::_iushr: {
    static const int w[] = { W_I32ADD, W_I32SUB, W_I32MUL, W_I32AND, W_I32OR,
                             W_I32XOR, W_I32SHL, W_I32SHRS, W_I32SHRU };
    int k = c == Bytecodes::_iadd ? 0 : c == Bytecodes::_isub ? 1 :
            c == Bytecodes::_imul ? 2 : c == Bytecodes::_iand ? 3 :
            c == Bytecodes::_ior ? 4 : c == Bytecodes::_ixor ? 5 :
            c == Bytecodes::_ishl ? 6 : c == Bytecodes::_ishr ? 7 : 8;
    lget(S(d - 2)); lget(S(d - 1)); op(w[k]); lset(S(d - 2));
    break;
  }
  case Bytecodes::_idiv: case Bytecodes::_irem:
    lget(S(d - 1)); op(W_I32EQZ); exit_if(bci, d);
    lget(S(d - 1)); i32(-1); op(W_I32EQ);
    if_();
      if (c == Bytecodes::_idiv) { i32(0); lget(S(d - 2)); op(W_I32SUB); }
      else { i32(0); }
      lset(S(d - 2));
    else_();
      lget(S(d - 2)); lget(S(d - 1));
      op(c == Bytecodes::_idiv ? W_I32DIVS : W_I32REMS);
      lset(S(d - 2));
    end();
    break;
  case Bytecodes::_ineg:
    i32(0); lget(S(top)); op(W_I32SUB); lset(S(top)); break;
  case Bytecodes::_i2b: lget(S(top)); op(W_I32EXT8S); lset(S(top)); break;
  case Bytecodes::_i2s: lget(S(top)); op(W_I32EXT16S); lset(S(top)); break;
  case Bytecodes::_i2c:
    lget(S(top)); i32(0xffff); op(W_I32AND); lset(S(top)); break;

  // long arithmetic
  case Bytecodes::_ladd: case Bytecodes::_lsub: case Bytecodes::_lmul:
  case Bytecodes::_land: case Bytecodes::_lor: case Bytecodes::_lxor: {
    int w = c == Bytecodes::_ladd ? W_I64ADD : c == Bytecodes::_lsub ? W_I64SUB :
            c == Bytecodes::_lmul ? W_I64MUL : c == Bytecodes::_land ? W_I64AND :
            c == Bytecodes::_lor ? W_I64OR : W_I64XOR;
    get_long(S(d - 4)); get_long(S(d - 2)); op(w); set_long(S(d - 4));
    break;
  }
  case Bytecodes::_ldiv: case Bytecodes::_lrem:
    get_long(S(d - 2)); op(W_I64EQZ); exit_if(bci, d);
    get_long(S(d - 2)); i64(-1); op(W_I64EQ);
    if_();
      if (c == Bytecodes::_ldiv) { i64(0); get_long(S(d - 4)); op(W_I64SUB); }
      else { i64(0); }
      set_long(S(d - 4));
    else_();
      get_long(S(d - 4)); get_long(S(d - 2));
      op(c == Bytecodes::_ldiv ? W_I64DIVS : W_I64REMS);
      set_long(S(d - 4));
    end();
    break;
  case Bytecodes::_lneg:
    i64(0); get_long(S(d - 2)); op(W_I64SUB); set_long(S(d - 2)); break;
  case Bytecodes::_lshl: case Bytecodes::_lshr: case Bytecodes::_lushr:
    get_long(S(d - 3)); lget(S(d - 1)); op(W_I64EXTU);
    op(c == Bytecodes::_lshl ? W_I64SHL :
       c == Bytecodes::_lshr ? W_I64SHRS : W_I64SHRU);
    set_long(S(d - 3));
    break;

  // float / double
  case Bytecodes::_fadd: case Bytecodes::_fsub: case Bytecodes::_fmul:
  case Bytecodes::_fdiv: {
    int w = c == Bytecodes::_fadd ? W_F32ADD : c == Bytecodes::_fsub ? W_F32SUB :
            c == Bytecodes::_fmul ? W_F32MUL : W_F32DIV;
    get_float(S(d - 2)); get_float(S(d - 1)); op(w); set_float(S(d - 2));
    break;
  }
  case Bytecodes::_dadd: case Bytecodes::_dsub: case Bytecodes::_dmul:
  case Bytecodes::_ddiv: {
    int w = c == Bytecodes::_dadd ? W_F64ADD : c == Bytecodes::_dsub ? W_F64SUB :
            c == Bytecodes::_dmul ? W_F64MUL : W_F64DIV;
    get_double(S(d - 4)); get_double(S(d - 2)); op(w); set_double(S(d - 4));
    break;
  }
  case Bytecodes::_fneg:
    get_float(S(top)); op(W_F32NEG); set_float(S(top)); break;
  case Bytecodes::_dneg:
    get_double(S(d - 2)); op(W_F64NEG); set_double(S(d - 2)); break;
  case Bytecodes::_fcmpl: case Bytecodes::_fcmpg:
  case Bytecodes::_dcmpl: case Bytecodes::_dcmpg: {
    bool dbl = (c == Bytecodes::_dcmpl || c == Bytecodes::_dcmpg);
    int a = S(d - (dbl ? 4 : 2)), b = S(d - (dbl ? 2 : 1));
    #define GA() (dbl ? get_double(a) : get_float(a))
    #define GB() (dbl ? get_double(b) : get_float(b))
    if (c == Bytecodes::_fcmpl || c == Bytecodes::_dcmpl) {
      // (a > b) - !(a >= b)
      GA(); GB(); op(dbl ? W_F64GT : W_F32GT);
      GA(); GB(); op(dbl ? W_F64GE : W_F32GE); op(W_I32EQZ);
      op(W_I32SUB);
    } else {
      // !(a <= b) - (a < b)
      GA(); GB(); op(dbl ? W_F64LE : W_F32LE); op(W_I32EQZ);
      GA(); GB(); op(dbl ? W_F64LT : W_F32LT);
      op(W_I32SUB);
    }
    #undef GA
    #undef GB
    lset(a);
    break;
  }
  case Bytecodes::_i2l: lget(S(top)); op(W_I64EXTS); set_long(S(top)); break;
  case Bytecodes::_i2f: lget(S(top)); op(W_F32CVTI32); set_float(S(top)); break;
  case Bytecodes::_i2d: lget(S(top)); op(W_F64CVTI32); set_double(S(top)); break;
  case Bytecodes::_l2i: lget(S(d - 1)); lset(S(d - 2)); break;   // low word
  case Bytecodes::_l2f:
    get_long(S(d - 2)); op(W_F32CVTI64); set_float(S(d - 2)); break;
  case Bytecodes::_l2d:
    get_long(S(d - 2)); op(W_F64CVTI64); set_double(S(d - 2)); break;
  case Bytecodes::_f2i: get_float(S(top)); fc(0); lset(S(top)); break;
  case Bytecodes::_f2l: get_float(S(top)); fc(4); set_long(S(top)); break;
  case Bytecodes::_f2d:
    get_float(S(top)); op(W_F64PROMOTE); set_double(S(top)); break;
  case Bytecodes::_d2i: get_double(S(d - 2)); fc(2); lset(S(d - 2)); break;
  case Bytecodes::_d2l: get_double(S(d - 2)); fc(6); set_long(S(d - 2)); break;
  case Bytecodes::_d2f:
    get_double(S(d - 2)); op(W_F32DEMOTE); set_float(S(d - 2)); break;

  // fields
  case Bytecodes::_fast_igetfield: case Bytecodes::_fast_fgetfield:
  case Bytecodes::_fast_agetfield: case Bytecodes::_fast_bgetfield:
  case Bytecodes::_fast_sgetfield: case Bytecodes::_fast_cgetfield:
  case Bytecodes::_fast_lgetfield: case Bytecodes::_fast_dgetfield: {
    int off = u2_native(bcp, bci + 1);
    null_check(S(top), bci, d);
    lget(S(top));
    switch (c) {
    case Bytecodes::_fast_bgetfield: mem(W_I32LOAD8S, 0, off); break;
    case Bytecodes::_fast_sgetfield: mem(W_I32LOAD16S, 1, off); break;
    case Bytecodes::_fast_cgetfield: mem(W_I32LOAD16U, 1, off); break;
    case Bytecodes::_fast_lgetfield: case Bytecodes::_fast_dgetfield:
      load64(off * 4); set_long(S(top)); goto done;
    default: load32(off * 4); break;
    }
    lset(S(top));
    break;
  }
  case Bytecodes::_fast_igetfield_1: case Bytecodes::_fast_agetfield_1:
    null_check(S(top), bci, d);
    lget(S(top)); load32(u1(bcp, bci + 1) * 4); lset(S(top)); break;
  case Bytecodes::_aload_0_fast_igetfield_1:
  case Bytecodes::_aload_0_fast_agetfield_1:
  case Bytecodes::_aload_0_fast_igetfield_4:
  case Bytecodes::_aload_0_fast_agetfield_4:
  case Bytecodes::_aload_0_fast_igetfield_8:
  case Bytecodes::_aload_0_fast_agetfield_8: {
    int off = (c == Bytecodes::_aload_0_fast_igetfield_1 ||
               c == Bytecodes::_aload_0_fast_agetfield_1)
              ? u1(bcp, bci + 1) * 4
              : (c == Bytecodes::_aload_0_fast_igetfield_4 ||
                 c == Bytecodes::_aload_0_fast_agetfield_4) ? 4 : 8;
    null_check(L(0), bci, d);
    lget(L(0)); load32(off); lset(S(d));
    break;
  }
  case Bytecodes::_fast_iputfield: case Bytecodes::_fast_fputfield:
  case Bytecodes::_fast_aputfield: case Bytecodes::_fast_bputfield:
  case Bytecodes::_fast_sputfield: {
    int off = u2_native(bcp, bci + 1);
    int obj = S(d - 2);
    null_check(obj, bci, d);
    switch (c) {
    case Bytecodes::_fast_bputfield:
      lget(obj); lget(S(d - 1)); mem(W_I32STORE8, 0, off); break;
    case Bytecodes::_fast_sputfield:
      lget(obj); lget(S(d - 1)); mem(W_I32STORE16, 1, off); break;
    case Bytecodes::_fast_aputfield:
      lget(obj); i32(off * 4); op(W_I32ADD); ltee(L_T0);
      lget(S(d - 1)); store32();
      write_barrier_at(L_T0);
      break;
    default:
      lget(obj); lget(S(d - 1)); store32(off * 4); break;
    }
    break;
  }
  case Bytecodes::_fast_lputfield: case Bytecodes::_fast_dputfield: {
    int off = u2_native(bcp, bci + 1);
    int obj = S(d - 3);
    null_check(obj, bci, d);
    lget(obj); get_long(S(d - 2)); store64(off * 4);
    break;
  }
  case Bytecodes::_pop_and_npe_if_null:
    null_check(S(top), bci, d); break;

  // statics (the field's address is looked up each time: task mirrors)
  case Bytecodes::_fast_1_getstatic: case Bytecodes::_fast_init_1_getstatic:
    if (!static_address(bci, d)) return false;
    lget(L_T0); load32(); lset(S(d)); break;
  case Bytecodes::_fast_2_getstatic: case Bytecodes::_fast_init_2_getstatic:
    if (!static_address(bci, d)) return false;
    lget(L_T0); load64(); set_long(S(d)); break;
  case Bytecodes::_fast_1_putstatic: case Bytecodes::_fast_init_1_putstatic:
    if (!static_address(bci, d)) return false;
    lget(L_T0); lget(S(top)); store32(); break;
  case Bytecodes::_fast_2_putstatic: case Bytecodes::_fast_init_2_putstatic:
    if (!static_address(bci, d)) return false;
    lget(L_T0); get_long(S(d - 2)); store64(); break;
  case Bytecodes::_fast_a_putstatic: case Bytecodes::_fast_init_a_putstatic:
    if (!static_address(bci, d)) return false;
    lget(L_T0); lget(S(top)); store32();
    write_barrier_at(L_T0);
    break;

  // control flow
  case Bytecodes::_ifeq: case Bytecodes::_ifnull:
    lget(S(top)); op(W_I32EQZ); cond_jump(bci, o.target); break;
  case Bytecodes::_ifne: case Bytecodes::_ifnonnull:
    lget(S(top)); cond_jump(bci, o.target); break;
  case Bytecodes::_iflt: lget(S(top)); i32(0); op(W_I32LTS); cond_jump(bci, o.target); break;
  case Bytecodes::_ifge: lget(S(top)); i32(0); op(W_I32GES); cond_jump(bci, o.target); break;
  case Bytecodes::_ifgt: lget(S(top)); i32(0); op(W_I32GTS); cond_jump(bci, o.target); break;
  case Bytecodes::_ifle: lget(S(top)); i32(0); op(W_I32LES); cond_jump(bci, o.target); break;
  case Bytecodes::_if_icmpeq: case Bytecodes::_if_acmpeq:
  case Bytecodes::_if_icmpne: case Bytecodes::_if_acmpne:
  case Bytecodes::_if_icmplt: case Bytecodes::_if_icmpge:
  case Bytecodes::_if_icmpgt: case Bytecodes::_if_icmple: {
    int w = (c == Bytecodes::_if_icmpeq || c == Bytecodes::_if_acmpeq) ? W_I32EQ :
            (c == Bytecodes::_if_icmpne || c == Bytecodes::_if_acmpne) ? W_I32NE :
            c == Bytecodes::_if_icmplt ? W_I32LTS : c == Bytecodes::_if_icmpge ? W_I32GES :
            c == Bytecodes::_if_icmpgt ? W_I32GTS : W_I32LES;
    lget(S(d - 2)); lget(S(d - 1)); op(w); cond_jump(bci, o.target);
    break;
  }
  case Bytecodes::_goto: case Bytecodes::_goto_w:
    jump(bci, o.target); break;
  case Bytecodes::_tableswitch: case Bytecodes::_lookupswitch:
    return gen_switch(bci, o, d);

  default:
    return false;
  }
done:
  return true;
}

/* A call: the bytecode is done by the interpreter's handler, or - for an
 * invoke of a compiled method - by calling its run() directly, after
 * building its frame as interpreter_method_entry() would. Either way the
 * compiled code goes on only if afterwards the state is right after the
 * bytecode in this frame. */
void Gen::gen_call(int bci, Op& o, int d) {
  int after = d - o.pops + o.pushes;
  // too deep on the native stack: let the interpreter call
  global_addr(&nesting); load32(); i32(MAX_NESTING); op(W_I32GES);
  exit_if(bci, d);
  write_back(d);
  block();      // after the call
  block();      // the interpreter's handler
  helper_level = nest;
  if (gen_direct_call(bci, o, d)) {
    br(1);      // done
  }
  end();
  set_globals(bci, d);
  call(F_EXEC_BYTECODE);
  end();
  // still here, just after the bytecode?
  global_addr(&g_jfp); load32(); lget(L_JFP); op(W_I32NE);
  global_addr(&g_jpc); load32(); method_base(); i32(bci + o.len);
  op(W_I32ADD); op(W_I32NE);
  op(W_I32OR);
  if (jit_trace) {
    if_(); bump(ST_CALL_LEFT); br(rel_done()); end();
  } else {
    br_if(rel_done());   // no: the interpreter goes on, as it is
  }
  bump(ST_CALLS);
  // the expression stack may have moved (monitorenter allocates a monitor
  // below it): take its base from the stack pointer
  global_addr(&g_jsp); load32(); i32(4 * after); op(W_I32ADD); lset(L_SBASE);
  reload(after);
}

/* Inside block "handler" (relative 0): branch there when the direct way
 * cannot be taken. Returns false if this invoke has no direct way. */
bool Gen::gen_direct_call(int bci, Op& o, int d) {
  const int code = o.code;
  address cpool = get_cpool(M->method);
  int index = u2(M->bcp, bci + 1);
  const int params = o.pops;
  const int helper = 0;     // relative depth of block "handler" here
  (void)helper;
  // the callee into L_T0
  switch (code) {
  case Bytecodes::_fast_invokestatic:
  case Bytecodes::_fast_init_invokestatic:
  case Bytecodes::_fast_invokevirtual_final:
    lget(L_CP); load32(index * 4); lset(L_T0);
    break;
  case Bytecodes::_fast_invokevirtual: {
    jushort vindex = first_ushort_from_cpool(cpool, index);
    int recv = S(d - params);
    null_check(recv, bci, d);
    // receiver -> near -> class info -> vtable entry
    lget(recv); load32(); load32(JavaNear::class_info_offset());
    load32(ClassInfoDesc::header_size() + vindex * 4);
    lset(L_T0);
    break;
  }
  case Bytecodes::_fast_invokespecial: {
    jushort vindex = first_ushort_from_cpool(cpool, index);
    jushort klazz_id = second_ushort_from_cpool(cpool, index);
    int recv = S(d - params);
    null_check(recv, bci, d);
    global_addr(&_class_list_base); load32(); load32(klazz_id * 4);
    load32(JavaClass::class_info_offset());
    load32(ClassInfoDesc::header_size() + vindex * 4);
    lset(L_T0);
    break;
  }
  default:
    return false;
  }
  // compiled, not synchronized?
  lget(L_T0); load32(Method::variable_part_offset()); load32(); ltee(L_T1);
  global_addr(&first_fn); load32(); op(W_I32LTU);
  br_if(rel_helper());
  lget(L_T0); mem(W_I32LOAD16U, 1, Method::access_flags_offset());
  i32(JVM_ACC_SYNCHRONIZED); op(W_I32AND);
  br_if(rel_helper());
#if ENABLE_ISOLATES
  if (code == Bytecodes::_fast_invokestatic ||
      code == Bytecodes::_fast_init_invokestatic) {
    // the holder's class must be initialized in this task
    global_addr(&_mirror_list_base); load32();
    lget(L_T0); mem(W_I32LOAD16U, 1, Method::holder_id_offset());
    i32(2); op(W_I32SHL); op(W_I32ADD); load32();
    global_addr(&_task_class_init_marker); load32();
    op(W_I32EQ);
    br_if(rel_helper());
  }
#else
  if (code == Bytecodes::_fast_invokestatic ||
      code == Bytecodes::_fast_init_invokestatic) {
    br(rel_helper());
    return true;
  }
#endif
  // the caller's top of stack: L_T2; the callee's locals: L_T3
  lget(L_SBASE); i32(4 * d); op(W_I32SUB); lset(L_T2);
  lget(L_T2);
  lget(L_T0); mem(W_I32LOAD16U, 1, Method::method_attributes_offset());
  i32(Method::SIZE_OF_PARAMETERS_MASK); op(W_I32AND);
  i32(BytesPerStackElement); op(W_I32MUL); op(W_I32ADD);
  i32(JavaFrame::arg_offset_from_sp(-1)); op(W_I32ADD);
  lset(L_T3);
  // the callee's stack pointer after its locals and frame: L_T4
  lget(L_T2);
  lget(L_T0); mem(W_I32LOAD16U, 1, Method::max_locals_offset());
  lget(L_T0); mem(W_I32LOAD16U, 1, Method::method_attributes_offset());
  i32(Method::SIZE_OF_PARAMETERS_MASK); op(W_I32AND);
  op(W_I32SUB); i32(BytesPerStackElement); op(W_I32MUL);
  op(W_I32SUB);
  i32(JavaFrame::frame_desc_size()); op(W_I32SUB);
  lset(L_T4);
  // room for its expression stack?
  lget(L_T4);
  lget(L_T0); mem(W_I32LOAD16U, 1, Method::max_execution_stack_count_offset());
  i32(4); op(W_I32MUL); op(W_I32SUB);
  global_addr(&_current_stack_limit); load32();
  op(W_I32LTU);
  br_if(rel_helper());
  if (jit_trace) bump(ST_DIRECT);

  // the caller's frame, as invoke_java_method() leaves it
  method_base(); i32(bci); op(W_I32ADD); lset(L_T5);     // bcp of the invoke
  addr(L_JFP, JavaFrame::locals_pointer_offset()); lget(L_JLOCALS); store32();
  addr(L_JFP, JavaFrame::bcp_store_offset()); lget(L_T5); store32();
  addr(L_JFP, JavaFrame::stored_int_value1_offset()); i32(o.len); store32();
  // the callee's frame (interpreter_method_entry), fp into L_T1... keep
  // the run function in L_T1: fp goes to L_T2 (the caller's top is done)
  lget(L_T1); i32(1); op(W_I32ADD); lset(L_T1);
  lget(L_T4); i32(JavaFrame::empty_stack_offset()); op(W_I32SUB); lset(L_T2);
  addr(L_T2, JavaFrame::stored_int_value1_offset()); i32(0); store32();
  addr(L_T2, JavaFrame::stored_int_value2_offset()); i32(0); store32();
  addr(L_T2, JavaFrame::stack_bottom_pointer_offset()); lget(L_T4); store32();
  addr(L_T2, JavaFrame::locals_pointer_offset()); lget(L_T3); store32();
  addr(L_T2, JavaFrame::method_offset()); lget(L_T0); store32();
  addr(L_T2, JavaFrame::cpool_offset());
    // get_cpool(): the ROM's shared pool for compressed headers
    i32((int)(uintptr_t)_rom_constant_pool);
    lget(L_T0); load32(Method::constants_offset());
    lget(L_T0); mem(W_I32LOAD16U, 1, Method::access_flags_offset());
    i32(JVM_ACC_HAS_COMPRESSED_HEADER); op(W_I32AND);
    op(W_SELECT);
    i32(ConstantPool::base_offset()); op(W_I32ADD);
  store32();
  addr(L_T2, JavaFrame::caller_fp_offset()); lget(L_JFP); store32();
  addr(L_T2, JavaFrame::return_address_offset()); lget(L_T5); store32();
  // the interpreter's state: at the callee's first bytecode
  global_addr(&g_jfp); lget(L_T2); store32();
  global_addr(&g_jsp); lget(L_T4); store32();
  global_addr(&g_jlocals); lget(L_T3); store32();
  global_addr(&g_jpc); lget(L_T0); i32(Method::base_offset()); op(W_I32ADD);
  store32();
  // run(0)
  i32(0); lget(L_T1);
  c.byte(0x11); c.uleb(1); c.uleb(0);   // call_indirect type 1, table 0
  return true;
}

bool Gen::generate() {
  const int N = M->n_blocks;
  // prologue: state from the interpreter's globals
  global_addr(&nesting); global_addr(&nesting); load32(); i32(1); op(W_I32ADD);
  store32();
  global_addr(&g_jfp); load32(); lset(L_JFP);
  global_addr(&g_jlocals); load32(); lset(L_JLOCALS);
  // entries have an empty stack: the stack base is the stack pointer
  global_addr(&g_jsp); load32(); lset(L_SBASE);
  lget(P_BLOCK); lset(L_NEXT);
  nest = 0;
  bump(ST_RUNS);
  reload(0);

  block();    // done (1)
  block();    // exit (2)
  loop();     // dispatch (3)
  for (int j = N - 1; j >= 0; j--) block();
  // the dispatcher, inside block 0
  lget(L_NEXT);
  c.byte(W_BR_TABLE);
  c.uleb(N);
  for (int j = 0; j < N; j++) c.uleb(rel_block(j));
  c.uleb(rel_block(0));

  for (int j = 0; j < N; j++) {
    end();    // end of block j: its code follows
    cur_block = j;
    int bci = M->block_bci[j];
    int stop = (j + 1 < N) ? M->block_bci[j + 1] : M->code_size;
    for (;;) {
      Op o;
      decode(M->method, M->bcp, M->code_size, bci, &o);
      const int d = M->depth[bci];
      bool fallthrough = true;
      if (o.code == Bytecodes::_lcmp) {
        // (a > b) - (a < b)
        get_long(S(d - 4)); get_long(S(d - 2)); op(W_I64GTS);
        get_long(S(d - 4)); get_long(S(d - 2)); op(W_I64LTS);
        op(W_I32SUB); lset(S(d - 4));
      } else switch (o.kind) {
      case K_EXIT:
        // not quickened yet (never executed): once it is, the method is
        // worth compiling again - the exit tells (jit_stale_exit)
        if (is_unquickened(o.code)) { i32(1); lset(L_XSTALE); }
        exit_now(bci, d);
        fallthrough = false;
        break;
      case K_CALL:
        gen_call(bci, o, d);
        break;
      case K_RETURN: {
        int words = o.pops;
        // monitors on the frame, or back to an EntryFrame: interpreter
        frame_load(JavaFrame::stack_bottom_pointer_offset());
        addr(L_JFP, JavaFrame::empty_stack_offset());
        op(W_I32NE);
        frame_load(JavaFrame::caller_fp_offset()); lset(L_T1);
        addr(L_T1, JavaFrame::return_address_offset()); load32();
        i32((int)EntryFrame::FakeReturnAddress); op(W_I32EQ);
        op(W_I32OR);
        // then the interpreter's own return does it (unlocks, leaves the
        // entry frame) and this code is done: a compiled caller sees its
        // call complete and goes on. Leaving at the return bytecode
        // instead threw every caller of a synchronized method out of
        // compiled code (ByteArrayInputStream.read() under a
        // DataInputStream, for one).
        if_();
          write_back(d);
          set_globals(bci, d);
          call(F_EXEC_BYTECODE);
          br(rel_done());
        end();
        // the caller's state
        frame_load(JavaFrame::locals_pointer_offset());
        i32(JavaFrame::arg_offset_from_sp(-1)); op(W_I32SUB); lset(L_T0);
        global_addr(&g_jfp); lget(L_T1); store32();
        global_addr(&g_jpc);
        addr(L_T1, JavaFrame::bcp_store_offset()); load32();
        addr(L_T1, JavaFrame::stored_int_value1_offset()); load32();
        op(W_I32ADD); store32();
        global_addr(&g_jlocals);
        addr(L_T1, JavaFrame::locals_pointer_offset()); load32(); store32();
        // push the result (as PUSH: deeper word first)
        for (int i = 0; i < words; i++) {
          lget(L_T0); i32(4); op(W_I32SUB); ltee(L_T0);
          lget(S(d - words + i)); store32();
        }
        global_addr(&g_jsp); lget(L_T0); store32();
        bump(ST_RETURNS);
        br(rel_done());
        fallthrough = false;
        break;
      }
      case K_GOTO:
      case K_SWITCH:
        if (!gen_insn(bci, o)) return false;
        fallthrough = false;
        break;
      default:
        if (!gen_insn(bci, o)) {
          exit_now(bci, d);
          fallthrough = false;
        }
        break;
      }
      int next = bci + o.len;
      if (!fallthrough || next >= stop) {
        if (fallthrough) {
          if (next >= M->code_size || M->block_of[next] < 0) return false;
          // falls into the next block (it starts at stop == next)
          if (next != stop) return false;
        }
        break;
      }
      bci = next;
    }
  }
  op(W_UNREACHABLE);
  end();      // dispatch loop
  end();      // exit
  // exit: write back, state at L_XBCI / L_XD
  bump(ST_EXITS);
  if (jit_trace) { global_addr(&stats[ST_LAST_EXIT]); lget(L_XBCI); store32(); }
  write_back(M->max_stack);
  global_addr(&g_jfp); lget(L_JFP); store32();
  global_addr(&g_jlocals); lget(L_JLOCALS); store32();
  global_addr(&g_jsp); lget(L_SBASE); lget(L_XD); i32(2); op(W_I32SHL);
  op(W_I32SUB); store32();
  global_addr(&g_jpc); method_base(); lget(L_XBCI); op(W_I32ADD); store32();
  lget(L_XSTALE);
  if_();
    call(F_STALE_EXIT);
  end();
  end();      // done
  global_addr(&nesting); global_addr(&nesting); load32(); i32(1);
  op(W_I32SUB); store32();
  return nest == 0;
}

/* --------------------------------------------------------- the module */

static void func_body(Buf& code, const Buf& body,
                      const int* local_counts, const int* local_types, int n) {
  Buf f;
  f.uleb(n);
  for (int i = 0; i < n; i++) { f.uleb(local_counts[i]); f.byte(local_types[i]); }
  f.bytes(body);
  f.byte(W_END);
  code.uleb(f.n);
  code.bytes(f);
}

static int build_and_load(Method_* M, Gen& g) {
  Buf m;
  static const unsigned char header[] = { 0, 'a', 's', 'm', 1, 0, 0, 0 };
  for (unsigned i = 0; i < sizeof header; i++) m.byte(header[i]);

  // types: 0 = () -> (), 1 = (i32) -> ()
  Buf t;
  t.uleb(2);
  t.byte(0x60); t.uleb(0); t.uleb(0);
  t.byte(0x60); t.uleb(1); t.byte(VT_I32); t.uleb(0);
  m.section(1, t);

  // imports: memory, then the functions
  Buf im;
  im.uleb(2 + F_IMPORTS);
  im.uleb(1); im.byte('e'); im.uleb(1); im.byte('m');
  im.byte(2); im.byte(0); im.uleb(0);           // memory, min 0
  im.uleb(1); im.byte('e'); im.uleb(1); im.byte('t');
  im.byte(1); im.byte(0x70); im.byte(0); im.uleb(0);   // table, funcref
  for (int i = 0; i < F_IMPORTS; i++) {
    char name[4] = { 'f', (char)('0' + i), 0, 0 };
    im.uleb(1); im.byte('e'); im.uleb(2); im.byte(name[0]); im.byte(name[1]);
    im.byte(0); im.uleb(0);                      // func, type 0
  }
  m.section(2, im);

  // functions: entry (type 0), run (type 1)
  Buf fs;
  fs.uleb(2); fs.uleb(0); fs.uleb(1);
  m.section(3, fs);

  Buf ex;
  ex.uleb(2);
  ex.uleb(5); ex.byte('e'); ex.byte('n'); ex.byte('t'); ex.byte('r'); ex.byte('y');
  ex.byte(0); ex.uleb(F_ENTRY);
  ex.uleb(3); ex.byte('r'); ex.byte('u'); ex.byte('n');
  ex.byte(0); ex.uleb(F_RUN);
  m.section(7, ex);

  Buf code;
  code.uleb(2);
  {
    // entry(): build the frame as the interpreter would, then run block 0
    Buf e;
    // local 0: the caller's frame
    e.byte(W_I32CONST); e.sleb((int)(uintptr_t)&g_jfp);
    e.byte(W_I32LOAD); e.uleb(2); e.uleb(0);
    e.byte(W_LSET); e.uleb(0);
    e.byte(W_CALL); e.uleb(F_METHOD_ENTRY);
    // Go on only in the frame that call has just built: a new frame on top
    // of the caller's, at its first bytecode. interpreter_method_entry()
    // may come back without one - the stack had to grow (and moved), an
    // exception was thrown, or the method is synchronized and its lock is
    // taken: then ANOTHER thread is current now. Looking only at "g_jpc is
    // at the first bytecode of the current frame's method" let this
    // method's code run on the frame of a thread that was about to start
    // (or stood at a loop at bci 0): wrong code on a foreign frame.
    e.byte(W_I32CONST); e.sleb((int)(uintptr_t)&g_jfp);
    e.byte(W_I32LOAD); e.uleb(2); e.uleb(0);
    e.byte(W_I32CONST); e.sleb(JavaFrame::caller_fp_offset());
    e.byte(W_I32ADD);
    e.byte(W_I32LOAD); e.uleb(2); e.uleb(0);
    e.byte(W_LGET); e.uleb(0);
    e.byte(W_I32EQ);
    e.byte(W_I32CONST); e.sleb((int)(uintptr_t)&g_jfp);
    e.byte(W_I32LOAD); e.uleb(2); e.uleb(0);
    e.byte(W_LGET); e.uleb(0);
    e.byte(W_I32NE);
    e.byte(W_I32AND);
    e.byte(W_I32CONST); e.sleb((int)(uintptr_t)&g_jpc);
    e.byte(W_I32LOAD); e.uleb(2); e.uleb(0);
    e.byte(W_I32CONST); e.sleb((int)(uintptr_t)&g_jfp);
    e.byte(W_I32LOAD); e.uleb(2); e.uleb(0);
    e.byte(W_I32CONST); e.sleb(JavaFrame::method_offset());
    e.byte(W_I32ADD);
    e.byte(W_I32LOAD); e.uleb(2); e.uleb(0);
    e.byte(W_I32CONST); e.sleb(Method::base_offset());
    e.byte(W_I32ADD);
    e.byte(W_I32EQ);
    e.byte(W_I32AND);
    e.byte(W_IF); e.byte(T_VOIDBLOCK);
    e.byte(W_I32CONST); e.sleb(0);
    e.byte(W_CALL); e.uleb(F_RUN);
    e.byte(W_END);
    int one_c[1] = { 1 }, one_t[1] = { VT_I32 };
    func_body(code, e, one_c, one_t, 1);
  }
  {
    int counts[5] = { Gen::L_I64A - 1 /* i32 */, 2 /* i64 */, 1, 1,
                      M->max_locals + M->max_stack };
    int types[5] = { VT_I32, VT_I64, VT_F32, VT_F64, VT_I32 };
    // locals 1..L_I64A-1 are i32, then 2 x i64, f32, f64, then the slots
    func_body(code, g.c, counts, types, 5);
  }
  m.section(10, code);

  int imports[F_IMPORTS];
  imports[F_METHOD_ENTRY] = (int)(uintptr_t)&interpreter_method_entry;
  imports[F_POLL_TICKS] = (int)(uintptr_t)&os_emscripten_poll_ticks;
  imports[F_EXEC_BYTECODE] = (int)(uintptr_t)&jit_exec_bytecode_entry;
  imports[F_STALE_EXIT] = (int)(uintptr_t)&jit_stale_exit_entry;
  return fk_jit_load(m.p, m.n, imports, F_IMPORTS);
}

/* ------------------------------------------------------ compile & hooks */

/* a numeric setting: the runtime's environment (the page sets ENV), or
 * under node the process environment; -1 when not set */
EM_JS(int, fk_jit_setting, (const char* name), {
  var n = UTF8ToString(name);
  var v = (typeof ENV !== 'undefined' && ENV[n] !== undefined) ? ENV[n] :
          (typeof process !== 'undefined' && process.env && process.env[n]);
  if (v === undefined || v === false || String(v).length == 0) return -1;
  return parseInt(v, 10) | 0;
});

/* print the statistics when the page / process goes away */
EM_JS(void, fk_jit_report_at_exit, (const int* stats), {
  var report = function() {
    _fk_jit_left_report();
    var p = stats >> 2;
    err('jit: totals (every 5 s, not a compilation): runs ' + HEAP32[p] + ', exits ' + HEAP32[p + 1] +
        ' (last at bci ' + HEAP32[p + 5] + '), calls ' + HEAP32[p + 2] +
        ' on / ' + HEAP32[p + 3] + ' left (' + HEAP32[p + 6] +
        ' direct), returns ' + HEAP32[p + 4]);
  };
  if (typeof process !== 'undefined' && process.on) process.on('exit', report);
  else if (typeof setInterval !== 'undefined') setInterval(report, 5000);
});

static void read_settings() {
  jit_on = fk_jit_setting("PHONEME_JIT") != 0;
  int t = fk_jit_setting("PHONEME_JIT_THRESHOLD");
  if (t > 0) jit_threshold = t;
  jit_trace = fk_jit_setting("PHONEME_JIT_TRACE") > 0;
  jit_rom = fk_jit_setting("PHONEME_JIT_ROM") != 0;
  jit_skip = fk_jit_setting("PHONEME_JIT_SKIP");
  if (jit_trace) fk_jit_report_at_exit(stats);
}

static void print_symbol(OopDesc* sym) {
  if (sym == NULL) return;
  Symbol::Raw s = sym;
  int n = s().length();
  const char* p = s().base_address();
  for (int i = 0; i < n && i < 80; i++) tty->print("%c", p[i]);
}

static void print_method_name(address method) {
  Method::Raw m = (OopDesc*)method;
  JavaClass::Raw k = m().holder();
  if (!k.is_null()) print_symbol(k().name());
  tty->print(".");
  print_symbol(m().name());
}

/* Compile method (global context; no Java allocation happens here).
 * true when its execution entry is now compiled code. */
static bool compile(address method) {
  Method::Raw m = (OopDesc*)method;
  address plain_entry = m().execution_entry();
  if (!is_plain_interpreter_entry(plain_entry)) {
    if (jit_trace) {
      static int told = 0;
      if (told++ < 60) {
        tty->print("jit: no plain entry (%d) ", (int)(uintptr_t)plain_entry);
        print_method_name(method);
        tty->print_cr(" native %d", (int)m().is_native());
      }
    }
    return false;
  }
  if (!jit_rom && !ObjectHeap::contains((OopDesc*)method)) return false;
  // modules are never freed (a method compiled again leaves its old one)
  if (compiled_count >= INFOS - INFOS / 8) return false;
  static int attempt = 0;
  int this_attempt = attempt++;
  if (this_attempt == jit_skip) return false;
  if (m().is_native() || m().is_abstract()) return false;
  int size = m().code_size();
  int max_locals = m().max_locals();
  // the expression stack (and the stack locks of monitorenter); not
  // max_execution_stack_count(), which is this plus the locals: with that
  // every method with more than 64 words of locals + stack - the big inner
  // loops of software renderers - was silently left to the interpreter
  int max_stack = m().max_stack();
  if (size <= 0 || size > MAX_CODE || max_locals > MAX_LOCALS ||
      max_stack > MAX_STACK) {
    if (jit_trace) {
      tty->print("jit: #%d not compiled ", this_attempt);
      print_method_name(method);
      tty->print_cr(" (%d bytes, %d locals, %d stack: too big)", size,
                    max_locals, max_stack);
    }
    return false;
  }

  Method_ M;
  memset(&M, 0, sizeof M);
  M.method = method;
  M.bcp = method + Method::base_offset();
  M.code_size = size;
  M.max_locals = max_locals;
  M.max_stack = max_stack;
  M.depth = (signed char*)malloc(size);
  M.leader = (unsigned char*)calloc(size + 1, 1);
  M.is_insn = (unsigned char*)calloc(size + 1, 1);
  M.backward = (unsigned char*)calloc(size + 1, 1);
  M.block_of = (int*)malloc(sizeof(int) * (size + 1));
  M.block_bci = (int*)malloc(sizeof(int) * (size + 1));
  M.written = (unsigned char*)calloc(max_locals + 1, 1);

  int fn = 0;
  fail_bci = fail_line = -1;
  bool ok = analyze(&M);
  if (ok) {
    Gen g;
    g.M = &M;
    ok = g.generate();
    if (ok) fn = build_and_load(&M, g);
  }
  if (fn > 0) {
    Info* in = (Info*)malloc(sizeof(Info));
    in->entry_fn = fn;
    in->run_fn = fn + 1;
    in->plain_entry = plain_entry;
    in->n_stale = 0;
    for (int bci = 0; bci < size && in->n_stale < Info::MAX_STALE; bci++) {
      if (M.is_insn[bci] && is_unquickened(M.bcp[bci])) {
        in->stale[in->n_stale++] = bci;
      }
    }
    in->n_entries = 0;
    in->entries = (Entry*)malloc(sizeof(Entry) * (M.n_blocks + 1));
    for (int j = 0; j < M.n_blocks; j++) {
      int bci = M.block_bci[j];
      if ((bci == 0 || M.backward[bci]) && M.depth[bci] == 0) {
        in->entries[in->n_entries].bci = bci;
        in->entries[in->n_entries].block = j;
        in->n_entries++;
      }
    }
    add_info(in);
    if (fn < first_fn) first_fn = fn;
    compiled_count++;
    m().set_execution_entry((address)(uintptr_t)fn);
  }
  if (jit_trace) {
    tty->print("jit: #%d %s ", this_attempt, fn > 0 ? "compiled" : "failed");
    print_method_name(method);
    tty->print(" (%d bytes, %d blocks, %d unquickened) exits at:", size,
               M.n_blocks, M.unquickened);
    if (fn <= 0) tty->print(" [failed at bci %d, line %d]", fail_bci, fail_line);
    if (M.depth != NULL) {
      for (int bci = 0; bci < size; bci++) {
        if (!M.is_insn[bci]) continue;
        Op o;
        if (decode(method, M.bcp, size, bci, &o) && o.kind == K_EXIT) {
          tty->print(" %d:%d", bci, o.code);
        }
      }
    }
    int entries = 0;
    for (int bci = 0; bci < size; bci++) if (M.backward[bci]) entries++;
    tty->print_cr(" loops %d", entries);
  }
  free(M.depth); free(M.leader); free(M.is_insn); free(M.backward);
  free(M.block_of); free(M.block_bci); free(M.written);
  return fn > 0;
}

/* how many bytecodes are still to be quickened (postpone compiling) */
static int unquickened_count(address method) {
  Method::Raw m = (OopDesc*)method;
  address bcp = method + Method::base_offset();
  int size = m().code_size();
  int n = 0;
  for (int bci = 0; bci < size; ) {
    if (is_unquickened(bcp[bci])) n++;
    int len = Bytecodes::length_for(&m(), bci);
    if (len <= 0) return 1000;
    bci += len;
  }
  return n;
}

/* called (global context) when a counter reached the threshold */
static void hot(address method) {
  Counter* c = counter_for(method);
  if (unquickened_count(method) > 0 && c->attempts < 4) {
    c->attempts++;
    c->count = 0;
    return;
  }
  if (!compile(method)) {
    c->count = -0x40000000;     // do not try again
  }
}

static void reset_nesting() { nesting = 0; }

/* enter compiled code at a loop header, if it has an entry there */
static bool enter_at(address method, int bci) {
  Info* in = info_for((int)(uintptr_t)method_execution_entry(method));
  if (in == NULL) return false;
  for (int i = 0; i < in->n_entries; i++) {
    if (in->entries[i].bci == bci) {
      ((void (*)(int))(uintptr_t)in->run_fn)(in->entries[i].block);
      return true;
    }
  }
  return false;
}

/* hooks (global context) */
static void hot_invoke(address method) {
  hot(method);
}

/* A loop of a compiled method that runs in the interpreter: its code was
 * not compiled, as it was not quickened yet (or it has no entry). Compile
 * the method again once more of it is quickened. */
static void interpreted_loop(address method) {
  Info* in = info_for((int)(uintptr_t)method_execution_entry(method));
  Counter* c = counter_for(method);
  if (in == NULL || ++c->count < jit_threshold) return;
  c->count = 0;
  address bcp = method + Method::base_offset();
  bool changed = false;
  for (int i = 0; i < in->n_stale; i++) {
    if (!is_unquickened(bcp[in->stale[i]])) changed = true;
  }
  if (!changed) {
    if (in->n_stale == 0) c->count = -0x40000000;  // nothing to gain
    return;
  }
  Method::Raw m = (OopDesc*)method;
  m().set_execution_entry(in->plain_entry);
  if (jit_trace) tty->print_cr("jit: compiling again");
  if (!compile(method)) {
    c->count = -0x40000000;
  }
}

static void backedge(address method, int bci) {
  if ((int)(uintptr_t)method_execution_entry(method) < first_fn) {
    hot(method);
    if ((int)(uintptr_t)method_execution_entry(method) < first_fn) return;
  }
  if (!enter_at(method, bci)) interpreted_loop(method);
}

/* Compiled code left at a bytecode that was not quickened when the method
 * was compiled (global context, the frame is the method's). Without this
 * a method whose loop passes such a bytecode exits there on every
 * iteration for ever: the loop itself is entered in compiled code again,
 * so interpreted_loop() never sees it. */
static void stale_exit() {
  interpreted_loop(*(address*)(g_jfp + JavaFrame::method_offset()));
}

}  // namespace wasm_jit

/* the interpreter's handler for the bytecode at g_jpc (global context) */
/* PHONEME_JIT_TRACE: which calls leave compiled code (bytecode + where the
 * interpreter stands afterwards), the top ones are printed with the stats */
struct LeftCall { char name[72]; unsigned count; };
enum { LEFT_CALLS = 512 };
static LeftCall left_calls[LEFT_CALLS];

static void left_call_note(int opcode, address fp0, address pc0) {
  char name[72];
  int n = 0;
  n += jvm_sprintf(name + n, "%d ", opcode);
  if (g_jfp == fp0) {
    n += jvm_sprintf(name + n, g_jpc == pc0 ? "(same bytecode again)"
                                            : "(same frame, other bci)");
  } else if (*(address*)(g_jfp + JavaFrame::caller_fp_offset()) == fp0) {
    Method::Raw m = *(OopDesc**)(g_jfp + JavaFrame::method_offset());
    JavaClass::Raw k = m().holder();
    Symbol::Raw cn = k.is_null() ? (OopDesc*)NULL : k().name();
    Symbol::Raw mn = m().name();
    if (cn.not_null()) {
      for (int i = 0; i < cn().length() && n < 40; i++) name[n++] = cn().base_address()[i];
    }
    name[n++] = '.';
    if (mn.not_null()) {
      for (int i = 0; i < mn().length() && n < 62; i++) name[n++] = mn().base_address()[i];
    }
    name[n] = 0;
    n += jvm_sprintf(name + n, m().is_native() ? " N" : " %d", (int)m().code_size());
  } else {
    n += jvm_sprintf(name + n, "(other frame: return, exception, thread)");
  }
  unsigned h = 0;
  for (int i = 0; name[i]; i++) h = h * 31 + (unsigned char)name[i];
  for (int i = 0; i < LEFT_CALLS; i++) {
    LeftCall* c = &left_calls[(h + i) % LEFT_CALLS];
    if (c->count == 0) { jvm_strcpy(c->name, name); c->count = 1; return; }
    if (jvm_strcmp(c->name, name) == 0) { c->count++; return; }
  }
}

extern "C" EMSCRIPTEN_KEEPALIVE void fk_jit_left_report() {
  {
    // a line only when methods were compiled since the last report: a log
    // that goes quiet here means the game is warmed up
    static int reported = 0;
    if (wasm_jit::compiled_count != reported) {
      tty->print_cr("jit: %d methods compiled so far (%d since the last totals)",
                    wasm_jit::compiled_count, wasm_jit::compiled_count - reported);
      reported = wasm_jit::compiled_count;
    }
  }
  for (int round = 0; round < 24; round++) {
    LeftCall* best = NULL;
    for (int i = 0; i < LEFT_CALLS; i++) {
      LeftCall* c = &left_calls[i];
      if (c->count != 0 && (c->count & 0x80000000u) == 0 &&
          (best == NULL || c->count > best->count)) best = c;
    }
    if (best == NULL) break;
    tty->print_cr("jit: left %u  %s", best->count, best->name);
    best->count |= 0x80000000u;
  }
  for (int i = 0; i < LEFT_CALLS; i++) left_calls[i].count &= 0x7fffffffu;
}

/* An invoke of a native method that is not a quick one leaves a frame with
 * the method's own two bytecodes (invokenative, return) for the
 * interpreter. Run them here, so that the compiled caller sees its call
 * complete and goes on; anything else - the native blocked the thread,
 * threw, another thread is current - stops at once and the caller leaves,
 * as before. */
static inline void finish_native_call(address fp0) {
  address fp1 = g_jfp;
  if (fp1 == fp0 || *(address*)(fp1 + JavaFrame::caller_fp_offset()) != fp0) return;
  address method = *(address*)(fp1 + JavaFrame::method_offset());
  if ((*(jushort*)(method + Method::access_flags_offset()) & JVM_ACC_NATIVE) == 0) return;
  for (int i = 0; i < 2 && g_jfp == fp1; i++) {
    interpreter_dispatch_table[*g_jpc]();
  }
}

static void jit_exec_bytecode_entry() {
  address fp0 = g_jfp;
  if (wasm_jit::jit_trace) {
    address pc0 = g_jpc;
    int opcode = *g_jpc;
    int len = Bytecodes::length_for((Bytecodes::Code)opcode);
    interpreter_dispatch_table[opcode]();
    finish_native_call(fp0);
    if (g_jfp != fp0 || g_jpc != pc0 + len) left_call_note(opcode, fp0, pc0);
    return;
  }
  interpreter_dispatch_table[*g_jpc]();
  finish_native_call(fp0);
}

static void jit_stale_exit_entry() {
  wasm_jit::stale_exit();
}

#endif // ENABLE_WASM_JIT
