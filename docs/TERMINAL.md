# Terminal display

The terminal display shows GrADS pictures inside the terminal instead of an
X window. It needs no X server and no `ssh -X`, so it suits work on a remote
cluster from iTerm2: the picture appears in a pane next to the `ga->` prompt
and updates after each command.

```text
┌──────────────────────────┬──────────────────────────┐
│ ga-> sdfopen model.nc    │                          │
│ ga-> set gxout shaded    │      (current plot)      │
│ ga-> d t                 │                          │
│ ga->                     │                          │
└──────────────────────────┴──────────────────────────┘
```

Pictures are drawn with the iTerm2 inline image protocol, which iTerm2 and
WezTerm support. Other terminals ignore it or print noise.

## Quick start over ssh

```bash
ssh cluster
tmux                      # or: tmux attach
./opengrads               # picks the terminal display by itself
```

Inside tmux, GrADS splits a pane off to the right and shows the picture
there. When GrADS quits, the pane closes. Outside tmux, each picture is
printed below the command that drew it instead.

The launcher picks the terminal display when there is no `DISPLAY` and the
terminal is iTerm2 or WezTerm. It detects iTerm2 from `LC_TERMINAL`, which
iTerm2 sets and ssh forwards along with the other `LC_*` variables. If your
ssh or server configuration does not forward it, ask for the terminal
display explicitly:

```bash
OPENGRADS_TERM=1 ./opengrads     # or: ./opengrads -l -d Term
```

`OPENGRADS_TERM=0` turns the automatic choice off. With an X server
available (`ssh -X`), the launcher keeps using the X window unless
`OPENGRADS_TERM=1` is set.

## tmux setup

The viewer pane passes the image through tmux to iTerm2. tmux 3.3 and later
block this unless `allow-passthrough` is on. GrADS turns it on for the
viewer pane only, so no `.tmux.conf` change is needed.

Use ordinary tmux. iTerm2's tmux integration (`tmux -CC`) has not been
tested with inline images.

To keep a shell under the picture, like the lower-right pane in Spyder,
split the viewer pane once GrADS is running:

```bash
tmux split-window -v -d -t '{right}'
```

The picture shrinks to fit the smaller pane.

## Settings

| Variable | Meaning | Default |
|---|---|---|
| `GA_TERM_MODE` | `tmux` (viewer pane), `inline` (print under the command), `file` (only write the PNG), or `auto` | `auto`: `tmux` inside tmux, `inline` elsewhere |
| `GA_TERM_PANE` | Width of the viewer pane | `50%` |
| `GA_TERM_WIDTH` | Width of an inline image, in iTerm2 terms (`70%`, `80` cells, `600px`) | `70%` |
| `GA_TERM_SCALE` | Pixels per point, 1 to 4. 2 keeps lines and text sharp on Retina screens | `2` |
| `GA_TERM_ANIM` | `auto`, `gif`, `live`, or `off`; see [Animation](#animation) | `auto` |
| `GA_TERM_ANIM_DELAY` | Seconds per animation frame | `0.2` |
| `GA_TERM_ANIM_MAX` | Most frames kept in one animation | `300` |
| `GA_TERM_ANIM_SCALE` | Size of animation frames relative to the page, 0.25 to 1 | `1` |
| `GA_TERM_DIR` | Directory that receives `plot.png` and `plot.gif` | a new temporary directory, removed at exit |
| `GA_TERM_VIEWER` | Viewer program for the tmux pane | `libexec/grads-termview`, set by the launcher |
| `GA_TERM_SYNC` | `1` finishes writing each picture before GrADS goes on, for scripts that read `plot.png` at once | off |

The page is 1000 points along its longer side. Change it with `-g`
(`./opengrads -l -d Term -g 1200x900`) or, while running, with
`set xsize 1200 900`.

In `file` mode, GrADS prints where the PNG goes and the viewer command for
it. Run that command in any pane or window that can read the file, for
example a second ssh session.

## When the picture updates

The picture is sent each time GrADS waits for you: at the prompt, at a
script's `pull`, and at a `q pos`. A command that draws nothing (`q dims`)
sends nothing. Drawing many times in one command sends one picture, unless
the command ends frames along the way; see [Animation](#animation).

Encoding happens in a background thread, so the prompt comes back while the
picture is still being written, and the viewer wakes as soon as it is ready
rather than checking on a timer.

## Animation

A frame ends where the picture is replaced: at each `swap` in
double-buffer mode, or when a page with something on it is cleared.

- **Frames are shown as they are made**, in the viewer pane, so a long
  script shows its progress. When frames come faster than they can be sent,
  the viewer skips to the newest.
- **A double-buffered animation loops.** A command that swaps two or more
  frames leaves behind an animated GIF, which iTerm2 plays on its own, over
  and over, with nothing more sent over ssh. This covers the usual GrADS
  idioms:

  ```text
  ga-> set looping on
  ga-> set t 1 24
  ga-> d t
  ```

  and a script loop:

  ```text
  'set dbuff on'
  t = 1
  while (t <= 24)
    'set t 't
    'd t'
    'swap'
    t = t + 1
  endwhile
  ```

`GA_TERM_ANIM` changes this:

| Value | Live frames | Looping GIF |
|---|---|---|
| `auto` | yes | for double-buffered frames |
| `gif` | yes | for any command with two or more frames, cleared ones too |
| `live` | yes | never |
| `off` | no | never; only the picture at the prompt |

Inline mode prints no live frames, which would fill the scrollback, but
does print the looping GIF.

GIF frames are kept at the page size in points (1000 wide by default), with
up to 256 colours each, and only the part of a frame that changed is
stored. As a guide, 100 frames of `d ts` from `pytests/data/model.ctl` came
to 5 MB shaded and 7 MB contoured. Over a slow link, shrink the frames with
`GA_TERM_ANIM_SCALE=0.5`, which roughly halves the size, or keep fewer with
`GA_TERM_ANIM_MAX`. Ctrl-C stops a running animation; the frames made so
far still loop.

## Speed over ssh

What limits a slow link is the amount of data. A full-page picture at the
default `GA_TERM_SCALE=2` is about 300 to 500 KB, sent base64-encoded, which
adds a third.

- Turn on ssh compression (`Compression yes` in `~/.ssh/config`, or
  `ssh -C`). It wins back the base64 overhead.
- `GA_TERM_SCALE=1` sends about 40% of the data, at the cost of softer
  lines on a Retina screen.

iTerm2 and tmux both refuse a single image sequence over 1 MiB, so a larger
picture or animation is sent in parts. iTerm2 understands that from version
3.5; older versions show nothing for such pictures.

## Limits

- **No mouse.** `q pos` shows the picture and waits for Enter instead of a
  click, so scripts that use it to pause still pause. It reports position
  `-999.9 -999.9`.
- **No widgets.** Buttons, drop menus, rubber bands, dialog boxes, and the
  `screen` command need a window. They print a warning and do nothing, as
  they do with the Cairo X display.
- `gxout imap` is not supported, as with the Cairo X display.
- Animations are GIFs: 256 colours per frame and a fixed delay between
  frames. For a movie file, write frames with `gxprint`.
- `gxprint` and `printim` work as usual and are not affected by the display.
