# Undo

This fork can step the plot back one command at a time. Undo is **off by
default**; turn it on and choose how many steps to keep with `set undo`.

```text
ga-> set undo 10
Undo is on, keeping up to 10 steps
ga-> set gxout shaded
ga-> display ts
ga-> display ps
ga-> undo
Undid 1 step, 1 of 10 still available
```

## Commands

| Command | Effect |
| --- | --- |
| `set undo <steps>` | Turn undo on and keep that many steps (1 to 10000). |
| `set undo on` | Turn undo on with the default of 10 steps. |
| `set undo off` | Turn undo off and release the stored steps. |
| `set undo 0` | Same as `set undo off`. |
| `undo` | Step the plot back one step. |
| `undo <n>` | Step back up to `n` steps, stopping when none are left. |
| `q undo` | Report whether undo is on, how many steps are available, and how much of the graphics buffer the current plot occupies. |

Changing the step count starts a fresh stack: the steps stored under the old
setting are released.

## What a step is

One step is one command you issue that changes the picture. A command that
draws nothing — `set gxout shaded`, `open`, `q dims` — costs no step, so `undo`
always reaches the last thing that actually appeared.

A script counts as a single step, however much it draws: running
`run plot.gs` and then `undo` removes everything that script drew, not just its
last line. Commands issued through the Python interface are counted
individually, like typed commands.

## What undo restores, and what it does not

GrADS records every graphics primitive of the current plot in its graphics
(meta) buffer, which is what the program replays when a window is exposed or
resized and when `print`, `printim`, or `gxprint` renders. Undo rewinds that
record and replays what is left, so what it leaves is what the shorter command
sequence drew.

Exported output is exact. An image written after an undo is byte-for-byte the
image written by the command sequence without the undone commands, because
printing renders from the same buffer either way.

On screen, undo redraws exactly as GrADS itself redraws. The buffer stores
coordinates in single precision, so a redrawn plot can differ from the original
render by a fraction of a pixel along antialiased edges — the same difference
you already get when a GrADS window is exposed or resized. Measured on a shaded
global field, an undo redraw and a resize redraw of the same plot were
identical to each other, and both differed from the first render only in that
sub-pixel edge shading, invisible at normal viewing.

Undo is a **graphics** operation. It does not revert settings or state:

- Settings keep their current values. `set gxout shaded`, `set lev 500`, and
  `set ccolor 2` are not rolled back, and neither is the dimension
  environment. After an `undo`, re-issuing a `display` draws with the settings
  in force now.
- Open files, defined variables, and `sdfwrite` or shapefile output are
  untouched. `undo` cannot reverse `open`, `close`, `define`, `undefine`, or
  anything written to disk.
- Widgets created by scripts (`draw button`, `draw dropmenu`) are drawn through
  the widget list rather than the graphics buffer, so a rewind does not remove
  them.

## When stored steps are dropped

Stored steps describe the current frame, so anything that resets the frame
drops them. After that, `undo` reports `Nothing to undo` until new drawing
happens. This covers:

- `clear` (and `c`), including the implicit clears of `reinit`.
- `swap`, `set dbuff on`, and `set dbuff off`. Undo and double buffering do not
  mix: in double-buffering mode every frame resets the stack.
- A graphics-buffer allocation failure, which disables buffering for the
  current plot.

`set undo` itself is a session setting: `clear` and `reinit` drop the stored
steps but leave undo on.

## Cost

A stored step is a position in a buffer GrADS maintains anyway, so keeping
steps costs a few bytes each and no extra graphics memory. Rewinding hands the
buffers filled since that position back to GrADS for reuse; nothing is copied.
A large step count is therefore cheap, and the practical limit is 10000.
