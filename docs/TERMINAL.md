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
tmux                      # or: tmux attach, tmux -CC
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

Both ordinary tmux and iTerm2's tmux integration (`tmux -CC`, where tmux
panes become native iTerm2 splits) work; GrADS tells them apart by itself.
No `.tmux.conf` change is needed.

With ordinary tmux, GrADS draws into its pane through tmux to iTerm2. tmux
3.3 and later block this unless `allow-passthrough` is on; GrADS turns it on
for its pane only. tmux does not place such output at the pane by itself,
so each picture carries its own cursor movement to the pane's top-left
corner, worked out from tmux's layout (status line on top included).

**tmux before 3.3** (3.2a is common on clusters, RHEL 9 among them) throws
away everything it holds for your terminal once that is more than 8 bytes
per cell of the terminal, pictures included, and redraws the screen. GrADS
asks tmux which kind it is and, for an older one, sends each picture in
small parts, waiting for tmux to pass each on. On a slow link tmux can still
drop a part. It reports this, and GrADS then repairs the screen and sends the
picture again in smaller parts, which later pictures keep. A note at the
prompt says when this happened. tmux 3.3 or later, or `tmux -CC`, never
drops pictures, and sends them faster.

With `tmux -CC`, iTerm2 draws each pane itself from what runs in it, so
GrADS puts the picture into its pane as a program outside tmux would. This
has been checked against tmux's output in that mode, not yet on iTerm2
itself.

To keep a shell under the picture, like the lower-right pane in Spyder,
split the picture pane once GrADS is running:

```bash
tmux split-window -v -d -t '{right}'
```

The picture is redrawn to fit the smaller pane.

## Settings

| Variable | Meaning | Default |
|---|---|---|
| `GA_TERM_MODE` | `tmux` (picture pane), `inline` (print under the command), `file` (only write the PNG), or `auto` | `auto`: `tmux` inside tmux, `inline` elsewhere |
| `GA_TERM_PANE` | Width of the picture pane | `50%` |
| `GA_TERM_WIDTH` | Width of an inline image, in iTerm2 terms (`70%`, `80` cells, `600px`) | `70%` |
| `GA_TERM_SCALE` | Pixels per point, 1 to 4. 2 keeps lines and text sharp on Retina screens | `2` |
| `GA_TERM_ANIM` | `live`, `gif`, or `off`; see [Animation](#animation) | `live` |
| `GA_TERM_PROGRESS` | `auto`, `on` (for every picture), or `off`; see [Progress bar](#progress-bar) | `auto` |
| `GA_TERM_ANIM_DELAY` | Seconds per frame of a looping GIF | `0.2` |
| `GA_TERM_ANIM_MAX` | Most frames kept in one looping GIF | `300` |
| `GA_TERM_ANIM_SCALE` | Size of looping-GIF frames relative to the page, 0.25 to 1 | `1` |
| `GA_TERM_DIR` | Directory that receives `plot.png` and `plot.gif` | a new temporary directory, removed at exit |
| `GA_TERM_VIEWER` | Program that holds the picture pane open, and shows pictures in `file` mode | `libexec/grads-termview`, set by the launcher |
| `GA_TERM_SYNC` | `1` finishes writing and sending each picture before GrADS goes on, for scripts that read `plot.png` at once | off |
| `GA_TERM_TMUX_STEP` | Bytes handed to tmux at a time before waiting for it to pass them on; `0` for no waiting within a picture | worked out from the tmux version and terminal size |
| `GA_TERM_LOG` | A file to log what tmux reported and how each picture was sent | off |

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
picture is still being written and sent.

## Animation

A frame ends where the picture is replaced: at each `swap` in
double-buffer mode, or when a page with something on it is cleared. As with
an X window, **every frame is shown, in order, as it is drawn**: the usual
GrADS idioms animate in the picture pane step by step.

```text
ga-> set looping on
ga-> set t 1 24
ga-> d t
```

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

When the link is slower than the drawing, the drawing waits for it, as it
would for a forwarded X window, instead of piling pictures up in tmux.

**Ctrl-C** stops the animation: the script ends, frames not yet sent are
dropped, and nothing more is sent for that command. The picture already on
its way finishes, so at most one more arrives. On a slow link, data already
inside ssh (up to its 2 MB window) still has to drain, about 1.6 s at
10 Mbit/s.

`GA_TERM_ANIM` changes this:

| Value | Frames as they are drawn | Afterwards |
|---|---|---|
| `live` (default) | yes | the last frame stays |
| `gif` | yes | a command that swaps two or more frames also leaves a looping GIF, which iTerm2 plays on its own with nothing more sent over ssh |
| `off` | no | only the picture at the prompt |

Inline mode prints no frames as they are drawn, which would fill the
scrollback; it prints the last frame, or the looping GIF with `gif`.

A looping GIF keeps frames at the page size in points (1000 wide by
default), with up to 256 colours each, and stores only the part of a frame
that changed. As a guide, 100 frames of `d ts` from `pytests/data/model.ctl`
came to 5 MB shaded and 7 MB contoured. Shrink them with
`GA_TERM_ANIM_SCALE=0.5`, which roughly halves the size, or keep fewer with
`GA_TERM_ANIM_MAX`.

## Progress bar

With iTerm2, a picture that takes a while to arrive shows iTerm2's own
progress bar while it loads. The progress marks travel between the parts of
the picture, so the bar shows what has actually reached your Mac, not what
has left the server. Until the new picture is complete, the old one stays
up.

With `GA_TERM_PROGRESS=auto` the bar appears for pictures over 1 MiB, and
for every picture once the link has turned out to be slow (GrADS had to
wait for it); fast transfers do not flash a bar. `on` shows it for every
picture, and `off` never; `off` also sends each picture in one piece when
it fits, the oldest and most widely understood form of the protocol, which
is worth trying if pictures do not appear.

## Ctrl-C at the prompt

Ctrl-C while typing a command throws the line away and starts a fresh one,
as a shell does. It never ends GrADS, however often it is pressed; use
`quit`, or Ctrl-\\ to force GrADS to stop. While a command or script runs,
Ctrl-C interrupts it, as before. This holds with any display, not only the
terminal one.

## Speed over ssh

What limits a slow link is the amount of data. A full-page picture at the
default `GA_TERM_SCALE=2` is about 300 to 500 KB, sent base64-encoded, which
adds a third.

- Turn on ssh compression (`Compression yes` in `~/.ssh/config`, or
  `ssh -C`). It wins back the base64 overhead.
- `GA_TERM_SCALE=1` sends about 40% of the data, at the cost of softer
  lines on a Retina screen.

iTerm2 and tmux both refuse a single image sequence over 1 MiB, so with
iTerm2 pictures go in parts, which iTerm2 understands from version 3.5.

## When no picture appears

Start GrADS with a log, draw something, and look at the log:

```bash
GA_TERM_LOG=/tmp/grads-term.log ./opengrads
```

The `start:` line gives the mode and which kind of tmux GrADS found. A
`pane` line says where the picture pane is and which terminal shows it, and
`tmux -CC` when iTerm2's integration is in use. Two `picture:` lines per
picture say how it was sent and how long the link took. `tmux dropped` means
tmux threw part of it away, as tmux before 3.3 does on a slow link.

If the log looks right and still nothing shows, try `GA_TERM_PROGRESS=off`,
which sends a picture in one piece when it fits, the oldest form of the
protocol. Pictures in parts need iTerm2 3.5 or newer.

## Limits

- **No mouse.** `q pos` shows the picture and waits for Enter instead of a
  click, so scripts that use it to pause still pause. It reports position
  `-999.9 -999.9`.
- **No widgets.** Buttons, drop menus, rubber bands, dialog boxes, and the
  `screen` command need a window. They print a warning and do nothing, as
  they do with the Cairo X display.
- `gxout imap` is not supported, as with the Cairo X display.
- tmux redraws a pane from what it knows, and it does not know about the
  picture. After switching tmux windows or reattaching, the pane is blank
  until the next picture or a resize of the pane.
- Inline mode is for use outside tmux: inside tmux the picture is not
  anchored to the scrolling text.
- `gxprint` and `printim` work as usual and are not affected by the display.
