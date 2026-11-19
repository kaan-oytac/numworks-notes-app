// Notes: markdown + LaTeX-maths reader for the NumWorks calculator.
#include <string.h>

#include "doc.h"
#include "eadkpp.h"
#include "mathlayout.h"
#include "notes.h"
#include "ui.h"

const char eadk_app_name[]
#if PLATFORM_DEVICE
    __attribute__((section(".rodata.eadk_app_name")))
#endif
    = "Notes";

const uint32_t eadk_api_level
#if PLATFORM_DEVICE
    __attribute__((section(".rodata.eadk_api_level")))
#endif
    = 0;

using E = EADK::Keyboard::Event;

namespace {

constexpr int kRowH = 35;
constexpr int kListY = UI::ContentY;
constexpr int kVisibleRows = (UI::ScreenH - kListY) / kRowH;  // 5
constexpr int kMarginX = 10;
constexpr int kTextW = UI::ScreenW - 2 * kMarginX - 4;  // room for the scroll bar
constexpr int kReaderTop = UI::ContentY + 4;
constexpr int kReaderBottom = UI::ScreenH - 2;

enum Screen { kList, kReader, kContents };

struct Pos { int block; int line; };
struct State {
  int selected;
  int listScroll;
  int note;
  Pos pos;           // first visible block and line within it
  Pos next;          // first position not shown (for paging)
  bool atEnd;
  Pos pageStart[64]; // history of page starts for Left
  int pages;
  Math::Font font;
  int bottom;        // lowest pixel the reader may use (search bar shrinks it)
  Pos saved[Notes::kMax];   // reading position per note
  // table of contents
  int toc[100];      // heading block indices
  int tocCount;
  int tocSel;
  int tocScroll;
  // search
  bool searching;
  char query[32];
  int qlen;
  int matches[300];  // source offsets
  int matchCount;
  int matchIndex;
};
State s;

constexpr int kBarH = UI::SmallH + 6;
constexpr int kTocRowH = 24;
constexpr int kTocRows = (UI::ScreenH - UI::ContentY) / kTocRowH;

// ------------------------------------------------------------- list screen
void drawListRow(int i) {
  int slot = i - s.listScroll;
  if (slot < 0 || slot >= kVisibleRows) return;
  int y = kListY + slot * kRowH;
  bool sel = i == s.selected;
  EADK::Color bg = sel ? UI::Select : UI::White;
  UI::fill(0, y, UI::ScreenW, kRowH - 1, bg);
  UI::fill(0, y + kRowH - 1, UI::ScreenW, 1, UI::GrayBright);
  int nl; const char* nm = Notes::name(i, &nl);
  char buf[40]; int L = nl > 28 ? 28 : nl; memcpy(buf, nm, L); buf[L] = 0;
  UI::text(buf, kMarginX, y + 4, true, UI::Black, bg);
  UI::text(Notes::sourceLabel(i), kMarginX, y + 4 + UI::LargeH - 2, false, UI::GrayDark, bg);
  int tl; Notes::text(i, &tl);
  char sz[16]; int k = 0; int v = (tl + 1023) / 1024; if (v < 1) v = 1;
  if (v >= 10) { sz[k++] = '0' + v / 10; }
  sz[k++] = '0' + v % 10; sz[k++] = ' '; sz[k++] = 'k'; sz[k++] = 'B'; sz[k] = 0;
  UI::textRight(sz, UI::ScreenW - kMarginX, y + 10, false, UI::GrayDark, bg);
}

void drawListScreen() {
  UI::titleBar("NOTES");
  UI::header("Notes");
  UI::fill(0, kListY, UI::ScreenW, UI::ScreenH - kListY, UI::White);
  int n = Notes::count();
  for (int i = s.listScroll; i < n && i < s.listScroll + kVisibleRows; i++) drawListRow(i);
  if (n > kVisibleRows) {
    int trackH = kVisibleRows * kRowH;
    int barH = trackH * kVisibleRows / n;
    int barY = kListY + (trackH - barH) * s.listScroll / (n - kVisibleRows);
    UI::fill(UI::ScreenW - 2, barY, 2, barH, UI::GrayDark);
  }
}

// ----------------------------------------------------------- reader screen
void drawReader() {
  UI::fill(0, UI::ContentY, UI::ScreenW, s.bottom + 2 - UI::ContentY, UI::White);
  int n = Doc::blockCount();
  int y = kReaderTop;
  s.atEnd = true;
  s.next = s.pos;
  for (int b = s.pos.block; b < n; b++) {
    int first = b == s.pos.block ? s.pos.line : 0;
    Doc::layout(b, s.font, kTextW);
    int lines = Doc::lineCount();
    if (first >= lines) first = lines > 0 ? lines - 1 : 0;
    int drawn = Doc::draw(kMarginX, y, s.bottom, UI::White, first);
    if (first + drawn < lines) {
      // this block continues below the screen
      if (drawn == 0 && b == s.pos.block) { drawn = 1; }  // never get stuck on a line taller than the screen
      s.next.block = b; s.next.line = first + drawn; s.atEnd = false;
      break;
    }
    y += (lines > 0 ? Doc::layout(b, s.font, kTextW) - Doc::lineY(first) : 0);
    if (b + 1 < n) { s.next.block = b + 1; s.next.line = 0; s.atEnd = false; }
    else { s.atEnd = true; }
  }
  if (n > 1) {
    int trackH = s.bottom + 2 - UI::ContentY;
    int shown = s.next.block - s.pos.block + 1;
    int barH = trackH * shown / n; if (barH < 6) barH = 6; if (barH > trackH) barH = trackH;
    int barY = UI::ContentY + (trackH - barH) * s.pos.block / (n - 1);
    if (s.atEnd) barY = UI::ContentY + trackH - barH;
    UI::fill(UI::ScreenW - 2, barY, 2, barH, UI::GrayDark);
  }
}

// Copies heading text without Markdown markers, for one-line display.
void plainText(const char* t, int len, char* out, int cap) {
  int n = 0;
  for (int i = 0; i < len && n < cap - 1; i++) {
    char c = t[i];
    if (c == '*' || c == '`' || c == '$' || c == '\\' || c == '#') continue;
    if (c == '{' || c == '}') continue;
    out[n++] = c;
  }
  out[n] = 0;
}

void drawReaderHeader() {
  UI::titleBar("NOTES");
  int tl; const char* t = Doc::title(&tl);
  if (t == nullptr) { t = Notes::name(s.note, &tl); }
  char buf[40];
  plainText(t, tl, buf, 31);
  UI::header(buf);
}

void openNote(int i) {
  s.note = i;
  int len; const char* txt = Notes::text(i, &len);
  Doc::open(txt, len);
  Doc::setHighlight(nullptr, 0);
  s.pos = s.saved[i];
  if (s.pos.block >= Doc::blockCount()) { s.pos.block = 0; s.pos.line = 0; }
  s.pages = 0;
  s.searching = false;
  s.bottom = kReaderBottom;
  drawReaderHeader();
  drawReader();
}

// ------------------------------------------------------------ contents
void buildToc() {
  s.tocCount = 0;
  int n = Doc::blockCount();
  for (int b = 0; b < n && s.tocCount < 100; b++) if (Doc::headingLevel(b) > 0) s.toc[s.tocCount++] = b;
  // select the section the reader is in
  s.tocSel = 0;
  for (int k = 0; k < s.tocCount; k++) if (s.toc[k] <= s.pos.block) s.tocSel = k;
  s.tocScroll = s.tocSel - kTocRows / 2;
  if (s.tocScroll > s.tocCount - kTocRows) s.tocScroll = s.tocCount - kTocRows;
  if (s.tocScroll < 0) s.tocScroll = 0;
}

void drawTocRow(int k) {
  int slot = k - s.tocScroll;
  if (slot < 0 || slot >= kTocRows) return;
  int y = UI::ContentY + slot * kTocRowH;
  bool sel = k == s.tocSel;
  EADK::Color bg = sel ? UI::Select : UI::White;
  UI::fill(0, y, UI::ScreenW, kTocRowH - 1, bg);
  UI::fill(0, y + kTocRowH - 1, UI::ScreenW, 1, UI::GrayBright);
  int level = Doc::headingLevel(s.toc[k]);
  int len; const char* t = Doc::blockText(s.toc[k], &len);
  char buf[64];
  plainText(t, len, buf, sizeof(buf));
  bool large = level == 1;
  int x = kMarginX + (level - 1) * 14;
  if (level > 1) UI::fill(x - 8, y + kTocRowH / 2 - 1, 3, 3, UI::GrayDark);
  UI::textClipped(buf, x, y + (kTocRowH - 1 - (large ? UI::LargeH : UI::SmallH)) / 2, UI::ScreenW - x - kMarginX, large, UI::Black, bg);
}

void drawContents() {
  UI::titleBar("NOTES");
  UI::header("Contents");
  UI::fill(0, UI::ContentY, UI::ScreenW, UI::ScreenH - UI::ContentY, UI::White);
  if (s.tocCount == 0) {
    UI::textCentered("No headings in this note", UI::ScreenW / 2, UI::ContentY + 60, true, UI::Black, UI::White);
    UI::textCentered("Start lines with # to make sections", UI::ScreenW / 2, UI::ContentY + 92, false, UI::GrayVeryDark, UI::White);
    return;
  }
  for (int k = s.tocScroll; k < s.tocCount && k < s.tocScroll + kTocRows; k++) drawTocRow(k);
  if (s.tocCount > kTocRows) {
    int trackH = kTocRows * kTocRowH;
    int barH = trackH * kTocRows / s.tocCount;
    int barY = UI::ContentY + (trackH - barH) * s.tocScroll / (s.tocCount - kTocRows);
    UI::fill(UI::ScreenW - 2, barY, 2, barH, UI::GrayDark);
  }
}

// -------------------------------------------------------------- search
char lower(char c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

void findMatches() {
  s.matchCount = 0;
  if (s.qlen == 0) return;
  int len; const char* src = Doc::source(&len);
  for (int i = 0; i + s.qlen <= len && s.matchCount < 300; i++) {
    int k = 0;
    while (k < s.qlen && lower(src[i + k]) == lower(s.query[k])) k++;
    if (k == s.qlen) { s.matches[s.matchCount++] = i; i += s.qlen - 1; }
  }
}

void drawSearchBar() {
  int y = UI::ScreenH - kBarH;
  UI::fill(0, y, UI::ScreenW, kBarH, UI::GrayMiddle);
  int ty = y + (kBarH - UI::SmallH) / 2;
  char buf[48];
  strcpy(buf, "Find: ");
  strcat(buf, s.query);
  UI::text(buf, kMarginX, ty, false, UI::Black, UI::GrayMiddle);
  UI::fill(kMarginX + UI::textWidth(buf, false), ty, 1, UI::SmallH, UI::Black);  // cursor
  char right[24];
  if (s.qlen == 0) strcpy(right, "type to search");
  else if (s.matchCount == 0) strcpy(right, "no match");
  else {
    int n = 0, v = s.matchIndex + 1, t = s.matchCount;
    char a[8], b[8]; int na = 0, nb = 0;
    do { a[na++] = '0' + v % 10; v /= 10; } while (v);
    do { b[nb++] = '0' + t % 10; t /= 10; } while (t);
    while (na) right[n++] = a[--na];
    right[n++] = '/';
    while (nb) right[n++] = b[--nb];
    right[n] = 0;
  }
  UI::textRight(right, UI::ScreenW - kMarginX, ty, false, s.qlen && !s.matchCount ? UI::Red : UI::GrayVeryDark, UI::GrayMiddle);
}

// Scrolls the reader so that match `index` is on the top line.
void showMatch(int index) {
  if (s.matchCount == 0) { Doc::setHighlight(nullptr, 0); drawReader(); return; }
  s.matchIndex = index;
  int off = s.matches[index];
  int len; const char* src = Doc::source(&len);
  Doc::setHighlight(src + off, s.qlen);
  int b = Doc::blockAtOffset(off);
  Doc::layout(b, s.font, kTextW);
  int line = Doc::lineForOffset(off);
  s.pos.block = b; s.pos.line = line;
  s.pages = 0;
  drawReader();
}

// After the query changed: first match at or after the top of the screen.
void searchFromHere() {
  findMatches();
  if (s.matchCount == 0) { Doc::setHighlight(nullptr, 0); drawReader(); return; }
  int len; const char* top = Doc::blockText(s.pos.block, &len);
  int len2; const char* src = Doc::source(&len2);
  int from = (int)(top - src);
  int k = 0;
  while (k < s.matchCount && s.matches[k] < from) k++;
  if (k == s.matchCount) k = 0;
  showMatch(k);
}

void openSearch() {
  s.searching = true;
  s.qlen = 0; s.query[0] = 0; s.matchCount = 0; s.matchIndex = 0;
  s.bottom = UI::ScreenH - kBarH - 2;
  drawReader();
  drawSearchBar();
}

void closeSearch() {
  s.searching = false;
  Doc::setHighlight(nullptr, 0);
  s.bottom = kReaderBottom;
  drawReader();
}

// Printable single character typed with this event, or 0.
char typedChar(E e) {
  if ((int)e >= 256) return 0;
  const char* t = EADK::Keyboard::getText(e);
  if (t == nullptr || t[0] == 0 || t[1] != 0) return 0;
  char c = t[0];
  return (c >= 32 && c < 127) ? c : 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  s.selected = 0; s.listScroll = 0; s.font = Math::Small; s.pages = 0;
  s.bottom = kReaderBottom; s.searching = false;
  for (int i = 0; i < Notes::kMax; i++) { s.saved[i].block = 0; s.saved[i].line = 0; }
  Notes::scan();
  Screen screen = kList;
  drawListScreen();
  while (true) {
    int32_t timeout = 2000;
    E e = EADK::Keyboard::getEvent(&timeout);
    int n = Notes::count();
    if (screen == kList) {
      if (e == E::Down && s.selected < n - 1) {
        s.selected++;
        if (s.selected >= s.listScroll + kVisibleRows) { s.listScroll++; drawListScreen(); }
        else { drawListRow(s.selected - 1); drawListRow(s.selected); }
      } else if (e == E::Up && s.selected > 0) {
        s.selected--;
        if (s.selected < s.listScroll) { s.listScroll--; drawListScreen(); }
        else { drawListRow(s.selected + 1); drawListRow(s.selected); }
      } else if ((e == E::Ok || e == E::Exe || e == E::Right) && n > 0) {
        screen = kReader;
        openNote(s.selected);
      }
      continue;
    }
    // contents
    if (screen == kContents) {
      if (e == E::Back) { screen = kReader; drawReaderHeader(); drawReader(); continue; }
      if (s.tocCount == 0) continue;
      if (e == E::Down && s.tocSel < s.tocCount - 1) {
        s.tocSel++;
        if (s.tocSel >= s.tocScroll + kTocRows) { s.tocScroll++; drawContents(); }
        else { drawTocRow(s.tocSel - 1); drawTocRow(s.tocSel); }
      } else if (e == E::Up && s.tocSel > 0) {
        s.tocSel--;
        if (s.tocSel < s.tocScroll) { s.tocScroll--; drawContents(); }
        else { drawTocRow(s.tocSel + 1); drawTocRow(s.tocSel); }
      } else if (e == E::Ok || e == E::Exe || e == E::Right) {
        s.pos.block = s.toc[s.tocSel]; s.pos.line = 0; s.pages = 0;
        screen = kReader; drawReaderHeader(); drawReader();
      }
      continue;
    }
    // reader: search bar
    int blocks = Doc::blockCount();
    if (s.searching) {
      if (e == E::Back) { closeSearch(); continue; }
      if (e == E::Backspace) {
        if (s.qlen > 0) { s.query[--s.qlen] = 0; searchFromHere(); drawSearchBar(); }
        else { closeSearch(); }
        continue;
      }
      if ((e == E::Exe || e == E::Ok || e == E::Down) && s.matchCount > 0) {
        showMatch((s.matchIndex + 1) % s.matchCount); drawSearchBar(); continue;
      }
      if (e == E::Up && s.matchCount > 0) {
        showMatch((s.matchIndex + s.matchCount - 1) % s.matchCount); drawSearchBar(); continue;
      }
      char c = typedChar(e);
      if (c && s.qlen < (int)sizeof(s.query) - 1) {
        s.query[s.qlen++] = c; s.query[s.qlen] = 0;
        searchFromHere(); drawSearchBar();
      }
      continue;
    }
    // reader
    if (e == E::Back) { s.saved[s.note] = s.pos; screen = kList; drawListScreen(); continue; }
    if (e == E::Ok || e == E::Exe) { buildToc(); screen = kContents; drawContents(); continue; }
    if (e == E::Down) {
      if (s.atEnd && s.next.block == s.pos.block && s.next.line == s.pos.line) continue;
      // advance one line within the current block, or to the next block
      Doc::layout(s.pos.block, s.font, kTextW);
      if (s.pos.line + 1 < Doc::lineCount()) s.pos.line++;
      else if (s.pos.block + 1 < blocks) { s.pos.block++; s.pos.line = 0; }
      else continue;
      drawReader();
    } else if (e == E::Up) {
      if (s.pos.line > 0) s.pos.line--;
      else if (s.pos.block > 0) { s.pos.block--; Doc::layout(s.pos.block, s.font, kTextW); s.pos.line = Doc::lineCount() - 1; if (s.pos.line < 0) s.pos.line = 0; }
      else continue;
      drawReader();
    } else if ((e == E::Right || e == E::Shift_down) && !s.atEnd) {
      if (s.pages < 64) s.pageStart[s.pages++] = s.pos;
      s.pos = s.next; drawReader();
    } else if ((e == E::Left || e == E::Shift_up)) {
      if (s.pages > 0) { s.pos = s.pageStart[--s.pages]; drawReader(); }
      else if (s.pos.block > 0 || s.pos.line > 0) { s.pos.block = 0; s.pos.line = 0; drawReader(); }
    } else if (e == E::Toolbox) {
      s.font = s.font == Math::Small ? Math::Large : Math::Small;
      s.pos.line = 0; s.pages = 0;
      drawReader();
    } else {
      char c = typedChar(e);
      if (c && c != ' ') {
        openSearch();
        s.query[0] = c; s.query[1] = 0; s.qlen = 1;
        searchFromHere(); drawSearchBar();
      }
    }
  }
  return 0;
}
