# ECWolf libretro: C++ -> MSVC C89 Migration Plan

Long-term goal: rewrite the engine in MSVC-compatible C89. This is the
dependency-ordered road map. It is derived from an actual scan of the tree
(134 .cpp files, ~94k LOC in src/ excluding libretro-common and tools/rbench),
not from guesswork.

## Guiding principle (what the canvas arc taught us)

Convert **leaves first, foundation last**. A file can only become C89 once
everything it depends on is either already C or is plain data it can access
directly. Converting a heavily-included type (TArray, FString, DObject) before
its consumers means rewriting the foundation with nothing able to compile
against it. The video subsystem proved the method: take one self-contained
unit, turn `class X { methods; data; }` into `struct x_t { data; }` + free
functions, requalify call sites, compile-verify, commit. Repeat.

The discipline that worked, reused here:
  - One subsystem per commit; compile-verify before every commit.
  - Pure-data struct + free functions (the framebuffer_t shape).
  - Keep behavior identical; this is a language port, not a redesign.
  - Where a C++ feature has no C89 equivalent, replace with the established
    shim/pattern (below), never with a half-measure.
  - Runtime smoke-test the risky conversions on hardware.

## The C++ features that must go (and their C89 replacements)

| C++ construct            | Count        | C89 replacement pattern                          |
|--------------------------|--------------|--------------------------------------------------|
| Templates (def sites)    | ~24 files    | concrete types, or macro-generated families      |
| Virtual / inheritance    | 78 files     | tagged unions / function-pointer tables / flatten|
| Operator overloading     | 45 files     | named functions (FString_Cmp, vec_add, ...)      |
| Classes with methods     | most         | struct + free functions (framebuffer_t shape)    |
| new/delete               | pervasive    | malloc/free + explicit init/teardown funcs       |
| References (T&)          | pervasive    | pointers (T*)                                    |
| Default args             | common       | explicit args / wrapper funcs                    |
| C++ // comments in .c    | --           | /* */ (already enforced in converted code)       |
| declarations mid-block   | pervasive    | declarations at top of block (C89 rule)          |

Note: templates with zero instantiations emit no binary, so the *cost* of
leaving an unused template variant in place is zero. The reason to convert
them is the language goal, not performance.

## Dependency ripple (why ordering matters)

Files that break if you touch the header (include count):
  wl_def.h    99   <- the universal include; converts effectively last
  zstring.h   40   (FString: 86 files)   <- core string type
  templates.h 39   (MIN/MAX/clamp etc.)  <- mostly already foldable
  tarray.h    31   (TArray: 55 sites)    <- core container
  actor.h     28
  farchive.h  18   (serialization)
  tmemory.h   16   (TUniquePtr 20 files, TSharedPtr live in 1)
  thinker.h   15
  dobject.h   13   <- GC/reflection root; converts LAST

## Tiers (conversion order)

### TIER 0 - DONE
  - Video subsystem: DCanvas -> framebuffer_t + V_* free functions; IVideo
    factory removed. Plain data + free functions, zero classes. (commits
    in the 008x-009x patch series.)
  - Dead-code removal that shrinks the surface: vlinec4 walls, R_DrawSpan*
    blend variants, FCanvasTexture, mysnprintf shim, dead IVideo stubs.

### TIER A - LEAF VALUE-TYPES (51 files, start here)
Self-contained: no template use, no virtual, no inheritance, no DObject.
Each is a small, independently testable class->struct conversion.

Best first targets (pure value types, smallest blast radius):
  colormatcher.cpp   (FColorMatcher: ctor/op=/Pick - the cleanest leaf)
  files.cpp          (FileReader family - already mostly procedural)
  scanner.cpp        (lexer - self-contained)
  name.cpp           (FName - but note 45 files use FName; convert the
                      impl in place, the type stays until its consumers move)
  language.cpp, m_png.cpp, sfmt/SFMT.cpp, dosbox/dbopl.cpp

Render leaves (already class-free, mostly procedural C already):
  r_2d/r_draw.cpp, r_drawt.cpp, r_main.cpp, r_things.cpp,
  r_data/renderstyle.cpp, v_palette.cpp, v_pfx.cpp, v_text.cpp,
  v_video.cpp, wl_floorceiling.cpp, wl_atmos.cpp, wl_cloudsky.cpp,
  wl_parallax.cpp, wl_dir3dspr.cpp, id_vh.cpp, id_vl.cpp

libretro glue (upstream is already cleaning these):
  libretro/wl_inter.cpp, wl_text.cpp, id_in.cpp, id_us_1.cpp,
  id_sd_adlib.cpp, id_sd_n3dmus.cpp, w32_random.cpp, g_conversation.cpp

Caveat: "Tier A" means the .cpp body is class-light. Several still include
heavy headers (wl_def.h, zstring.h) for types like FString. Those types
can't fully vanish until Tier C, but the file's own classes/methods convert
now, and FString use can be migrated to a C string API incrementally.

### TIER B - USES C++ CONSTRUCTS (56 files)
Use virtual/inheritance/templates but don't DEFINE templates and aren't the
DObject root. Convert after the leaves they lean on are C, and after the
container decisions (Tier C) are made. Examples: the texture loaders
(resourcefiles/*), the menu system, thingdef parsers that lean on TArray/TMap.

### TIER C - CORE CONTAINERS & STRING (the bottleneck)
Convert only once enough Tier A/B consumers are C that there's something to
test against, and once the C replacement patterns are proven.

  - templates.h: MIN/MAX/clamp/etc. - mostly trivial macro/inline-fn folds;
    can be done early and opportunistically.
  - tmemory.h: TUniquePtr (20 files) -> explicit malloc/free + teardown;
    TSharedPtr/TWeakPtr/TSharedPtrRef (1 live use, coupled) -> refcount
    struct + functions, or eliminate the single use first.
  - tarray.h: TArray (55 files, 175 sites) -> a single C dynamic-array
    module (e.g. `struct array { void *data; size_t count, cap, elemsize; }`
    + array_push/array_get/...), or per-type generated arrays. TMap (21
    files) -> a C hash-map module. This is the single largest sub-effort.
  - zstring.h: FString (86 files) -> C string helpers (FString_*) over
    char* with explicit ownership. Pervasive; convert consumers first.
  - farchive.h: FArchive serialization (18 files) -> C serialize functions.

### TIER D - DObject / actor / reflection / GC (20 files, LAST)
dobject.h/.cpp, dobjgc.cpp, thinker.h, actor.h, actordef.h, the a_*.cpp
actor implementations, thingdef. This is the most C++-dependent system:
inheritance trees, RTTI-like reflection (DECLARE/IMPLEMENT_CLASS), garbage
collection, TObjPtr. Nearly everything sits on it, so it converts last.
Likely the largest single design problem (the class hierarchy -> tagged
struct + vtable-of-function-pointers, GC -> explicit ownership or a C GC).

## Recommended immediate next step
Convert `colormatcher.cpp` / `FColorMatcher` -> `struct ColorMatcher` +
`ColorMatcher_*` free functions, exactly as DCanvas became framebuffer_t.
Smallest possible blast radius, re-proves the pattern on a value type, and
starts the Tier A march.

## Reality check
~94k LOC, 134 .cpp files, 78 with inheritance, a GC and a reflection system
to dismantle. This is a multi-phase, long-horizon effort. The value is
realized incrementally: every Tier A/B file converted is permanent progress
and keeps compiling the whole way. Do not start at TArray or DObject.
