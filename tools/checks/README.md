# tools/checks

Assertions over the real types. **Not built by default** — the target is
`EXCLUDE_FROM_ALL`, so it costs a normal build nothing.

    cmake --build build/<your-kit> --target UIMaker2Checks
    QT_QPA_PLATFORM=offscreen build/<your-kit>/UIMaker2Checks

Exit code is the failure count, so it drops into a git hook or CI unchanged.

Note the target is `EXCLUDE_FROM_ALL`: a plain `make` does **not** rebuild it.
Always name the target, or you will run a stale binary against fresh sources and
believe the wrong result.

## What is here, and why only this

`checks.cpp` — three invariants, each guarding something that has already
shipped a bug or is about to be relied on:

1. **The elements-only row index space.** "Row" means position among child
   *elements*, components not counted. This used to be three hand-copied walks
   in three translation units that merely happened to agree. `UiElement` now
   owns the single definition; this asserts it still matches `ReparentTo`,
   `EntityTreeModel`, and `dropMimeData`'s pre-removal row correction.
2. **Whole-document geometry checksum.** Thirty lines, no framework, and it
   catches every silent regression in the layout pipeline. It is what proved
   the structure-batching change behaviour-preserving.
3. **Anchor forward/inverse round trip.** `SceneElementItem`'s own comment
   claimed the pair were exact inverses while the forward direction applied
   `PixelModel::SnapPoint` and the inverse did not. This check failed on its
   first run with a worst error of 2.0 at unit 4 — exactly half a unit. The
   snap now happens at the call site instead, and the pair really are inverses.

`regressions.cpp` — defects that were fixed and must stay fixed: the
`SceneDocument` teardown order, the `UiBinReader` bounds hardening (a crafted
length used to SIGBUS), the bake round trip, save-in-place, body drags being
owned by the tool system, renames producing an element-level undo record, and
structure batching keeping load linear.

Deliberately **not** here: per-component unit tests (22 of them, each asserting
what its own `Update` body already does — they would encode current behaviour
including the bugs), paint/pixel tests (a maintenance sink with no CI to run
them), and `.uibin` value-fidelity tests (worth writing the day `UiBinWriter`
is touched, not before). Four kinds of check is the right number for a solo
developer with no CI. Add a fifth when something breaks twice.

## One CMake trap worth knowing

The checks target compiles `${UIMAKER_SOURCES}` directly rather than linking a
static library. That is deliberate: `REGISTER_COMPONENT` registers each
component through a file-scope static initialiser, and the linker discards
object files from a static library when nothing references their symbols.
`Component::Registry()` would come back silently **empty** and
`Component::Create` would return nullptr for every kind — with no error, and
tests passing for the wrong reason.
