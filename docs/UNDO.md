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
ga-> clear
ga-> undo
Undid 1 step, 1 of 10 still available
```

The last `undo` brings back the `ts` plot that `clear` removed.

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

`clear` (`c`) is a step too, and so are `clear norset` and `clear graphics`:
undoing it brings the cleared picture back, and the steps taken on that
picture can then be undone in turn.

A script counts as a single step, however much it draws: running
`run plot.gs` and then `undo` removes everything that script drew, not just its
last line. A script that starts with `c`, as many do, is undone back to the
picture from before it. Commands issued through the Python interface are
counted individually, like typed commands.

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

Along with the picture, undo puts back what GrADS knows about it, as the
shorter command sequence would have left it, so that queries and the next
plot see the picture that is on screen:

- what a plot drawn over it builds on: the overlay count, the fixed y-axis
  range of a line plot (a second line plot over a first one shares its axis),
  vector scaling, and the contour-label mask that keeps labels apart;
- what the queries report: `q gxinfo` (last graphic, plot area, axes),
  `q xy2w`, `q w2xy`, `q gr2xy` and the like, `q shades` (which color-bar
  scripts such as `cbarn` read), and `q contours`;
- the coordinate transforms, map projection included.

For example, after `display ts` (a line plot), `display ts*10` and `undo`, the
next line plot gets the axis range it would get on a cleared page, not the
range of the undone plot.

Settings are not rolled back, with two exceptions, both of which make undo
reverse what the command did:

- A display uses up the options that apply to one display only (`set cint`,
  `set clevs`, `set ccols`, `set ccolor`, `set cstyle`, `set cmin`/`cmax`,
  `set rbrange`, `set arrscl` and the like); undoing it gives them back, so
  the display can be issued again as it was.
- A clear resets options such as `set vrange`, `set xlint`, `set xlabs`, and
  `set grads off`; undoing it gives them back.

Either way, an option you have set since keeps your new value. Everything
else keeps its current value:

- `set gxout shaded`, `set lev 500`, and the rest of the dimension
  environment are not rolled back. After an `undo`, re-issuing a `display`
  draws with the settings in force now.
- Open files, defined variables, and `sdfwrite` or shapefile output are
  untouched. `undo` cannot reverse `open`, `close`, `define`, `undefine`, or
  anything written to disk. When a plot's file has been closed or its
  defined variable released since, undo cannot bring back that plot's
  scaling, and `q xy2w` answers `No scaling environment` until the next plot.
- Widgets created by scripts (`draw button`, `draw dropmenu`) are drawn through
  the widget list rather than the graphics buffer, so a rewind does not remove
  them, and undoing a clear does not bring them back.

## When stored steps are dropped

A clear keeps the picture it removes, but other ways of resetting the frame
drop the stored steps. After that, `undo` reports `Nothing to undo` until new
drawing happens. This covers:

- `reset` and `reinit`.
- `swap`, `set dbuff on`, and `set dbuff off`. Undo and double buffering do not
  mix: in double-buffering mode every frame resets the stack, and a clear
  there is not a step.
- `clear hbuff`.
- A graphics-buffer allocation failure, which disables buffering for the
  current plot.

When the stack is full, the oldest step goes, and with it the picture a clear
kept, if that was the step.

`set undo` itself is a session setting: `reinit` drops the stored steps but
leaves undo on.

## Cost

A step that drew is a position in a buffer GrADS maintains anyway, plus what
GrADS knows about the picture from before and after the command: a few KB, a
little more for a page with masked contour labels. Rewinding hands the buffers
filled since that position back to GrADS for reuse; nothing is copied. A clear
keeps the buffers of the picture it removed (at least 1 MB) until its step is
undone or dropped. The practical limit is 10000 steps.
