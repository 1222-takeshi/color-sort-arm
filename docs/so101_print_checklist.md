# SO-101 frame printing: checklist (for #3)

Status: checklist template, pre-hardware. The official SO-101 print
data has not been downloaded yet; step 1 creates the part list this
checklist is then filled in against. Nothing is printed until the
owner approves printing (parts, time, and filament are reported
first).

## Step 1: obtain the official data

- Download the SO-101 follower STL set from the official source
  (TheRobotStudio SO-ARM100 project, SO-101 follower parts). Record
  the source URL and the download date in the table below; if the set
  has been revised upstream, note the revision.
- The frame must be the follower configuration (this arm has no
  leader). Camera pan-tilt parts are custom and are NOT part of this
  checklist (see docs/pan_tilt_mount.md).

## Step 2: per-part fit check (Adventurer 3, 150 x 150 x 150 mm)

For every STL: measure the bounding box (slicer or a script), choose
a print orientation, and fill in one row. A part passes if every
dimension is <= 150 mm in the chosen orientation; plan with a 140 mm
working limit so brims and skirts fit. If a part fails, record the
split plan (cut plane + joining method: screws / inserts) instead of
scaling the part - never scale frame parts, the servo pockets and
shaft spacing are dimensioned for the official size.

| # | File | BBox X x Y x Z [mm] | Orientation | Fits <=140 | Est. time | Filament [g] | Status |
|---|---|---|---|---|---|---|---|
| 1 | (fill at download) | | | | | | |

## Step 3: pre-print readiness

- [ ] Filament on hand covers the total estimate from step 2, plus
      one spare part's worth. (The org-side estimate for a full
      SO-101 set was roughly 51 hours of printing; treat as an
      order-of-magnitude reference, not a quote.)
- [ ] Fasteners ready per the official BOM: M3 screws in the lengths
      the BOM lists, plus the brass heat-set inserts used elsewhere
      in this project (HW-0027). Record any missing length here
      before printing starts.
- [ ] Printer check: bed adhesion test / calibration cube within
      tolerance; servo pocket dimensions depend on it.
- [ ] One test print first: the part with a servo pocket, checked
      against a real STS3215 (body 40.7 x 20 mm class pocket fit)
      before printing the whole set.

## Acceptance (from #3, restated)

All frame parts printed and listed in the table, each verified
against the build volume. The filled-in table is pasted into #3.
