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
| `GA_TERM_DIR` | Directory that receives `plot.png` | a new temporary directory, removed at exit |
| `GA_TERM_VIEWER` | Viewer program for the tmux pane | `libexec/grads-termview`, set by the launcher |

The page is 1000 points along its longer side. Change it with `-g`
(`./opengrads -l -d Term -g 1200x900`) or, while running, with
`set xsize 1200 900`.

In `file` mode, GrADS prints where the PNG goes and the viewer command for
it. Run that command in any pane or window that can read the file, for
example a second ssh session.

## When the picture updates

The picture is written each time GrADS waits for you: at the prompt, at a
script's `pull`, and at a `q pos`. It is not written after every command, so
a script that draws fifty times produces one picture at the end. A command
that draws nothing (`q dims`) produces no picture.

## Limits

- **No mouse.** `q pos` shows the picture and waits for Enter instead of a
  click, so scripts that use it to pause still pause. It reports position
  `-999.9 -999.9`.
- **No widgets.** Buttons, drop menus, rubber bands, dialog boxes, and the
  `screen` command need a window. They print a warning and do nothing, as
  they do with the Cairo X display.
- `gxout imap` is not supported, as with the Cairo X display.
- An animation (`set dbuff on` in a loop) shows only its last frame. Write
  frames with `gxprint` to make a movie.
- `gxprint` and `printim` work as usual and are not affected by the display.
