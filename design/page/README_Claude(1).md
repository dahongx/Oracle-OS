# Oracle OS LCD reference package

This package is designed to be pasted directly into Claude or another coding model as a visual implementation contract.

## Files

- `oracle_ui_reference_tft_espi.ino`
  - A complete 240×240 TFT_eSPI reference renderer.
  - Includes Boot, Home, Mode Select, five ritual modes, Reveal, History, Settings, and Long-Press Menu.
  - Includes wheel-speed feedback, short-press lock feedback, and long-press progress.
- `oracle_ui_spec.json`
  - Exact palette, layout coordinates, typography, animation, and interaction constraints.

## The main problem visible in the prototype photo

The current LCD result is visually far from the design because it uses:
- a black background instead of the warm off-white field;
- bright green, blue, and cyan instead of muted champagne/sage;
- a large filled character illustration instead of restrained 1-bit line art;
- thick borders and a single “game screen” composition instead of a precise instrument layout;
- little hierarchy between status, ritual focus, and bottom action;
- no visual linkage between the physical wheel and orbit motion.

## Prompt to give Claude

Use this exact instruction together with the `.ino` and `.json` files:

> Implement the attached Oracle OS UI specification exactly. Do not redesign it. The target is a 240×240 square LCD with an encoder wheel and push button. Use the supplied coordinates, palette, and state machine. The interface must remain warm-white, sparse, line-based, and pixel-sharp. Do not use saturated green backgrounds, neon blue outlines, large filled cartoon sprites, random particles, or mobile-app card layouts. Preserve the right-edge wheel cue, top status bar, central orbit instrument, and bottom action bar. Map encoder rotation to selection/tuning/energy, short press to confirm/capture/lock, and 1.2-second long press to back/system menu. Keep redraw at 10 fps or less. First make the Home screen match the reference; then implement the remaining screens using the same primitives.

## Hardware adaptation

The sample uses TFT_eSPI. Replace:
- `encoderReadDelta()`
- `buttonPressed()`

with the actual encoder/button driver.

If the project uses Adafruit_GFX, LovyanGFX, LVGL, Arduino_GFX, or ESP-IDF, keep the state machine and coordinates unchanged and only remap the drawing calls.

## RAM note

The sample creates a full 240×240 RGB565 sprite (~115 KB). On memory-limited hardware:
1. draw the static frame once;
2. allocate only a 160×160 sprite for the center ritual region;
3. redraw status text and bottom prompts only when values change.
