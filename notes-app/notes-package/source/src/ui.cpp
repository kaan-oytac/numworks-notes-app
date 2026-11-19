#include "ui.h"

#include <string.h>

namespace UI {

int textWidth(const char* s, bool large) { return (int)strlen(s) * (large ? LargeW : SmallW); }

void fill(int x, int y, int w, int h, EADK::Color c) {
  if (w <= 0 || h <= 0) return;
  EADK::Display::pushRectUniform(EADK::Rect(x, y, w, h), c);
}

void text(const char* s, int x, int y, bool large, EADK::Color fg, EADK::Color bg) {
  EADK::Display::drawString(s, EADK::Point(x, y), large, fg, bg);
}

void textRight(const char* s, int rightX, int y, bool large, EADK::Color fg, EADK::Color bg) {
  text(s, rightX - textWidth(s, large), y, large, fg, bg);
}

void textCentered(const char* s, int cx, int y, bool large, EADK::Color fg, EADK::Color bg) {
  text(s, cx - textWidth(s, large) / 2, y, large, fg, bg);
}

void titleBar(const char* appName) {
  fill(0, 0, ScreenW, TitleBarH, TitleBar);
  textCentered(appName, ScreenW / 2, (TitleBarH - SmallH) / 2, false, White, TitleBar);
}

void header(const char* title) {
  fill(0, TitleBarH, ScreenW, HeaderH, AppColor);
  textCentered(title, ScreenW / 2, TitleBarH + (HeaderH - LargeH) / 2, true, White, AppColor);
}

void button(const char* label, int cx, int y, int w, int h, bool selected) {
  int x = cx - w / 2;
  // Embossed-style button: 1px border, white (or Select) body
  fill(x, y, w, h, GrayDark);
  fill(x + 1, y + 1, w - 2, h - 2, selected ? Select : White);
  // bottom shadow line
  fill(x + 1, y + h - 2, w - 2, 1, selected ? Select : GrayMiddle);
  textCentered(label, cx, y + (h - LargeH) / 2, true, Black, selected ? Select : White);
}

void textClipped(const char* s, int x, int y, int maxPx, bool large, EADK::Color fg, EADK::Color bg) {
  int cw = large ? LargeW : SmallW;
  int maxChars = maxPx / cw;
  int len = strlen(s);
  if (len <= maxChars) { text(s, x, y, large, fg, bg); return; }
  if (maxChars < 4) return;
  char buf[64];
  int keep = maxChars - 3;
  if (keep > 60) keep = 60;
  memcpy(buf, s, keep);
  buf[keep] = '.'; buf[keep + 1] = '.'; buf[keep + 2] = '.'; buf[keep + 3] = 0;
  text(buf, x, y, large, fg, bg);
}

}  // namespace UI
