## opengrads-hpc 1.0.10

GrADS for modern simulation output: an ADIOS2/BP5 reader, OpenMP-threaded
calculations, and native archives for Linux and macOS.

### Added in 1.0.10

- **Plots in the terminal, without X.** A new display, `-d Term`, draws GrADS
  pictures inside the terminal with the iTerm2 inline image protocol (iTerm2,
  WezTerm), so a session on a cluster needs no X server and no `ssh -X`.
  Inside tmux, GrADS splits a pane off beside the prompt and draws each
  picture there, sized to the pane; outside tmux it prints the picture below
  the command. Pictures are encoded in a background thread, so the prompt
  comes back at once. The launcher picks this display when there is no
  `DISPLAY` and the terminal is iTerm2 or WezTerm, which it learns from
  `LC_TERMINAL` (ssh forwards it); `OPENGRADS_TERM=1` or `0` overrides the
  choice. Both ordinary tmux and iTerm2's tmux integration (`tmux -CC`)
  work, tmux before 3.3 (3.2a, as on RHEL 9 and many clusters) included:
  there, since such a tmux throws away output a slow terminal cannot take,
  each picture goes in small parts. tmux may also skip a picture without a
  word while it waits to redraw; GrADS sets `allow-passthrough all` on its
  pane where tmux has it (3.4 and later), checks with any tmux that each
  picture was passed on, and sends it again if not. A picture drawn while
  its pane is in another tmux window waits until the pane is shown.
  `GA_TERM_LOG=file` records which kind of tmux GrADS found and how each
  picture was sent, for tracking down a picture that does not appear.
  Linux and macOS archives. See [TERMINAL.md](TERMINAL.md).
- **Animations play frame by frame in the terminal.** `set looping on`, or a
  `set dbuff on` loop, shows every frame in order as it is drawn, as an X
  window does. On a slow link the drawing waits for it rather than piling
  pictures up, and Ctrl-C stops the animation and sends nothing more.
  `GA_TERM_ANIM=gif` also leaves a looping GIF, which iTerm2 plays on its
  own.
- **How long a calculation will take.** A calculation that runs for more
  than a second shows a progress line on the terminal: what runs, how far
  along it is, the calculation threads at work, the time taken and about
  how long is left, as in
  `ave [██████░░░░]  35%  1022/2920  4 threads  0:41, about 1:16 left`. It follows
  `ave`, `mean`, `sum`, `sumg`, `min`, `max`, `minloc`, `maxloc`, `tloop`,
  `eloop`, `gint`, and `define`; one inside another counts as part of its
  step (`define > ave`). The line goes away before any other output and when
  the command ends, and never appears in a file, a pipe, or a script's
  `result`. iTerm2 also shows it in its own progress bar. `GA_PROGRESS=off`
  turns it off; a number sets the delay in seconds. Ctrl-C now also stops a
  BP5 time average between the batches of times it reads at once. See
  [PERFORMANCE.md](PERFORMANCE.md).
- **A progress bar for slow pictures.** With iTerm2, a picture over 1 MiB,
  or every picture once the link has proved slow, shows iTerm2's progress
  bar. It is updated between the parts of the picture, so it follows what
  has actually arrived rather than what has left the server.

- **macOS: an X window with XQuartz.** The macOS archive now carries the
  `Cairo` and `X11` displays, so with XQuartz installed `./opengrads` opens
  a GrADS window as on Linux, and `-d X11` picks the classic one. They are
  plug-ins, loaded only when asked for: without XQuartz (no `DISPLAY`) the
  archive runs headless as before, and `printim` and `print` still need
  nothing. Before, `-d X11` stopped at start-up with `Could not find a
  record for the display plug-in`. The packager checks that both load from
  the archive, and draws with them on XQuartz's virtual X server.
- **macOS: plots in the terminal.** The macOS archive also carries the
  terminal display, `-d Term`, so in iTerm2 or WezTerm on a Mac pictures
  appear in the terminal, inside tmux too, without XQuartz. The launcher
  picks it when there is no `DISPLAY` and the terminal is iTerm2 or WezTerm;
  with XQuartz installed the X window wins, and `-d Term` or
  `OPENGRADS_TERM=1` asks for the terminal instead. The terminal display's
  tests, tmux included, now run on macOS too.

### Changed in 1.0.10

- **`clear` can be undone.** With undo on, `clear` (`c`, also `c norset` and
  `c graphics`) is a step of its own: `undo` brings the cleared picture back,
  along with the options the clear reset (`set vrange`, `set xlint`, and the
  like) unless they have been set since, and the steps taken on that picture
  can then be undone in turn. A script that starts with `c` is undone back to
  the picture from before it. Before, a clear dropped every stored step.
  `reset`, `reinit`, and double buffering still do. A clear keeps the
  picture's buffers, at least 1 MB, until its step is undone or dropped.
- **Ctrl-C no longer ends GrADS.** At the prompt it now throws away a
  half-typed command and starts a fresh line, as a shell does. Before, the
  next line was appended to the half-typed one (`d ts`, Ctrl-C, `q dims` ran
  `d tsqdims`), and a second Ctrl-C ended GrADS. While a command or script
  runs, Ctrl-C interrupts it, as before. To force GrADS to stop, use Ctrl-\\.

### Fixed in 1.0.10

- **Plug-in names now match whatever the case.** `-d x11` found no `X11`
  display, and `-h cairo` no `Cairo` printer; display and print plug-in
  names are now compared without regard to case. The error for a display
  that cannot be found also named the print plug-in (`Cairo`) instead of
  the display asked for.

- **Undo left the undone plot's state behind.** Undo rewound the picture but
  not what GrADS knew about it, so after undoing a plot the next one was laid
  out as if it were still there: a line plot kept the undone plot's y-axis
  range, an overlay counted it, `q gxinfo` and `q xy2w` described it, and
  `q shades` (read by `cbarn`) gave its colors. Undo now puts these back as
  the shorter command sequence left them: the overlay count, axis ranges,
  scaling environment, plot area, shading and contour levels, vector
  scaling, coordinate transforms, and the contour-label mask. The options a
  display uses up (`set cint`, `set clevs`, `set ccolor`, ...) come back too,
  so an undone display can be issued again as it was; a setting made after
  the step keeps its new value. See [UNDO.md](UNDO.md).
- **Linux: an old OpenGrADS install next to the archive broke it.** The
  launcher looked beside itself for a legacy `opengrads-2.2.1.oga.1` bundle,
  a convenience meant for a source checkout, and in a packaged archive it
  adopted that install's plug-ins and extensions in place of its own. Those
  were built for other libraries, so GrADS stopped at start-up with
  `dlopen failed to get a handle on gxprint plug-in named "Cairo"` and
  `libpng15.so.15: cannot open shared object file`. A packaged archive now
  uses only its own plug-ins; `OPENGRADS_BUNDLE_ROOT` still selects a bundle
  explicitly. The packager checks this by starting the archive beside a
  decoy install. With 1.0.9, move the archive (or the old install) so the two
  are not in the same directory.
- **`-b` over time weighted every time but the last wrongly.** `ave`, `mean`,
  and `sum` with the boundary flag are meant to count each time by how much
  of its cell lies between the two bounds: times inside fully, the first and
  last in part, as they already did over longitude, latitude, and level. Over
  time, every time instead got the weight `gr2 + 0.5 - t`, larger the further
  it lay from the end. On six times, `sum(var,t=1.5,t=5.5,-b)` weighted times
  2 to 5 by 4, 3, 2, 1 instead of 1, 1, 1, 1, so the sum of a constant came
  out two and a half times too large and `ave` leaned toward the early
  times. Each time now counts by its overlap with the bounds, in the
  step-by-step path and in the many-steps-at-once path for BP5 alike. Without
  `-b`, and for `sumg`, `min`, `max`, `minloc`, and `maxloc`, which do not
  weight, nothing changes. The fault is in GrADS 2.2.1 and is still there in
  2.2.3, so results from those versions with `-b` over time differ from these.
  This fix was listed under 1.0.9, but it came after that release was
  built; the 1.0.9 archives still have the old weights.

### Added in 1.0.9

- **Plotting scripts from bGASL.** Thirteen scripts from Bin Guan's GrADS
  Script Library (BSD 2-Clause) now ship in `lib/scripts`: `plot` for 1-D
  graphs and profiles, `shadcon` for shading and contours, `vcr` for vertical
  cross-sections, `vector`, `subplot` for multi-panel figures, `legend`,
  `drawstr`, `drawline`, `drawbox`, `drawmark`, `ppp` for cropped
  publication output (needs ghostscript), `save`, and `bhist` (bGASL's
  histogram calculator, renamed to leave the existing `hist` plotter in
  place). `subplot` replaces the earlier, much smaller script of that name and
  takes different arguments: `subplot <panels> <index> [<columns>]` instead of
  `subplot <rows> <columns> <index>`. See `THIRD_PARTY_NOTICES.md`.
- **`sdfopen` opens files without an X or a Y coordinate.** A zonal mean
  (`time, lev, lat`), a y-z section, or a single column used to fail with
  `SDF file has no discernable X coordinate` and a pointer to writing a
  descriptor. The missing axis is now a single point, as `XDEF 1 LINEAR 0 1`
  would make it, and the open says so. X and Y are also found where the file
  marks them less formally: by a CF `standard_name` (`longitude`,
  `projection_x_coordinate`, and the Y equivalents), or by a dimension named
  `x`, `xc`, `lon`, `longitude`, `west_east` (`y`, `yc`, `lat`, `latitude`,
  `south_north` for Y). A dimension without a coordinate variable counts grid
  points, 1 to its size.
- **`sdfopen` and `xdfopen` read 365-day calendars.** A time coordinate with
  `calendar = "noleap"` (or `365_day`, `no_leap`) used to be refused. Its dates
  are now decoded with every year 365 days long and GrADS switches to its
  365-day calendar, so `days since 2000-01-01` values 58, 59, 60 read as 28 Feb,
  1 Mar, 2 Mar rather than landing on 29 Feb. Standard and Gregorian files work
  as before. This also lets `sdfopen` read back what `sdfwrite` writes from a
  365-day dataset. The 360-day, 366-day, and all-leap calendars, which GrADS
  cannot represent, are refused with a message naming the calendar instead of
  being misread as standard.
- **`bpopen` reads 1-D data.** A 1-D array whose length matches one axis is
  now a field: a reference profile such as `thbar(z)` opens as a Z profile
  that is the same at every X and Y, so `d th - thbar` works in any section.
  A global value written every step opens as a time series. A length that
  fits two axes is skipped with a warning rather than guessed, and a dataset
  that is a single column opens on its Z coordinate. Descriptors can say the
  same: a dimension list may leave out X or Y (`thbar=>thbar 300 z`), and
  `t` describes a per-step global value.
- **Fields written once hold for every time.** A BP5 variable written at one
  step, in a dataset whose other variables have more, such as terrain or a
  reference profile, used to read as undefined after the first time. It is
  now the same at every time, and the open names such variables.
- **Time averages of BP5 data run in parallel.** `ave`, `mean`, `sum`,
  `sumg`, `min`, `max`, `minloc`, and `maxloc` over time used to read a BP5
  variable one step at a time, and a vertical section one level at a time
  within each step. When the expression is a plain variable of the default
  file, the steps now go to ADIOS2 in batches its reader threads serve in
  parallel, and they are accumulated on the calculation threads in the same
  order as before; the regression checks the results are exactly those of
  the step-by-step path. Vertical sections and profiles are one read per step
  for every BP5 request. On a
  96 x 96 x 300, 241-step dataset an x-z section average went from 5.5 s to
  0.06 s and a 3-D `define` of the time mean from 13.6 s to 3.3 s. See
  [PERFORMANCE.md](PERFORMANCE.md#time-averages-of-bp5-data).

### Changed in 1.0.9

- **`bpopen` now behaves like opening a descriptor.** On a VVM-shaped dataset
  the two paths now print the same results for every command checked and draw
  byte-identical plots:
  - **Cartesian X and Y.** A model on a metre grid needs its X and Y written as
    degrees, because GrADS has only longitude and latitude; read as degrees,
    metres wrap the map labels round the globe and throw `aave` off by 70 %.
    `bpopen` now maps X and Y in a length unit onto GrADS's 6370 km sphere,
    centred on 0, as descriptors for Cartesian models do. A 35 m grid gives
    `xdef 96 linear -0.0149536 0.000314812`.
  - **Time.** T was always labelled from 00Z01JAN2000 in one-minute steps,
    whatever the data. It now comes from a CF time coordinate (`time`, units
    `<unit> since <date>`), and the open says when it has to fall back.
- **`sdfopen` maps Cartesian X and Y the same way.** An X or Y coordinate in
  metres or kilometres becomes degrees on the same sphere, centred on 0, so a
  VVM NetCDF file, its BP5 output through `bpopen`, and a hand-written
  descriptor all give the same grid. Before, such a file either failed to open
  or, when its axis carried `axis = "X"`, used the metres as degrees.
- **One calendar at a time, enforced for `sdfopen` too.** GrADS keeps a single
  calendar for all open files. Descriptors already enforced that, but
  `sdfopen` never set it, so a 365-day descriptor and a standard NetCDF file
  could be open together with one of them dated wrongly. Opening a file whose
  calendar differs from the open files' is now refused with a message saying
  which is which. Closing every file clears the calendar, so the next file may
  use either; before, only `reinit` did.

### Fixed in 1.0.9

- **Linux: GrADS crashed on start when the locale was not installed.** With
  `LC_ALL`, `LC_CTYPE` or `LANG` naming a locale the machine does not have --
  `LC_CTYPE=UTF-8`, which macOS Terminal sends over ssh, or a language pack
  that is not installed -- every start, batch or interactive, died with a
  segmentation fault right after `GX Package Initialization`. The bundled GNU
  Readline 8.2 did not check that the locale could be set; the upstream fix
  is its official patch 001. The Linux archive now builds Readline 8.2 with
  the full official patch series, 001 to 013, and the packager starts it under
  an uninstalled locale before it ships. With 1.0.8, start GrADS as
  `LC_ALL=C ./opengrads`, or set `LANG` to a locale `locale -a` lists.
- **1.0.8 let `LD_LIBRARY_PATH` replace the bundled libraries.** It embedded
  the bundle's library paths as RUNPATH, which the loader searches *after*
  `LD_LIBRARY_PATH`, so the libraries an environment module or conda had put
  there -- their own cairo, freetype, HDF5 and so on -- were loaded in place
  of the bundled ones and mixed with them. That is the likely cause of a
  segmentation fault reported on a RHEL 8 cluster right after `GX Package
  Initialization`. The paths are now embedded as RPATH, which the loader searches first,
  so the bundle wins whatever the shell carries; like RUNPATH it stays inside
  the binaries, so shell escapes still see the user's own `LD_LIBRARY_PATH`.
  The packager now refuses an archive that a decoy `LD_LIBRARY_PATH` can
  override. With 1.0.8, start GrADS as `env -u LD_LIBRARY_PATH ./opengrads`.

### Added in 1.0.8

- **Undo for the plot.** `set undo 10` turns undo on and keeps ten steps,
  `undo` steps the picture back one command, `undo <n>` steps back several,
  and `q undo` reports the state. It is off by default, so nothing changes
  for anyone who does not ask for it. A step is a command that changed the
  picture — settings and opens cost nothing — and a whole script counts as
  one step. Undo rewinds GrADS's graphics buffer and replays what is left,
  so an image exported after an undo is byte-for-byte the image the shorter
  command sequence exports, and on screen it redraws exactly as GrADS does
  when a window is exposed. It rewinds graphics only: settings, the dimension environment, open
  files, and anything written to disk are untouched, and `clear`, `reinit`,
  and double buffering drop the stored steps. See [docs/UNDO.md](UNDO.md).

### Changed in 1.0.8

- **macOS x86_64 archives are no longer published.** GitHub's `macos-15-intel`
  runner has bottles for few of the formulas this build needs, so Homebrew
  builds them from source and the job spent over ninety minutes installing
  prerequisites without reaching the compile step. Intel Macs can still build
  from source following [INSTALL.md](INSTALL.md); the arm64 archive will not
  run on them. Linux x86_64 and aarch64 and macOS arm64 are unaffected.

### Fixed in 1.0.8

- **`bpopen` now reports the same levels as a descriptor.** A coordinate array
  whose `units` attribute said meters was silently divided by 1000, so a
  descriptor-free open of a dataset with `z_mid` in meters showed `lev` in
  kilometers while the same dataset opened through a CTL showed it in meters:
  `zdef 2 levels 1000 500` became `zdef 2 levels 1 0.5`, and the same scaling
  hit X and Y. Coordinates are now used exactly as the dataset stores them, in
  the dataset's own units, so both paths agree.
- **`bpopen` no longer leaks the ADIOS2 variable list, hangs on crowded name
  stems, or writes descriptors GrADS cannot parse.** Three defects found while
  reviewing the backend: the name array `adios2_available_variables` allocates
  was never freed, so every `bpopen` leaked it along with one string per
  variable; the alias de-duplicator trimmed its own numeric suffix once it
  passed 999, repeating a candidate it had already rejected and looping
  forever; and a BP variable name holding whitespace, a `~`, an `=>`, or a
  leading character the descriptor parser reads as a comment produced a
  descriptor that failed to open, with an error naming a variable nobody
  wrote. Such fields are now skipped with a warning that names them.
- **The bundled libraries no longer leak into other programs.** The launcher
  exported `LD_LIBRARY_PATH`, which every subprocess GrADS spawns inherited,
  so a shell escape such as `!ls` ran the host's `ls` against the bundled
  libraries and warned `no version information available`. Each binary now
  carries its own `$ORIGIN`-relative RUNPATH instead, which cannot be
  inherited, and the launcher exports nothing.

### Fixed in 1.0.7

- **1.0.6 did not start on modern Linux.** It bundled glibc alongside the
  other libraries, so any host with a glibc newer than 2.28 loaded the bundled
  2.28 `libc.so.6` with its own loader and died with SIGILL before printing
  anything. The bundled glibc now lives in its own directory that is never
  placed on `LD_LIBRARY_PATH`; it is reached only through the bundled loader,
  where loader and libc match. Verified on glibc 2.39, 2.28, 2.27, and 2.24.

### Fixed in 1.0.6

- **The Linux archive now carries everything but the kernel.** glibc, the
  dynamic loader, and the UDUNITS unit database are bundled alongside the 65
  libraries already included. The launcher uses the host's glibc when it is
  new enough and the bundled one only when it is not, so archives now start on
  systems older than the build machine. Verified on glibc 2.28, 2.27, and
  2.24; the previous archive failed to start on the latter two.
- **`sdfopen` no longer needs udunits2 installed on the host.** The archive
  shipped `libudunits2.so` without its unit database, which UDUNITS-2 reads
  from a path compiled into the library. On a machine without udunits2 that
  path does not exist and `sdfopen` failed with `UDUNITS package
  initialization failure`. This affected 1.0.5 and earlier.

### Fixed in 1.0.5

- **Line editing no longer corrupts the display.** The coloured prompt handed
  readline 15 bytes for something 5 columns wide, so it counted the ANSI
  escapes as visible width and every cursor calculation on a wrapped line was
  off by ten columns, leaving stray spaces and fragments of the previous
  command. The escapes are now marked non-printing with readline's
  `RL_PROMPT_START_IGNORE`/`RL_PROMPT_END_IGNORE`.

### Fixed in 1.0.4

- **`dtype hdf5_grid` now reads data.** 1.0.3 could open an HDF5 file but
  failed on every variable with `invalid object ID` and `H5Dopen2 failed`.
  GrADS stored HDF5 object ids in a 32-bit `gaint`, but HDF5 widened `hid_t`
  to 64 bits in version 1.10, so every id was truncated. The ids are now held
  in a 64-bit type. Verified against hdf5-1.10.5.

### Fixed in 1.0.3

- **Linux archives now run on RHEL 8 era systems, including HPC clusters.**
  Earlier releases were built on Ubuntu 22.04 (glibc 2.35) and failed to start
  on anything older with `version 'GLIBC_2.xx' not found`. Linux archives are
  now built in AlmaLinux 8 (glibc 2.28), so they require glibc 2.28 or newer.
- **Restored `-ldl` and `-lpthread` at link time.** A missing automake
  substitution meant `host_runtime_libs` expanded to nothing, so these were
  silently dropped. It went unnoticed because glibc 2.34 merged both into
  libc; on older systems the link fails outright.
- **`sdfopen` on RHEL-family builds.** UDUNITS headers live in
  `/usr/include/udunits2/` there rather than `/usr/include`, so the probe
  missed them.
- **HDF5 was disabled in every earlier Linux archive.** Debian hides its
  headers under `/usr/include/hdf5/serial/`, which the configure probe does
  not search, so `USEHDF5` was 0 and the descriptor
  keyword was rejected with `Data file type invalid`. The AlmaLinux build
  finds HDF5 in the standard location, and source builds on Debian now add
  the multiarch path explicitly.

### Fixed in 1.0.2

- **Prompt colour and Tab completion.** 1.0.0 and 1.0.1 linked BSD libedit,
  which mangles the ANSI escape sequences in the coloured prompt (printing
  `[32mga-> [39m` as literal text) and appends a space after ambiguous
  completions. GNU Readline is restored, so the green prompt and Tab
  behaviour match the earlier `bp5.*` releases again.

### Fixed in 1.0.1

- **Terminal handling at the `ga->` prompt.** The bundled ncurses carried the
  build machine's terminfo path compiled into it. That directory does not
  exist on your machine, so terminal lookup failed and the prompt fell back to
  dumb terminal settings, printing `No entry for terminal type ...`. The
  launcher now points `TERMINFO_DIRS` at the usual system locations. An
  explicit `TERMINFO` or `TERMINFO_DIRS` you set yourself still wins.

1.0.0 was the first release under the `opengrads-hpc` name and its own version
line. Earlier `v2.2.1.oga.1-bp5.*` tags remain available and are unaffected.

**Based on GrADS 2.2.1 / OpenGrADS 2.2.1.oga.1.** Descriptor files, `.gs`
scripts, and command syntax are unchanged, and `grads` still reports the GrADS
baseline it was built from. Each archive carries a `VERSION` file naming both
the distribution version and that baseline.

"HPC" here means the ADIOS2/BP5 engine plus OpenMP threading on a single node.
There is no MPI or GPU support, and none is planned — ADIOS2 is deliberately
built with `ADIOS2_USE_MPI=OFF`.

### What's in it

- **ADIOS2 BP5 input.** `bpopen` opens a BP5 dataset without a descriptor and
  behaves as a descriptor for it would; a descriptor with `dtype bp5` gives
  exact control. Handles partial `TDEF` from a run that did not finish, 1-D
  profiles and fields written once, dataset attributes, and descriptor
  precedence. Time averages read many steps at once, in parallel.
- **OpenMP-threaded calculations.** Defaults to 4 threads; `-j N` or
  `GA_NUM_THREADS` override it, and `q threads` reports the active count.
- **`sdfopen` / `xdfopen`** against NetCDF-4 and HDF5.
- **Plots in the terminal** over plain ssh, from iTerm2 or WezTerm, in a
  tmux pane beside the prompt; no X server needed (Linux and macOS).
- **Three native archives**, each self-contained: Linux x86_64 and aarch64,
  and macOS arm64. No dependency installation and no library paths to set.

### Graphics drivers per platform

Plug-ins are loaded with `dlopen()` and call back into the `grads` executable,
which constrains what each platform can carry:

| Platform | Display (`-d`) | Hardcopy (`-h`) |
| --- | --- | --- |
| Linux | `Cairo`, `X11`, `Term`, `gxdummy` | `Cairo`, `gxdummy` |
| macOS | `Cairo`, `X11` (with XQuartz), `Term`, `gxdummy` | `Cairo`, `gxdummy` |

On macOS the `Cairo` and `X11` displays draw in a window through XQuartz,
and `Term` draws in iTerm2 or WezTerm. Without either the archive runs
headless but keeps the full Cairo hardcopy path, so `printim` and `print`
produce PNG, PS, PDF, and SVG.

A native Windows build is not published yet; see `docs/RELEASES.md` for its
status.

### Verifying and running

```bash
sha256sum -c opengrads-hpc-1.0.10-linux-x86_64.tar.gz.sha256
tar -xzf opengrads-hpc-1.0.10-linux-x86_64.tar.gz
cd opengrads-hpc-1.0.10-linux-x86_64
./opengrads
```

On macOS start `./opengrads`. The launcher opens a GrADS window when
XQuartz is installed (it sets `DISPLAY`), draws in the terminal in iTerm2 or
WezTerm otherwise, and runs headless elsewhere, so no extra flags are needed.
`./opengrads -l -d Term` draws in the terminal even with XQuartz.

### Known limitations

- No Windows build in this release.
- Linux archives are built on AlmaLinux 8 (glibc 2.28). They use the host's
  glibc when it is 2.28 or newer and their bundled glibc and loader otherwise.
- Reading BP5 written by a multi-rank MPI job is supported by ADIOS2's format
  but is not yet covered by the regression suite.
- The terminal display is tested through tmux 3.2a, 3.4, and 3.7c, plain and
  `-CC`, with a terminal emulator standing in for iTerm2, not yet on iTerm2
  itself. If no picture appears, `GA_TERM_LOG=/tmp/grads-term.log` records
  what happened; `GA_TERM_PROGRESS=off` sends each picture in the oldest form
  of the protocol. Pictures in parts, which tmux before 3.3 and `tmux -CC`
  always use, need iTerm2 3.5 or newer.
