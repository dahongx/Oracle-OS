/*
  Oracle OS — 240x240 LCD reference implementation
  Target: ESP32 / RP2040 + ST7789-like 240x240 LCD
  Graphics API: TFT_eSPI

  Purpose:
  - This is a visual and interaction reference, not a finished hardware driver.
  - Keep the UI geometric, sparse, warm-white, and low-motion.
  - Replace encoderReadDelta()/buttonPressed() with your own hardware code.

  Core input mapping:
  Rotate      -> change selection / tune orbit / accumulate energy
  Short press -> confirm / capture / lock
  Long press  -> back / cancel / system menu
*/

#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite ui = TFT_eSprite(&tft);

static const int LCD_W = 240;
static const int LCD_H = 240;

// ---------- Theme ----------
struct Theme {
  uint16_t bg;
  uint16_t panel;
  uint16_t ink;
  uint16_t muted;
  uint16_t line;
  uint16_t gold;
  uint16_t sage;
  uint16_t clover;
  uint16_t black;
} C;

uint16_t rgb565(uint32_t hex) {
  uint8_t r = (hex >> 16) & 0xFF;
  uint8_t g = (hex >> 8) & 0xFF;
  uint8_t b = hex & 0xFF;
  return ui.color565(r, g, b);
}

void initTheme() {
  C.bg     = rgb565(0xF7F6F1);
  C.panel  = rgb565(0xEEEDE8);
  C.ink    = rgb565(0x625B50);
  C.muted  = rgb565(0x9A9388);
  C.line   = rgb565(0xD9D5CC);
  C.gold   = rgb565(0xC5B48E);
  C.sage   = rgb565(0xAEB3A1);
  C.clover = rgb565(0x7D8276);
  C.black  = rgb565(0x171717);
}

// ---------- State ----------
enum ScreenId {
  SCR_BOOT,
  SCR_HOME,
  SCR_MODE_SELECT,
  SCR_QUICK_ASK,
  SCR_ORBIT,
  SCR_TRIAD,
  SCR_MOOD,
  SCR_DAILY,
  SCR_REVEAL,
  SCR_HISTORY,
  SCR_SETTINGS,
  SCR_SYSTEM_MENU
};

struct UiState {
  ScreenId screen = SCR_BOOT;
  int modeIndex = 0;
  int optionIndex = 0;
  int wheelSpeed = 0;       // 0..3
  int energy = 0;           // 0..100
  int holdProgress = 0;     // 0..100
  bool pressed = false;
  bool locked = false;
  uint32_t bootAt = 0;
  uint32_t lastFrame = 0;
  uint32_t lastRotateAt = 0;
} S;

// ---------- Hardware adapter stubs ----------
// Replace these three functions with your encoder + button implementation.
int encoderReadDelta() { return 0; }       // return negative / positive detents
bool buttonPressed()   { return false; }   // current level
uint32_t millisNow()   { return millis(); }

// ---------- Layout ----------
namespace L {
  const int margin = 8;
  const int topBarH = 24;
  const int bottomBarH = 28;
  const int contentTop = 30;
  const int contentBottom = 205;
  const int centerX = 120;
  const int centerY = 116;
}

// ---------- Drawing primitives ----------
void clearUi() {
  ui.fillSprite(C.bg);
}

void pixel(int x, int y, uint16_t color, int s = 2) {
  ui.fillRect(x, y, s, s, color);
}

void cornerMarks(int x, int y, int w, int h, uint16_t color) {
  const int n = 6;
  ui.drawLine(x, y, x+n, y, color);
  ui.drawLine(x, y, x, y+n, color);
  ui.drawLine(x+w-1-n, y, x+w-1, y, color);
  ui.drawLine(x+w-1, y, x+w-1, y+n, color);
  ui.drawLine(x, y+h-1-n, x, y+h-1, color);
  ui.drawLine(x, y+h-1, x+n, y+h-1, color);
  ui.drawLine(x+w-1-n, y+h-1, x+w-1, y+h-1, color);
  ui.drawLine(x+w-1, y+h-1-n, x+w-1, y+h-1, color);
}

void drawTinyStar(int x, int y, uint16_t color) {
  pixel(x, y, color, 2);
  pixel(x-2, y, color, 1);
  pixel(x+3, y, color, 1);
  pixel(x+1, y-2, color, 1);
  pixel(x+1, y+3, color, 1);
}

void drawClover(int cx, int cy, int r, uint16_t color, bool filled = false) {
  int d = r * 2;
  if (filled) {
    ui.fillCircle(cx-r, cy-r, r, color);
    ui.fillCircle(cx+r, cy-r, r, color);
    ui.fillCircle(cx-r, cy+r, r, color);
    ui.fillCircle(cx+r, cy+r, r, color);
  } else {
    ui.drawCircle(cx-r, cy-r, r, color);
    ui.drawCircle(cx+r, cy-r, r, color);
    ui.drawCircle(cx-r, cy+r, r, color);
    ui.drawCircle(cx+r, cy+r, r, color);
  }
  ui.drawLine(cx, cy+r*2, cx+1, cy+r*3+2, color);
}

void drawCat(int cx, int cy, int scale, uint16_t color, bool fillFace = false) {
  // Pixel-style cat face built only from rectangles/lines.
  int s = scale;
  int x = cx - 9*s;
  int y = cy - 7*s;

  // ears
  ui.drawLine(x+1*s, y+4*s, x+2*s, y, color);
  ui.drawLine(x+2*s, y, x+6*s, y+3*s, color);
  ui.drawLine(x+12*s, y+3*s, x+16*s, y, color);
  ui.drawLine(x+16*s, y, x+17*s, y+4*s, color);

  // face
  if (fillFace) ui.fillRoundRect(x+1*s, y+3*s, 16*s, 12*s, 3*s, C.panel);
  ui.drawRoundRect(x+1*s, y+3*s, 16*s, 12*s, 3*s, color);

  // eyes, nose
  ui.fillRect(x+5*s, y+8*s, s, s, color);
  ui.fillRect(x+12*s, y+8*s, s, s, color);
  ui.fillRect(x+8*s, y+10*s, 2*s, s, color);

  // mouth
  ui.drawLine(x+9*s, y+11*s, x+8*s, y+12*s, color);
  ui.drawLine(x+9*s, y+11*s, x+10*s, y+12*s, color);
}

void drawOrbit(int cx, int cy, int baseR, int speed, int energy, bool focusCat) {
  // Static orbital geometry: 3 circles, 4 axis markers, sparse stars.
  ui.drawCircle(cx, cy, baseR, C.gold);
  ui.drawCircle(cx, cy, baseR - 14, C.line);
  ui.drawCircle(cx, cy, baseR - 30, C.gold);

  ui.drawLine(cx, cy-baseR-6, cx, cy-baseR+5, C.ink);
  ui.drawLine(cx, cy+baseR-5, cx, cy+baseR+6, C.ink);
  ui.drawLine(cx-baseR-6, cy, cx-baseR+5, cy, C.ink);
  ui.drawLine(cx+baseR-5, cy, cx+baseR+6, cy, C.ink);

  // Deterministic moving points; no random flicker.
  uint32_t t = millisNow() / (120 - speed * 22);
  for (int i = 0; i < 4; ++i) {
    float a = (t * 0.08f) + i * 1.5708f;
    int r = baseR - (i % 3) * 14;
    int px = cx + (int)(cos(a) * r);
    int py = cy + (int)(sin(a) * r);
    ui.fillCircle(px, py, (i == speed ? 2 : 1), C.clover);
  }

  if (focusCat) drawCat(cx, cy, 2, C.ink, true);
  else drawClover(cx, cy, 5, C.clover, true);

  // Energy arc approximation.
  int segs = map(energy, 0, 100, 0, 24);
  for (int i = 0; i < segs; ++i) {
    float a = -1.5708f + i * (6.28318f / 24.0f);
    int x1 = cx + cos(a) * (baseR + 8);
    int y1 = cy + sin(a) * (baseR + 8);
    int x2 = cx + cos(a) * (baseR + 11);
    int y2 = cy + sin(a) * (baseR + 11);
    ui.drawLine(x1, y1, x2, y2, C.clover);
  }
}

void drawTopBar(const char* title) {
  ui.setTextDatum(TL_DATUM);
  ui.setTextColor(C.ink, C.bg);
  ui.setTextSize(1);
  ui.drawString(title, 12, 10, 1);

  ui.setTextDatum(TR_DATUM);
  ui.drawString("12:00", 206, 10, 1);

  ui.drawRect(212, 11, 14, 7, C.ink);
  ui.fillRect(226, 13, 2, 3, C.ink);
  ui.fillRect(214, 13, 8, 3, C.clover);

  ui.drawLine(10, 26, 230, 26, C.line);
}

void drawBottomBar(const char* centerLabel, bool showWheel = true) {
  ui.drawLine(10, 207, 230, 207, C.line);

  drawClover(21, 222, 3, C.clover, true);
  drawClover(219, 222, 3, C.clover, false);

  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.ink, C.bg);
  ui.drawString(centerLabel, 120, 222, 1);

  if (showWheel) {
    // right-edge wheel cue
    ui.fillRoundRect(230, 98, 8, 38, 4, C.panel);
    for (int y = 103; y < 132; y += 4) ui.drawLine(232, y, 236, y, C.gold);
  }
}

void drawHoldRing(int cx, int cy, int progress) {
  const int r = 18;
  ui.drawCircle(cx, cy, r, C.line);
  int segs = map(progress, 0, 100, 0, 20);
  for (int i = 0; i < segs; i++) {
    float a = -1.5708f + i * (6.28318f / 20.0f);
    int x1 = cx + cos(a) * (r-1);
    int y1 = cy + sin(a) * (r-1);
    int x2 = cx + cos(a) * (r+2);
    int y2 = cy + sin(a) * (r+2);
    ui.drawLine(x1, y1, x2, y2, C.clover);
  }
}

// ---------- Screens ----------
void drawBoot() {
  clearUi();
  cornerMarks(8, 8, 224, 224, C.line);

  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.ink, C.bg);
  ui.setTextSize(2);
  ui.drawString("ORACLE", 120, 88, 1);
  ui.drawString("OS", 120, 108, 1);
  ui.setTextSize(1);

  drawClover(120, 142, 5, C.clover, true);

  int phase = ((millisNow() - S.bootAt) / 160) % 8;
  for (int i = 0; i < 8; i++) {
    uint16_t c = (i <= phase) ? C.gold : C.line;
    ui.fillRect(89 + i*8, 177, 4, 2, c);
  }
}

void drawHome() {
  clearUi();
  drawTopBar("ORACLE OS");
  cornerMarks(10, 32, 220, 170, C.line);

  drawTinyStar(34, 51, C.gold);
  drawTinyStar(205, 60, C.gold);
  drawTinyStar(43, 179, C.muted);
  drawTinyStar(199, 173, C.muted);

  drawOrbit(L::centerX, 116, 64, S.wheelSpeed, S.energy, true);
  drawBottomBar("FOCUS");
}

const char* modeNames[] = {
  "QUICK ASK", "ORBIT", "TRIAD",
  "MOOD", "DAILY", "HISTORY",
  "SETTINGS"
};

void drawModeIcon(int cx, int cy, int index, bool selected) {
  uint16_t edge = selected ? C.ink : C.line;
  uint16_t icon = selected ? C.clover : C.muted;

  if (selected) ui.fillRoundRect(cx-25, cy-23, 50, 46, 6, C.panel);
  ui.drawRoundRect(cx-25, cy-23, 50, 46, 6, edge);

  switch (index) {
    case 0:
      ui.drawCircle(cx, cy-2, 10, icon);
      ui.drawString("?", cx-3, cy-8, 1);
      break;
    case 1:
      ui.drawCircle(cx, cy-2, 12, icon);
      ui.drawCircle(cx+8, cy-2, 2, icon);
      break;
    case 2:
      ui.drawTriangle(cx, cy-13, cx-13, cy+9, cx+13, cy+9, icon);
      break;
    case 3:
      drawCat(cx, cy-2, 1, icon, false);
      break;
    case 4:
      ui.drawLine(cx-11, cy+7, cx+11, cy+7, icon);
      ui.drawCircle(cx, cy-2, 8, icon);
      break;
    case 5:
      ui.drawRect(cx-10, cy-11, 20, 20, icon);
      ui.drawLine(cx-6, cy-5, cx+6, cy-5, icon);
      ui.drawLine(cx-6, cy, cx+6, cy, icon);
      break;
    case 6:
      ui.drawCircle(cx, cy-2, 9, icon);
      for (int a = 0; a < 8; a++) {
        float rad = a * 0.7854f;
        int x1 = cx + cos(rad) * 11;
        int y1 = cy-2 + sin(rad) * 11;
        int x2 = cx + cos(rad) * 15;
        int y2 = cy-2 + sin(rad) * 15;
        ui.drawLine(x1, y1, x2, y2, icon);
      }
      break;
  }
}

void drawModeSelect() {
  clearUi();
  drawTopBar("SELECT MODE");

  const int cols = 3;
  const int x0 = 49, y0 = 63, dx = 71, dy = 64;
  for (int i = 0; i < 6; ++i) {
    int x = x0 + (i % cols) * dx;
    int y = y0 + (i / cols) * dy;
    drawModeIcon(x, y, i, i == S.modeIndex);
  }

  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.ink, C.bg);
  ui.drawString(modeNames[S.modeIndex], 120, 187, 1);
  drawBottomBar("PRESS TO ENTER");
}

void drawQuickAsk() {
  clearUi();
  drawTopBar("QUICK ASK");
  cornerMarks(12, 34, 216, 166, C.line);

  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.muted, C.bg);
  ui.drawString("HOLD A QUESTION", 120, 54, 1);

  ui.setTextColor(C.ink, C.bg);
  ui.setTextSize(4);
  ui.drawString("?", 120, 109, 1);
  ui.setTextSize(1);

  drawOrbit(120, 116, 57, S.wheelSpeed, S.energy, false);

  ui.setTextDatum(MC_DATUM);
  ui.drawString("ROTATE TO GATHER", 120, 185, 1);
  drawBottomBar("PRESS TO REVEAL");
}

void drawOrbitCapture() {
  clearUi();
  drawTopBar("ORBIT CAPTURE");
  cornerMarks(12, 34, 216, 166, C.line);

  drawOrbit(120, 113, 69, S.wheelSpeed, S.energy, false);

  // target point
  ui.drawCircle(166, 79, 7, C.ink);
  ui.fillCircle(166, 79, 2, C.clover);

  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.ink, C.bg);
  ui.drawString("CAPTURE  1 / 3", 120, 186, 1);
  drawBottomBar("PRESS ON ALIGN");
}

void drawTriad() {
  clearUi();
  drawTopBar("TRIAD");
  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.muted, C.bg);
  ui.drawString("FORM THREE SIGILS", 120, 48, 1);

  for (int i = 0; i < 3; ++i) {
    int cx = 53 + i * 67;
    ui.drawCircle(cx, 112, 27, C.line);
    ui.drawCircle(cx, 112, 19, C.gold);
    if (i < S.optionIndex) drawClover(cx, 112, 5, C.clover, true);
    else drawTinyStar(cx, 112, C.ink);
  }

  ui.setTextColor(C.ink, C.bg);
  ui.drawString("ROTATE / PRESS / REPEAT", 120, 177, 1);
  drawBottomBar("3 SIGILS");
}

void drawMood() {
  clearUi();
  drawTopBar("MOOD COMPASS");
  drawOrbit(120, 113, 66, 1, 65, true);

  // four direction labels
  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.muted, C.bg);
  ui.drawString("CALM", 120, 42, 1);
  ui.drawString("OPEN", 120, 188, 1);
  ui.drawString("QUIET", 35, 115, 1);
  ui.drawString("BRIGHT", 205, 115, 1);

  // pointer based on option index
  float a = S.optionIndex * 0.7854f;
  int x = 120 + cos(a) * 50;
  int y = 113 + sin(a) * 50;
  ui.fillCircle(x, y, 3, C.clover);

  drawBottomBar("PRESS TO LOCK");
}

void drawDaily() {
  clearUi();
  drawTopBar("DAILY GUIDE");

  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.muted, C.bg);
  ui.drawString("TODAY", 120, 51, 1);

  drawClover(120, 100, 12, C.clover, false);
  ui.drawCircle(120, 100, 35, C.gold);
  drawTinyStar(120, 62, C.gold);
  drawTinyStar(81, 100, C.gold);
  drawTinyStar(159, 100, C.gold);

  ui.setTextColor(C.ink, C.bg);
  ui.setTextSize(2);
  ui.drawString("STEADY", 120, 154, 1);
  ui.setTextSize(1);
  ui.drawString("MOVE WITHOUT HURRY", 120, 177, 1);

  drawBottomBar("PRESS TO SAVE");
}

void drawReveal() {
  clearUi();
  drawTopBar("REVEAL");
  cornerMarks(12, 34, 216, 166, C.gold);

  drawOrbit(120, 104, 48, 0, 100, false);

  ui.setTextDatum(MC_DATUM);
  ui.setTextColor(C.ink, C.bg);
  ui.setTextSize(2);
  ui.drawString("TRUST", 120, 163, 1);
  ui.setTextSize(1);
  ui.drawString("THE FLOW", 120, 182, 1);

  drawBottomBar("PRESS TO SAVE");
}

void drawHistory() {
  clearUi();
  drawTopBar("HISTORY");

  const char* dates[] = {"05/28", "05/27", "05/24", "05/22"};
  const char* labels[] = {"STEADY", "ORBIT", "THREE SIGILS", "QUIET"};
  for (int i = 0; i < 4; ++i) {
    int y = 52 + i * 37;
    if (i == S.optionIndex) ui.fillRoundRect(14, y-8, 212, 29, 5, C.panel);
    drawClover(28, y+6, 3, C.clover, i == S.optionIndex);
    ui.setTextDatum(TL_DATUM);
    ui.setTextColor(C.ink, i == S.optionIndex ? C.panel : C.bg);
    ui.drawString(dates[i], 43, y, 1);
    ui.setTextColor(C.muted, i == S.optionIndex ? C.panel : C.bg);
    ui.drawString(labels[i], 92, y, 1);
  }
  drawBottomBar("PRESS TO OPEN");
}

void drawSettings() {
  clearUi();
  drawTopBar("SETTINGS");

  const char* names[] = {"BRIGHTNESS", "MOTION", "SOUND", "LANGUAGE", "SLEEP"};
  const char* vals[]  = {"6", "LOW", "ON", "CN", "2 MIN"};

  for (int i = 0; i < 5; ++i) {
    int y = 50 + i * 31;
    if (i == S.optionIndex) ui.fillRoundRect(14, y-6, 212, 25, 5, C.panel);
    ui.setTextDatum(TL_DATUM);
    ui.setTextColor(C.ink, i == S.optionIndex ? C.panel : C.bg);
    ui.drawString(names[i], 24, y, 1);
    ui.setTextDatum(TR_DATUM);
    ui.drawString(vals[i], 216, y, 1);
    ui.drawLine(20, y+22, 220, y+22, C.line);
  }
  drawBottomBar("ROTATE TO ADJUST");
}

void drawSystemMenu() {
  clearUi();
  drawTopBar("SYSTEM");
  const char* items[] = {"RETURN", "HOME", "SETTINGS", "RESET"};
  for (int i = 0; i < 4; ++i) {
    int y = 62 + i * 33;
    bool sel = i == S.optionIndex;
    if (sel) ui.fillRoundRect(42, y-8, 156, 26, 4, C.panel);
    ui.setTextDatum(MC_DATUM);
    ui.setTextColor(C.ink, sel ? C.panel : C.bg);
    ui.drawString(items[i], 120, y+4, 1);
  }
  if (S.pressed) drawHoldRing(120, 192, S.holdProgress);
  drawBottomBar("LONG PRESS TO EXIT", false);
}

// ---------- Input semantics ----------
void onRotate(int delta) {
  if (delta == 0) return;

  uint32_t now = millisNow();
  uint32_t dt = now - S.lastRotateAt;
  S.lastRotateAt = now;

  if (dt < 45) S.wheelSpeed = 3;
  else if (dt < 100) S.wheelSpeed = 2;
  else S.wheelSpeed = 1;

  switch (S.screen) {
    case SCR_HOME:
      S.energy = constrain(S.energy + abs(delta) * 3, 0, 100);
      break;

    case SCR_MODE_SELECT:
      S.modeIndex = (S.modeIndex + delta + 6) % 6;
      break;

    case SCR_QUICK_ASK:
    case SCR_ORBIT:
      S.energy = constrain(S.energy + abs(delta) * 4, 0, 100);
      break;

    case SCR_TRIAD:
      S.optionIndex = constrain(S.optionIndex + (delta > 0 ? 1 : -1), 0, 3);
      break;

    case SCR_MOOD:
      S.optionIndex = (S.optionIndex + delta + 8) % 8;
      break;

    case SCR_HISTORY:
      S.optionIndex = (S.optionIndex + delta + 4) % 4;
      break;

    case SCR_SETTINGS:
      S.optionIndex = (S.optionIndex + delta + 5) % 5;
      break;

    case SCR_SYSTEM_MENU:
      S.optionIndex = (S.optionIndex + delta + 4) % 4;
      break;

    default:
      break;
  }
}

void onShortPress() {
  switch (S.screen) {
    case SCR_HOME:
      S.screen = SCR_MODE_SELECT;
      break;

    case SCR_MODE_SELECT:
      if      (S.modeIndex == 0) S.screen = SCR_QUICK_ASK;
      else if (S.modeIndex == 1) S.screen = SCR_ORBIT;
      else if (S.modeIndex == 2) S.screen = SCR_TRIAD;
      else if (S.modeIndex == 3) S.screen = SCR_MOOD;
      else if (S.modeIndex == 4) S.screen = SCR_DAILY;
      else if (S.modeIndex == 5) S.screen = SCR_HISTORY;
      S.energy = 0;
      S.optionIndex = 0;
      break;

    case SCR_QUICK_ASK:
    case SCR_ORBIT:
    case SCR_TRIAD:
    case SCR_MOOD:
      S.screen = SCR_REVEAL;
      break;

    case SCR_DAILY:
    case SCR_REVEAL:
      S.locked = true; // play one short lock flash; store result here
      break;

    case SCR_HISTORY:
      S.screen = SCR_REVEAL;
      break;

    case SCR_SETTINGS:
      // modify selected setting here
      break;

    case SCR_SYSTEM_MENU:
      if (S.optionIndex == 0) S.screen = SCR_HOME;
      if (S.optionIndex == 1) S.screen = SCR_HOME;
      if (S.optionIndex == 2) S.screen = SCR_SETTINGS;
      if (S.optionIndex == 3) {
        S.energy = 0;
        S.modeIndex = 0;
        S.screen = SCR_HOME;
      }
      break;

    default:
      break;
  }
}

void onLongPress() {
  if (S.screen == SCR_HOME) S.screen = SCR_SYSTEM_MENU;
  else if (S.screen == SCR_SYSTEM_MENU) S.screen = SCR_HOME;
  else S.screen = SCR_HOME;
}

// ---------- Button recognizer ----------
void pollInput() {
  static bool prev = false;
  static uint32_t downAt = 0;
  static bool longSent = false;

  int delta = encoderReadDelta();
  if (delta != 0) onRotate(delta);

  bool nowPressed = buttonPressed();
  uint32_t now = millisNow();

  if (nowPressed && !prev) {
    downAt = now;
    longSent = false;
    S.pressed = true;
    S.holdProgress = 0;
  }

  if (nowPressed) {
    uint32_t held = now - downAt;
    S.holdProgress = constrain((int)(held * 100 / 1200), 0, 100);
    if (held >= 1200 && !longSent) {
      longSent = true;
      onLongPress();
    }
  }

  if (!nowPressed && prev) {
    uint32_t held = now - downAt;
    if (!longSent && held > 30 && held < 700) onShortPress();
    S.pressed = false;
    S.holdProgress = 0;
  }

  prev = nowPressed;
}

// ---------- Render ----------
void renderScreen() {
  switch (S.screen) {
    case SCR_BOOT:        drawBoot(); break;
    case SCR_HOME:        drawHome(); break;
    case SCR_MODE_SELECT: drawModeSelect(); break;
    case SCR_QUICK_ASK:   drawQuickAsk(); break;
    case SCR_ORBIT:       drawOrbitCapture(); break;
    case SCR_TRIAD:       drawTriad(); break;
    case SCR_MOOD:        drawMood(); break;
    case SCR_DAILY:       drawDaily(); break;
    case SCR_REVEAL:      drawReveal(); break;
    case SCR_HISTORY:     drawHistory(); break;
    case SCR_SETTINGS:    drawSettings(); break;
    case SCR_SYSTEM_MENU: drawSystemMenu(); break;
  }

  // Short lock feedback: one restrained border flash.
  if (S.locked) {
    ui.drawRoundRect(5, 5, 230, 230, 9, C.clover);
    S.locked = false;
  }

  ui.pushSprite(0, 0);
}

void setup() {
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  ui.setColorDepth(16);
  ui.createSprite(LCD_W, LCD_H);
  ui.setTextWrap(false);

  initTheme();
  S.bootAt = millisNow();
}

void loop() {
  pollInput();

  uint32_t now = millisNow();

  if (S.screen == SCR_BOOT && now - S.bootAt > 1600) {
    S.screen = SCR_HOME;
  }

  // 10 fps maximum: calm motion, low power, avoids needless redraw.
  if (now - S.lastFrame >= 100) {
    S.lastFrame = now;
    renderScreen();
  }

  // Let wheel speed decay after rotation stops.
  if (now - S.lastRotateAt > 240) S.wheelSpeed = 0;
}
