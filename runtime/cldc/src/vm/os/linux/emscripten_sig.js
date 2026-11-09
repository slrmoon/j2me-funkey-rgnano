// WebAssembly build of the VM (--pre-js): the real signature of every
// function in the wasm table.
//
// The C interpreter calls VM entries and KNI natives through one generic
// pointer type. Natively a mismatch is harmless (extra arguments, an int
// returned to a void call site...), and phoneME has many: natives declared
// "native int" in Java and KNI_RETURNTYPE_VOID in C, entries that take no
// Thread* in product builds. WebAssembly traps on any of them, so
// invoke_native_entry() asks here what the callee really takes and
// returns, and calls it exactly so.
//
// Module.fkSig(tableIndex) = number of i32 parameters + 16 * result kind
// (0 void, 1 i32, 2 i64, 3 f32, 4 f64); -1 if unknown. The table comes from
// the module's own bytes: Module.wasmBinary (the page fetches it) or, under
// node, the .wasm file.
Module['fkSig'] = (function () {
  var map = null;

  function wasmBytes() {
    if (Module['wasmBinary']) return new Uint8Array(Module['wasmBinary']);
    if (typeof process === 'object' && typeof require === 'function') {
      return new Uint8Array(require('fs').readFileSync(wasmBinaryFile));
    }
    return null;
  }

  function build() {
    var d = wasmBytes(), i = 8, types = [], funcTypes = [], m = new Map();
    if (!d) return m;
    function leb() {
      var r = 0, s = 0, b;
      do { b = d[i++]; r += (b & 0x7f) * Math.pow(2, s); s += 7; } while (b & 0x80);
      return r;
    }
    function sleb() {
      var r = 0, s = 0, b;
      do { b = d[i++]; r |= (b & 0x7f) << s; s += 7; } while (b & 0x80);
      if (s < 32 && (b & 0x40)) r |= -1 << s;
      return r;
    }
    function skipLimits() { var f = d[i++]; leb(); if (f & 1) leb(); }
    var kinds = { 0x7f: 1, 0x7e: 2, 0x7d: 3, 0x7c: 4 };
    while (i < d.length) {
      var id = d[i++], size = leb(), end = i + size, n, k;
      if (id === 1) {                       // types
        for (n = leb(); n > 0; n--) {
          i++;                              // 0x60
          var np = leb(); i += np;
          var nr = leb(), r = nr ? kinds[d[i]] || 0 : 0; i += nr;
          types.push(np + 16 * r);
        }
      } else if (id === 2) {                // imports: functions come first
        for (n = leb(); n > 0; n--) {
          var len = leb(); i += len;        // module name
          len = leb(); i += len;            // field name
          k = d[i++];
          if (k === 0) funcTypes.push(types[leb()]);
          else if (k === 1) { i++; skipLimits(); }
          else if (k === 2) skipLimits();
          else if (k === 3) i += 2;
        }
      } else if (id === 3) {                // functions
        for (n = leb(); n > 0; n--) funcTypes.push(types[leb()]);
      } else if (id === 9) {                // elements: the table
        for (n = leb(); n > 0; n--) {
          var flags = leb();
          if (flags !== 0) break;           // only the MVP form is emitted
          i++;                              // i32.const
          var off = sleb();
          i++;                              // end
          for (var c = leb(), j = 0; j < c; j++) m.set(off + j, funcTypes[leb()]);
        }
      }
      i = end;
    }
    return m;
  }

  return function (index) {
    if (map === null) map = build();
    var v = map.get(index);
    return v === undefined ? -1 : v;
  };
})();
