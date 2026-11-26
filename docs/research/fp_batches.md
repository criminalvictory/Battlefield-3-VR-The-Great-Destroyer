# First-person batches: the probe and the plan

Research note, 2026-11-25. Status: **the probe is built, read-only and off by default; it has not run in the game yet.**
Everything below about bf3.exe was read in the decrypted image (`build\bf3_mem.exe`, bf3.exe 1.6.0.0, not relocated) and is
checked again by `tools\fpbatchtest.cpp` against that file. What the game does at run time is marked *(to see in the game)*.

The lead (the first-person batch's camera, and its call of the camera setter) came from engine research shared with the owner;
its credit goes in as the owner words it. Every address here was found again and checked in BF3's own code; none of that
research's code was used.

## Why

Two open problems:

- **The floating ACOG** (the bridge M249 in Operation Swordbreaker, 2026-11-19): the sight stays where the game holds the gun
  while the gun itself is drawn in the hand.
- **The dark magazine copy.** The M249's box in the hand is drawn dark (2026-11-18). The mod draws the box off the gun only as a
  copy of its own: the game's box is hidden, and colour and depth are drawn again from palettes made for it. An earlier "black
  copy" of the M416 was its depth-only pass (13080/13080), left where the game held the gun.

Today `skin.cpp` moves a held gun draw by draw. Every part needs its draw known by its index count and start
(`kGunPartDraws`, `kHeldKinds`, the per-gun rows), and every part's place on the gun is learned from a frame where both are
drawn (`g_learned`: a part's bone 0 on the gun's body; `placedLike` for a part only drawn in a later pass). A part whose draw
isn't in a table, or a pass whose offset was never learned, stays at the game's gun: in the sky, or as a dark copy. A part
taken off the gun (a box in the hand) is drawn as a copy.

The engine draws first-person things in **batches**, each with a camera of its own: the first-person field of view, in the
batch's own space about an origin. If a held gun and everything on it (sight, magazine, box, every pass) is one batch, and the
arms are not in it, then moving that batch's camera moves all of it at once. No part tables, no learned offsets.

## What the code says

| Where | What |
|---|---|
| `0x006BBFC0` | A camera block's transform set. Thiscall: ECX the block, one pointer to 16 floats on the stack, `ret 4`. It compares the first three columns of each row with the block's own; if any differs it copies all 16 and ORs 9 into the block's `+0x80` (its matrices are worked out again). 24 callers. |
| `0x010181B0` | The first-person batch's camera (`ret 10h`: the batch, the frame's view list, the view's entry, the draw state). EBX holds the batch from its sixth instruction (`0x010181BD`) to its end. |
| `0x010183B5` → returns to `0x010183BA` | With the batch's flag 1 (and the view allowing it): the view's own first-person camera set as the transform. It is ViewDesc `+0x1080`, which the view builder copies from its source's `+0xD90` (`0x01018FFE`). The batch's FOV (`+0xB4`) is set just before (`0x006BBE00`). |
| `0x0101841D` → returns to `0x01018422` | Without flag 4, and with the batch's origin (`+0x60`) more than about 3 cm from 0 (its x² + y² + z² over the float 0.001 at `0x0207FCFC`, worked out at `0x01018215`, tested at `0x010183C3`): the block's transform less the origin is set. The batch is drawn in its own space about its origin. So the view's own first-person camera stays in the block, and the batch's per-view block is made from it, only with flag 4 or an origin within those 3 cm. Otherwise the transform less the origin is set over it at once. |
| `0x0101ACD0` | The one caller (the call at `0x0101AD74`). For each view entry of the frame's list (0x18 bytes: `+4` the ViewDesc, `+8` a kind, `+0xC` how many batches, `+0x10` their pointers), each batch. |
| ViewDesc `+4` | Its name: "Root", "SecondaryRoot" (the second eye), "RootLeft", "RootRight" (`0x01018E8C`, `0x01019275`). |

The camera is set per **sub-view**: a loop whose number is at `[esp+10h]` and whose last number is at `[esp+1Ch]`. It runs once
for most views. Each sub-view's matrices go into a 0x160-byte block, and the first block becomes the draw state's
per-view block (`[state+0x3A0]+0x20`). 0x160 is 352: the size of the per-view uploads `skin.cpp` already reads
(`viewturn.h`: view at +48, projection +112, view-projection +176, eye +320).

The batch, as `0x010181B0` reads it:

| Offset | What |
|---|---|
| `+0x10` | its transform, four rows of four |
| `+0x60` | its origin |
| `+0x78`, `+0x7C` | its meshes (8 bytes each: the mesh, a word) and how many |
| `+0x80`, `+0x84` | a second list, taken instead in some views (when the byte at `0x02386160` is set) |
| `+0xA0` | a bounding sphere (centre, radius): its distance from the view's cull camera, for its draw distance and level of detail (`0x017A8BA0`) |
| `+0xB4` | its FOV in degrees (-1: the view's own), set in radians and kept between 0.0002 and 3.1415 |
| `+0xBC` | flags: 1 the view's first-person camera and the batch's FOV; 2 its meshes drawn through `0x017326C0` with `+0x70` as a count (instanced, it seems) instead of `0x01732500`; 4 kept at the view's camera, never made relative to its origin (its first mesh drawn with its own bones composed with the transform); 0x10 drawn even in a view that otherwise leaves a flag-1 batch out |

## The probe

`weapon.txt` next to the DLL: the line **`fpbatchlog 1`**, near the top of the file (only its first 1023 bytes are read; a
line past them is never seen). Remove the line, or write `fpbatchlog 0`, to stop it. Without the line nothing is hooked at
all: no cost, and the release's `weapon.txt` never has it.

While it is on, `src/fpbatch.cpp` does this:

- **The hook.** It hooks `0x006BBFC0` with MinHook, but only after checking 19 places of the game's code byte for byte (the
  setter whole, the call sites, the offsets, the origin's test and its 0.001). If any place differs, it logs where and stays
  off.
- **The stub.** The stub (`src/fpbatchstub.h`) looks only at the return address. Every other caller goes straight on with its
  registers and stack untouched. For the two calls above it notes the block, the matrix, EBX (the batch) and EBP (the batch
  camera's frame).
- **The reads.** It reads nothing until the frame proves to be the batch camera's own: its return to `0x0101AD79`, `[ebp+8]`
  equal to EBX, ECX at its camera block, the matrix where that call keeps it. Every read is guarded (a fault is a miss, never
  a crash). These reads aren't checked page by page: they are the game's own structures on the processor's side (the batch
  camera's stack, the view entry, the ViewDesc and its name, the batch and its lists), each found from the one before as the
  batch camera itself reads them, never memory mapped for the graphics card.
- **Never written.** Nothing of the game's is written.
- **A batch's cameras.** Only what its per-view block is made from: the transform less the origin; the view's own first-person
  camera only with flag 4 or an origin within 3 cm. A far-origin batch's view's own camera, set over at once, is never its
  upload's, and every flag-1 batch of the view sets that same camera, so it isn't kept as one of its cameras.
- **Per-view uploads.** Each 352-byte per-view upload that reads as a view (`skin.cpp`'s Unmap, before anything changes it) is
  matched to the batches whose camera it is: its eye within 1 mm of one of a batch's last 8 cameras. Batches of one kind
  (the same flags and meshes: one made again each frame) count as one. If one kind of batch had that camera, the upload is
  that batch's, and this thread's draws after it are its own. If batches of more than one kind had it (the same origin, or
  flag 4 in one view), which batch the upload was can't be told from its camera: the draws after it go into each of those
  batches' lines as **shared**, never to the first.
- **The tag's end.** A thread's draws are counted for a batch from its upload until the first of: the next 352-byte upload;
  the first draw that finds the buffer bound to no vertex-shader slot; the second's end; an 80-byte view-projection in the
  world that comes after the batch's draws. One that comes between the upload and the batch's first draw may be the batch's
  own: it is counted and the tag kept. (The summary says how many came where. After one ended a tag, draws that still find
  the buffer bound are only counted, as nobody's.)
- **Names.** Once a second, the frame's thread looks for mesh names. These are pointers to printable strings among a mesh's
  first 32 words and those of its `+0x10`. This search checks every page first: only pages that read the ordinary, cached
  way are read; write-combined, uncached and guard pages never are (AMD-safe). There are at most 6 new searches a second,
  and the results are kept.
- **What isn't counted.** Only `DrawIndexed` draws (`skin.cpp`'s hook) are counted. A batch drawn instanced (flag 2, it
  seems) shows none.

Once a second, `bf3vr.log` gets lines like these *(the shape; these numbers are made up, from the test)*:

```
FP batches: last second 432 calls of the camera set from the batch camera (216 of the transform made relative to the batch's origin,
  216 of the view's own first-person camera, 0 not in the frame as read), 3 batches kept; per-view uploads 520: 216 a batch's camera
  (0 of them more than one kind of batch's, their draws shared: whose, not known; 0 with other rows), 300 no batch's in the arms' space
  (one with its eye at ...), 4 farther, 0 not read as views; 1600 draws made with a batch's per-view block (0 of them shared), 216
  ending a tag with it not bound; 80-byte view-projections in the world after a batch's upload: 0 before its first draw (the tag
  kept), 216 after its draws (the tag ended: 0 draws after it still with the block bound, not counted)
FP batch 1A2B3C40: flags 0x01, FOV 55.00; in views Root (kind 0, 1 sub-view, 1 view entry), SecondaryRoot (kind 0, 1 sub-view, 1 view
  entry); origin (x y z), its transform's last row (...); its camera less the origin at (...), forward (...); sets: 144 less the
  origin, 144 the view's own (144 in pairs: the view's own less the origin at most 0.0000 m off the other); meshes (+0x78) 2:
  0ABC0000/0 'name' (+0x10>+0x1C), ...; second list (+0x80) 0; per-view uploads its camera's 144; draws: 12768/0 W x144 s32,
  312/12768 W x144 s32, 9273/12 P x144 s32, ...
```

When an upload's camera was more than one kind of batch's, the line says "per-view uploads its camera's N (and M whose camera
another kind of batch had too)", and the draws after those uploads come in a part of their own: "draws after an upload whose
camera another kind of batch had too (whose, not known): ...". The same draws are then in each of those batches' lines.

The letters after a draw are `skin.cpp`'s own name for it:

- `W`: the M416's passes.
- `G`: a held gun's body, colour or depth pass.
- `P`: a part placed as the M416's body is (its ACOG, its launcher).
- `A`: the gloves, arms and body.
- `B`: the bare arms.
- `L`: the SMAW.
- No letter: a draw no table knows. With the batch's stencil (`s32`) it may be the very part that floats.

`sN` is the draw's stencil reference. The game marks first-person draws with 32. A draw with another stencil (`s0`, say) is
not evidence of a floating part: it is most likely something else drawn on the same thread while the batch's buffer was still
bound, after the batch's own draws.

### How to read it

1. **Can the draws be told apart?** The summary's "(N of them more than one kind of batch's ...)" should be 0, and no batch line
   should have "(and M whose camera another kind of batch had too)" or a shared draws part. If they do, those batches share a
   camera (the same origin, or flag 4 in one view). Their shared draws can't say which batch is whose: steps 4 to 6 can't be
   answered from them, only from their own draws. That isn't the same as being one batch: each still has its own pointer and
   meshes (the names say what each holds), and each would still be moved alone.
2. **Is the reading right?** Look at "in pairs: ... at most 0.0000 m off" (a few tenths of a millimetre at most). If it is
   centimetres off, the batch or its origin is misread, and nothing else in the line can be trusted.
3. **Do the per-view uploads match?** If "N a batch's camera" is near the number of camera sets, the 352-byte uploads are the
   inverse of the batch's camera, and the draw lists are each batch's own. If nothing matches, the draw lists are empty and only
   the batches themselves are known. Look at the eye shown for an unmatched upload to see how it differs. The first frame or two
   of each second can miss (the books start empty each second, and drawing runs behind the cameras set).
4. **The gun alone?** One kind of batch with the held gun's draws (`W` or `G`), its parts' (`P`, and keys with no letter and
   `s32`), and no `A`, means the gun is a batch of its own. If the arms are in the same batch, a batch move would carry the
   gloves too.
5. **The attachments?** Are the ACOG's draws (on the M416: 9273/12, 12/0, 72/9285, 129/9357, 108/9486, 9273/9594) in the same
   line as the gun's body? On the bridge M249 its ACOG is the floating one. Is it in the M249's line, or in a line of its own?
6. **The dark copy?** Is the M249 box's draw in the gun's line? Look at its stencil and in how many passes it appears.
7. **The views.** Root and SecondaryRoot are the eyes. Any other name or kind is another view, a shadow map perhaps: whether
   the gun's shadow would follow. Views are told apart by name and kind; "more than 4 view entries" means the frame's view list
   is made again each frame.
8. **Stable pointers?** "(and N more batch pointers ...)" means batches are made again each frame. They are then told apart by
   their meshes, not their pointers.
9. **The tag's end.** The summary's 80-byte view-projections: if they come "before its first draw", they are the batch's own
   and kept; if "after its draws", they ended its tag. If "draws after it still with the block bound" is large while the
   batches' draw lists are short, a batch's own draws were cut: say so with the log.

## The plan, if the batches separate the gun

**The move.** Keep the hook on the call returning to `0x01018422`, but for the held gun's batch alone, hand the setter a moved
camera instead of the game's. In the batch's space (the world less the batch's origin, axes unchanged), let `G` be the gun's
move from where the game holds it to where the hand holds it. `skin.cpp` already works this out in the world
(`skin::GunDeltaWorld`); in the batch's space it is `T(-origin) · G_world · T(origin)`.

With points as columns, a camera `C` (camera to batch space) and its view `V = C⁻¹`, the gun is wanted at `P · V · G · x`.
That is the view `V · G`, so the camera to set is **`C' = G⁻¹ · C`**: the camera moved by the inverse of the gun's move.

Everything drawn with that batch's per-view block then follows the hand at once: every mesh of the batch, every pass that uses
the block, both eyes (each eye's view entry sets its own camera), whatever sight or magazine the mission put on the gun.

**What it would replace:**

- `kGunPartDraws` and `placedLike` (the M416's ACOG and launcher placed as its body is).
- The learned part offsets (`g_learned`'s parts on the body).
- The tables a new gun's sight needs before it stops floating.
- For each new gun: its attachments without a hunt for their draws.

**What it would not replace:**

- Parts that move on the gun or come off it: the charging handle, the M249's lid, a magazine or the M249's box in the other
  hand. Those still need their bones moved in the palette, or a copy drawn, as now.
- The hands. The gloves are posed by `skin.cpp` as now, unless the probe shows them in the gun's batch.
- A gun whose batch has flag 4 (never made relative to its origin) passes only `0x010183BA`, and only with flag 1. The move would
  go there instead.
- A batch with neither (no flag 1, and flag 4 or an origin within 3 cm of 0) passes neither call, and can't be moved this way.

**To find out before building it** *(to see in the game)*:

1. **Lighting.** Moving the camera instead of the mesh leaves the gun where the game had it in the world. The lighting and the
   gun's own shadow may then be worked out for the game's pose, not the hand's: a highlight that doesn't turn with the hand, or
   a shadow in the wrong place. Compare it with today's drawing in the headset.
2. **Depth.** The batch is drawn into the scene's depth with its own projection. A moved camera moves its depth with it,
   consistently, but check the gun against walls up close.
3. **Shadow views.** If the gun's batch is also drawn in a shadow view (another view kind in its line), choose whether that
   view's call moves too.
4. **Recoil, the stand-in hand and the scope's own picture.** These read where the gun is drawn (`GameGunBody`, the sights'
   line). They would read the moved place, so check each.

## The in-game check

**What to run** (the game running, no headset needed):

1. Put the line `fpbatchlog 1` near the top of `weapon.txt` next to the DLL (only its first 1023 bytes are read). The log says
   "FP batches: the probe on ... the game's code as read at 19 places".
2. Hold the M416 with its ACOG for 20 seconds, then aim down the sights.
3. Do a reload by hand.
4. Pick up the bridge M249 (Swordbreaker's overpass), and reload it (the box).
5. Pick up the train pistol, a floor gun, and the SMAW if one is near.
6. Take the line out. The log says "the probe off".

**What to send:** every "FP batch" and "FP batches" line, and a note of what was in the hands when.

**What not to expect:** no change in the picture with the line in or out, and none in the frame rate with it out. With the
line in, the cost is:

- one more check of the return address on every call of the camera set, and for the batch camera's two calls the guarded reads
  and a lock;
- a lock and a short table lookup for each 352-byte per-view upload;
- for each draw while a thread's tag lasts (from a matched upload to its end, above), a query of the 14 vertex-shader constant
  buffer slots (each released again), one of the depth-stencil state, and the lock. The tag ends at the first draw without its
  buffer bound, so after a batch's own draws only those that still find its buffer bound pay it.

The test times a call at about 0.03 µs, a match against full books at about 0.3 µs, and a call passed by at about 15 ns. The
draws' queries can't be timed offline: if the frame rate drops with the line in, that is the probe, and goes with the line.
