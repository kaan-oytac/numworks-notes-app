#pragma once
#include "eadkpp.h"

// NumWorks-like drawing primitives and metrics for the external app.
namespace UI {

// Palette (Epsilon's escher/palette.h values)
constexpr EADK::Color White(0xffffff);
constexpr EADK::Color Black(0x000000);
constexpr EADK::Color TitleBar(0x414147);
constexpr EADK::Color GrayWhite(0xf5f5f5);
constexpr EADK::Color GrayBright(0xececec);
constexpr EADK::Color GrayMiddle(0xd9d9d9);
constexpr EADK::Color GrayDark(0xa7a7a7);
constexpr EADK::Color GrayVeryDark(0x8c8c8c);
constexpr EADK::Color Select(0xd4d7e0);
constexpr EADK::Color WallScreen(0xf7f9fa);
constexpr EADK::Color Red(0xff000c);
constexpr EADK::Color Highlight(0xffebc7);     // YellowLight
constexpr EADK::Color HighlightBar(0xffb734);  // YellowDark
constexpr EADK::Color AppColor(0x5075f2);       // notes blue
constexpr EADK::Color AppColorLight(0xdce3fd);

// Metrics (escher/metric.h)
constexpr int ScreenW = 320;
constexpr int ScreenH = 240;
constexpr int TitleBarH = 18;
constexpr int HeaderH = 27;          // tab/header bar
constexpr int ContentY = TitleBarH + HeaderH;
constexpr int LargeW = 10, LargeH = 18;
constexpr int SmallW = 7, SmallH = 14;
constexpr int CellMargin = 10;

int textWidth(const char* s, bool large);
void fill(int x, int y, int w, int h, EADK::Color c);
void text(const char* s, int x, int y, bool large, EADK::Color fg, EADK::Color bg);
void textRight(const char* s, int rightX, int y, bool large, EADK::Color fg, EADK::Color bg);
void textCentered(const char* s, int cx, int y, bool large, EADK::Color fg, EADK::Color bg);
void titleBar(const char* appName);
void header(const char* title);
void button(const char* label, int cx, int y, int w, int h, bool selected);
// Draws text clipped to maxChars with "..." if it does not fit.
void textClipped(const char* s, int x, int y, int maxPx, bool large, EADK::Color fg, EADK::Color bg);

}  // namespace UI
