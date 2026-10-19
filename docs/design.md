# How tinymenu works

## One frame

The application owns the loop. Each turn it may queue input with
`tm_ui_send()` and then calls `tm_ui_proc(ui, elapsed_ms)`:

1. If the UI is idle (nothing animating, nothing scrolling) and no message is
   queued, `tm_ui_proc` returns at once: no drawing, no flush.
2. Otherwise it clears the framebuffer and runs the state machine for one
   step, which reacts to at most one message and draws the current page.
3. The shared indicator (the box that follows the selection) and scroll bar
   are animated and drawn on top.
4. Pages whose content changed since the last flush are handed to the
   display callback as one range `[first_page, end_page)`.

Everything is redrawn every frame. That keeps pages simple (no invalidation
logic in drawing code) and costs 1–2 ms on an ESP32; the expensive part is the
transfer to the panel, and the dirty-page check below keeps that small.

## State machine

```
        jump, or react() asks to leave            fade-out done              in() and fade-in done
  PROC ─────────────────────────────────► OUT ─────────────────► IN ─────────────────────────► PROC
```

- **PROC**: the current page reacts to a message and draws itself.
- **OUT**: a full page fades out (the framebuffer is dithered progressively).
- **IN**: the next page runs its entry animation while the fade reverses.

A popup is a page whose type sorts after `TM_PAGE_POPUPS`. Jumping to one
skips OUT: the page underneath keeps being drawn, dimmed to the popup's blur
level, and the popup animates in on top. Leaving a popup runs its entry
animation backwards over the page underneath. Since popups are pages, a popup
can open another popup; `last_page` links form the way back.

Every page type provides the same methods (`tm_page_methods`): `in_para_init`
and `in` for the entry animation, `show` to draw, `react` to handle a message,
and `indicator_ctrl` / `scrollbar_ctrl` to aim the shared indicator and
scroll bar. Pages only write their own state during `show`/`react`; the shared
elements are set in the two `_ctrl` methods, so the UI can choose which page
drives them during a transition.

## Input

Input is a small ring buffer of `tm_msg` (`TM_MSG_QUEUE_SIZE`, 4 by default).
A page reads one message per frame in `react`. With `auto_input` on
(the default) the page handles navigation itself and then calls its callback;
with it off the callback receives the message and decides, returning true to
leave the page.

## Animation

`tm_anim` moves `pos_cur` toward `pos_tgt` by `distance * elapsed / duration`
per frame and carries the remainder, so an animation takes `duration` ms of
wall time whatever the frame rate, and never stalls one pixel short. Durations
live in `ui->params.ani_param`. Scrolling text advances one step per 10 ms of
elapsed time for the same reason. A step longer than one second completes the
animation at once (after a stall, jumping is better than a long catch-up).

`ui->params.style.animate = false` makes every duration 0 (`tm_ui_anim_ms`),
so each animation completes in the frame it starts, and turns the fade between
full pages off: the next page is drawn in the frame the old one leaves.
Popups still dim the page underneath, at once.

## Highlight

The highlight is the shared indicator drawn last, in XOR (`tm_ui_draw_highlight`):
an inverted filled box gives the same pixels as drawing a row's background and
then its text in the opposite colour, so the classic full-row look is the
indicator at full width with animation off rather than a second drawing path.
`TM_HIGHLIGHT_FRAME` draws an outline instead. Corner radii pass through
`tm_ui_radius`, which caps them at `style.radius`.

## Framebuffer and flushing

`tm_fb` is caller-owned memory in page-major layout: pixel (x, y) is bit
`y % 8` of `buf[(y / 8) * w + x]`. `tm_fb_size(w, h)` gives the byte count.

To find what changed without keeping a copy of the last frame, the UI keeps a
32-bit FNV-1a hash per page (`TM_MAX_PAGES`, 32 by default, covers 256 rows;
taller screens always send the rest). After drawing it hashes each page,
compares with the stored hash, and flushes the span from the first to the last
changed page. A false match needs a 32-bit collision on a page that actually
changed; the next change to that page repairs it. `tm_ui_invalidate()` forces
the next frame to be drawn and sent in full, for example after the panel lost
power.

## Layout

Each page and popup computes positions from `ui->fb.w` / `ui->fb.h` and the
compile-time font and spacing defaults in `tm_config.h`. Popups are centred
and never wider than the screen minus 4 px on each side; their inner columns
give up the width they lose. The layout macros live in each module's `.c`
file; the ones that depend on the screen take the UI as an argument.

## Memory

Nothing is allocated. The application provides:

| what | size |
|---|---|
| framebuffer | `w * ceil(h / 8)` bytes (1 KiB at 128x64) |
| `tm_ui` | a few hundred bytes, 128 of them the per-page hashes |
| each page/popup object | 100–250 bytes |
| option arrays, icons, strings | as declared; icons and strings can be `const` |
| wave history | `depth` samples per wave, caller provided |

## Testing

`test/render.c` drives the demo with a scripted session and writes PBM
snapshots at several screen sizes; `make -C test` compares them byte for byte
with `test/snapshots/`. Its flush callback updates a simulated display with
only the pages it is given, and each snapshot is read from that display after
checking it equals the framebuffer, so the dirty-page logic is tested on every
screen. `test/unit.c` covers helpers the session does not reach.
