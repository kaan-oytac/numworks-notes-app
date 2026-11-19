#include "doc.h"

#include <string.h>

#include "ui.h"

namespace Doc {

namespace {

enum Kind : uint8_t { kPara, kHeading, kBullet, kNumbered, kQuote, kRule, kCode, kMath, kTable, kBlank };

struct Block {
  uint8_t kind;
  uint8_t level;      // heading level / list indent
  uint16_t numStart;  // numbered list: start of the number text
  uint8_t numLen;
  int start;          // source offset
  int len;
};

struct Item {
  uint8_t isMath;
  uint8_t style;      // 1 bold, 2 italic, 4 code
  uint8_t glue;       // no space before
  uint8_t line;       // line, or table row
  uint8_t col;        // table column
  int16_t x, w, h, asc;
  int16_t node;       // math node
  const char* text;
  uint16_t len;
};

struct Line { int16_t h, asc, y; };

constexpr int kMaxItems = 300;
constexpr int kMaxLines = 60;

const char* g_src;
int g_len;
Block g_blocks[kMaxBlocks];
int g_count;

// layout scratch
Item g_items[kMaxItems];
int g_nItems;
Line g_lines[kMaxLines];
int g_nLines;
int g_cur;              // block laid out
Math::Font g_font;
int g_width;
int g_indent;
int g_blockH;
constexpr int kMaxMathLines = 6;
int g_mathNodes[kMaxMathLines];   // display math, one node per sub-line
int g_nMath;

// tables
constexpr int kMaxCols = 8;
constexpr int kCellPad = 4;
int g_nCols;
int g_headerRows;
uint8_t g_align[kMaxCols];        // 0 left, 1 center, 2 right
int16_t g_colX[kMaxCols + 1];     // left edge of each column, then the right edge
int16_t g_cellW[kMaxLines][kMaxCols];
bool g_tableClipped;

// search highlight
const char* g_hl;
int g_hlLen;

bool isBlank(const char* s, int len) { for (int i = 0; i < len; i++) if (s[i] != ' ' && s[i] != '\t' && s[i] != '\r') return false; return true; }
bool startsWith(const char* s, int len, const char* p) { int L = strlen(p); return len >= L && strncmp(s, p, L) == 0; }

void addBlock(Kind k, int level, int start, int len, int numStart = 0, int numLen = 0) {
  if (g_count >= kMaxBlocks) return;
  Block& b = g_blocks[g_count++];
  b.kind = k; b.level = level; b.start = start; b.len = len; b.numStart = numStart; b.numLen = numLen;
}

}  // namespace

void open(const char* text, int len) {
  g_src = text; g_len = len; g_count = 0;
  int i = 0;
  int paraStart = -1;
  bool inCode = false;
  auto flushPara = [&](int end) {
    if (paraStart >= 0) { addBlock(kPara, 0, paraStart, end - paraStart); paraStart = -1; }
  };
  while (i < len) {
    int e = i; while (e < len && text[e] != '\n') e++;
    const char* ln = text + i; int L = e - i;
    if (L > 0 && ln[L - 1] == '\r') L--;
    int ind = 0; while (ind < L && ln[ind] == ' ') ind++;
    const char* t = ln + ind; int tl = L - ind;
    if (inCode) {
      if (startsWith(t, tl, "```")) inCode = false; else addBlock(kCode, 0, i, L);
      i = e + 1; continue;
    }
    if (isBlank(ln, L)) { flushPara(i); i = e + 1; continue; }
    if (startsWith(t, tl, "```")) { flushPara(i); inCode = true; i = e + 1; continue; }
    if (startsWith(t, tl, "$$")) {
      flushPara(i);
      int s = i + ind + 2;
      int close = -1;
      // same line?
      for (int k = s; k + 1 < len && text[k] != '\n'; k++) if (text[k] == '$' && text[k + 1] == '$') { close = k; break; }
      if (close < 0) { for (int k = s; k + 1 < len; k++) if (text[k] == '$' && text[k + 1] == '$') { close = k; break; } }
      if (close < 0) close = len;
      addBlock(kMath, 0, s, close - s);
      int after = close + 2; while (after < len && text[after] != '\n') after++;
      i = after + 1; continue;
    }
    if (t[0] == '|') {
      flushPara(i);
      int start = i, end = e;
      int j = e + 1;
      while (j < len) {
        int je = j; while (je < len && text[je] != '\n') je++;
        int k = j; while (k < je && text[k] == ' ') k++;
        if (k >= je || text[k] != '|') break;
        end = je; j = je + 1;
      }
      addBlock(kTable, 0, start, end - start);
      i = end + 1; continue;
    }
    if (t[0] == '#') {
      int lvl = 0; while (lvl < tl && t[lvl] == '#') lvl++;
      if (lvl <= 4 && lvl < tl && t[lvl] == ' ') { flushPara(i); addBlock(kHeading, lvl, i + ind + lvl + 1, tl - lvl - 1); i = e + 1; continue; }
    }
    if ((startsWith(t, tl, "---") || startsWith(t, tl, "***") || startsWith(t, tl, "___")) && tl >= 3) {
      bool rule = true; for (int k = 0; k < tl; k++) if (t[k] != t[0] && t[k] != ' ') rule = false;
      if (rule) { flushPara(i); addBlock(kRule, 0, i, 0); i = e + 1; continue; }
    }
    if (tl >= 2 && (t[0] == '-' || t[0] == '*' || t[0] == '+') && t[1] == ' ') {
      flushPara(i); addBlock(kBullet, ind / 2, i + ind + 2, tl - 2); i = e + 1; continue;
    }
    if (tl >= 3 && t[0] >= '0' && t[0] <= '9') {
      int k = 0; while (k < tl && t[k] >= '0' && t[k] <= '9') k++;
      if (k < tl - 1 && (t[k] == '.' || t[k] == ')') && t[k + 1] == ' ') {
        flushPara(i); addBlock(kNumbered, ind / 2, i + ind + k + 2, tl - k - 2, i + ind, k + 1); i = e + 1; continue;
      }
    }
    if (t[0] == '>') {
      flushPara(i);
      int s = 1; if (s < tl && t[s] == ' ') s++;
      addBlock(kQuote, 0, i + ind + s, tl - s); i = e + 1; continue;
    }
    if (ind >= 4) { flushPara(i); addBlock(kCode, 0, i + 4, L - 4); i = e + 1; continue; }
    if (paraStart < 0) paraStart = i;
    i = e + 1;
  }
  flushPara(len);
}

int blockCount() { return g_count; }

const char* title(int* len) {
  for (int i = 0; i < g_count; i++) {
    if (g_blocks[i].kind == kHeading) { *len = g_blocks[i].len; return g_src + g_blocks[i].start; }
  }
  return nullptr;
}

int headingLevel(int i) { return (i >= 0 && i < g_count && g_blocks[i].kind == kHeading) ? g_blocks[i].level : 0; }

const char* blockText(int i, int* len) { *len = g_blocks[i].len; return g_src + g_blocks[i].start; }

const char* source(int* len) { *len = g_len; return g_src; }

int blockAtOffset(int off) {
  for (int i = 0; i < g_count; i++) {
    const Block& b = g_blocks[i];
    if (off < b.start + b.len) return i;  // inside it, or in the markup just before it
  }
  return g_count > 0 ? g_count - 1 : 0;
}

void setHighlight(const char* p, int len) { g_hl = p; g_hlLen = len; }

namespace {

int utf8Count(const char* s, int len) { int c = 0; for (int i = 0; i < len; i++) if (((unsigned char)s[i] & 0xC0) != 0x80) c++; return c; }

void addTextItem(const char* s, int len, uint8_t style, bool glue, Math::Font f) {
  if (len <= 0 || g_nItems >= kMaxItems) return;
  Item& it = g_items[g_nItems++];
  it.isMath = 0; it.style = style; it.glue = glue; it.text = s; it.len = len; it.node = -1;
  it.w = utf8Count(s, len) * Math::glyphW(f);
  if (style & 4) it.w += 4;
  it.h = Math::glyphH(f); it.asc = Math::textAsc(f);
}

void addMathItem(const char* s, int len, bool glue, Math::Font f) {
  if (g_nItems >= kMaxItems) return;
  int node = Math::parse(s, len, f);
  if (node < 0) return;
  Math::Metrics m = Math::metrics(node);
  Item& it = g_items[g_nItems++];
  it.isMath = 1; it.style = 0; it.glue = glue; it.text = s; it.len = len; it.node = node;
  it.w = m.w; it.h = m.h; it.asc = m.asc;
}

// Splits inline text into word/math items with styles.
void tokenize(const char* s, int len, Math::Font f, uint8_t baseStyle) {
  uint8_t style = baseStyle;
  int wordStart = -1;
  bool glue = false;  // next item attaches to previous without a space
  auto flushWord = [&](int end) {
    if (wordStart >= 0) { addTextItem(s + wordStart, end - wordStart, style, glue, f); glue = true; wordStart = -1; }
  };
  for (int i = 0; i < len; i++) {
    char c = s[i];
    if (c == '\\' && i + 1 < len && (s[i + 1] == '$' || s[i + 1] == '*' || s[i + 1] == '`' || s[i + 1] == '\\')) {
      if (wordStart < 0) wordStart = i + 1;
      // keep escaped char as part of word: shift start if word empty
      i++;
      continue;
    }
    if (c == '$' && !(style & 4)) {
      int close = -1;
      for (int k = i + 1; k < len; k++) if (s[k] == '$' && s[k - 1] != '\\') { close = k; break; }
      if (close > i + 1) {
        flushWord(i);
        addMathItem(s + i + 1, close - i - 1, glue, f);
        glue = true;
        i = close;
        continue;
      }
    }
    if (c == '`') { flushWord(i); style ^= 4; continue; }
    if (c == '*' && !(style & 4)) {
      if (i + 1 < len && s[i + 1] == '*') { flushWord(i); style ^= 1; i++; continue; }
      flushWord(i); style ^= 2; continue;
    }
    if (c == ' ' || c == '\n' || c == '\t' || c == '\r') { flushWord(i); glue = false; continue; }
    if (wordStart < 0) wordStart = i;
  }
  flushWord(len);
}

// Greedy line breaking of g_items into g_lines.
void breakLines(Math::Font f, int indent, int width) {
  g_nLines = 0;
  int space = Math::glyphW(f);
  int x = indent;
  int lineStart = 0;
  auto closeLine = [&](int end) {
    if (g_nLines >= kMaxLines) return;
    Line& L = g_lines[g_nLines];
    int asc = Math::textAsc(f), desc = Math::glyphH(f) - asc;
    for (int k = lineStart; k < end; k++) {
      const Item& it = g_items[k];
      if (it.asc > asc) asc = it.asc;
      if (it.h - it.asc > desc) desc = it.h - it.asc;
    }
    L.asc = asc; L.h = asc + desc + 2; L.y = 0;
    for (int k = lineStart; k < end; k++) g_items[k].line = g_nLines;
    g_nLines++;
    lineStart = end;
  };
  for (int k = 0; k < g_nItems; k++) {
    Item& it = g_items[k];
    int sp = (k == lineStart || it.glue) ? 0 : space;
    if (x + sp + it.w > width && k > lineStart) {
      closeLine(k);
      x = indent; sp = 0;
    }
    it.x = x + sp;
    x = it.x + it.w;
  }
  if (g_nItems == 0) { g_nItems = 0; }
  closeLine(g_nItems);
  int y = 0;
  for (int l = 0; l < g_nLines; l++) { g_lines[l].y = y; y += g_lines[l].h; }
}

bool isSeparatorRow(const char* s, int len) {
  bool dash = false;
  for (int i = 0; i < len; i++) {
    char c = s[i];
    if (c == '-') dash = true;
    else if (c != '|' && c != ':' && c != ' ' && c != '\t' && c != '\r') return false;
  }
  return dash;
}

// Splits a table row into cells; '|' inside $...$ or escaped as \| does not split.
int splitCells(const char* s, int len, const char** cells, int* lens) {
  int a = 0; while (a < len && (s[a] == ' ' || s[a] == '\t')) a++;
  int b = len; while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\r' || s[b - 1] == '\t')) b--;
  if (a < b && s[a] == '|') a++;
  if (b > a && s[b - 1] == '|' && !(b - 2 >= a && s[b - 2] == '\\')) b--;
  int n = 0, start = a;
  bool inMath = false;
  for (int i = a; i <= b; i++) {
    if (i < b && s[i] == '$' && (i == 0 || s[i - 1] != '\\')) inMath = !inMath;
    if (i == b || (s[i] == '|' && !inMath && !(i > 0 && s[i - 1] == '\\'))) {
      if (n < kMaxCols) {
        int cs = start, ce = i;
        while (cs < ce && s[cs] == ' ') cs++;
        while (ce > cs && s[ce - 1] == ' ') ce--;
        cells[n] = s + cs; lens[n] = ce - cs; n++;
      }
      start = i + 1;
    }
  }
  return n;
}

void parseAlignment(const char* s, int len) {
  const char* cells[kMaxCols]; int lens[kMaxCols];
  int n = splitCells(s, len, cells, lens);
  for (int c = 0; c < n; c++) {
    bool l = lens[c] > 0 && cells[c][0] == ':';
    bool r = lens[c] > 0 && cells[c][lens[c] - 1] == ':';
    g_align[c] = (l && r) ? 1 : (r ? 2 : 0);
  }
}

// Returns false when the table is wider than `width`.
bool layoutTable(const char* s, int len, Math::Font f, int width) {
  g_nItems = 0; g_nLines = 0; g_nCols = 0; g_headerRows = 0;
  g_tableClipped = false;
  for (int c = 0; c < kMaxCols; c++) g_align[c] = 0;
  int space = Math::glyphW(f);
  int16_t colW[kMaxCols] = {0};
  int p = 0;
  while (p < len && g_nLines < kMaxLines) {
    int e = p; while (e < len && s[e] != '\n') e++;
    if (isSeparatorRow(s + p, e - p)) {
      parseAlignment(s + p, e - p);
      if (g_headerRows == 0) g_headerRows = g_nLines;
      p = e + 1; continue;
    }
    const char* cells[kMaxCols]; int lens[kMaxCols];
    int n = splitCells(s + p, e - p, cells, lens);
    int row = g_nLines;
    int asc = Math::textAsc(f), desc = Math::glyphH(f) - asc;
    for (int c = 0; c < n; c++) {
      int first = g_nItems;
      tokenize(cells[c], lens[c], f, 0);
      int x = 0;
      for (int k = first; k < g_nItems; k++) {
        Item& it = g_items[k];
        int sp = (k == first || it.glue) ? 0 : space;
        it.x = x + sp; x = it.x + it.w;
        it.line = row; it.col = c;
        if (it.asc > asc) asc = it.asc;
        if (it.h - it.asc > desc) desc = it.h - it.asc;
      }
      g_cellW[row][c] = x;
      if (x > colW[c]) colW[c] = x;
    }
    for (int c = n; c < kMaxCols; c++) g_cellW[row][c] = 0;
    if (n > g_nCols) g_nCols = n;
    Line& L = g_lines[g_nLines++];
    L.asc = asc + 3; L.h = asc + desc + 6; L.y = 0;
    p = e + 1;
  }
  // header rows are the rows above the |---| line; bold them
  for (int k = 0; k < g_nItems; k++) if (g_items[k].line < g_headerRows && !g_items[k].isMath) g_items[k].style |= 1;
  int x = 0;
  for (int c = 0; c < g_nCols; c++) { g_colX[c] = x; x += colW[c] + 2 * kCellPad + 1; }
  g_colX[g_nCols] = x;
  int y = 1;
  for (int l = 0; l < g_nLines; l++) { g_lines[l].y = y; y += g_lines[l].h; }
  return x <= width;
}

}  // namespace

int layout(int i, Math::Font bodyFont, int width) {
  g_cur = i; g_font = bodyFont; g_width = width; g_nItems = 0; g_nLines = 0; g_nMath = 0;
  Math::reset();
  const Block& b = g_blocks[i];
  const char* s = g_src + b.start;
  int len = b.len;
  switch (b.kind) {
    case kHeading: {
      Math::Font f = b.level <= 2 ? Math::Large : bodyFont;
      g_font = f;
      g_indent = 0;
      tokenize(s, len, f, b.level == 1 || b.level == 3 ? 1 : 0);
      breakLines(f, 0, width);
      g_blockH = g_lines[g_nLines - 1].y + g_lines[g_nLines - 1].h + (b.level == 1 ? 8 : 4) + (b.level <= 2 ? 4 : 0);
      return g_blockH;
    }
    case kBullet:
    case kNumbered: {
      int numW = b.kind == kNumbered ? (b.numLen + 1) * Math::glyphW(bodyFont) : 12;
      g_indent = b.level * 12 + numW;
      tokenize(s, len, bodyFont, 0);
      breakLines(bodyFont, g_indent, width);
      g_blockH = g_lines[g_nLines - 1].y + g_lines[g_nLines - 1].h + 1;
      return g_blockH;
    }
    case kQuote: {
      g_indent = 8;
      tokenize(s, len, bodyFont, 2);
      breakLines(bodyFont, g_indent, width);
      g_blockH = g_lines[g_nLines - 1].y + g_lines[g_nLines - 1].h + 3;
      return g_blockH;
    }
    case kCode: {
      g_indent = 4;
      addTextItem(s, len, 4, true, bodyFont);
      g_items[0].w = utf8Count(s, len) * Math::glyphW(bodyFont);
      breakLines(bodyFont, g_indent, 10000);
      g_blockH = Math::glyphH(bodyFont) + 2;
      return g_blockH;
    }
    case kRule: g_blockH = 9; return g_blockH;
    case kTable: {
      // Too wide in the large font: retry in the small one, then clip.
      if (!layoutTable(s, len, bodyFont, width) && bodyFont == Math::Large) {
        Math::reset();
        g_font = Math::Small;
        layoutTable(s, len, Math::Small, width);
      }
      g_tableClipped = g_colX[g_nCols] > width;
      g_blockH = (g_nLines > 0 ? g_lines[g_nLines - 1].y + g_lines[g_nLines - 1].h : 0) + 5;
      return g_blockH;
    }
    case kMath: {
      // Split at \\, \quad and \qquad; each piece gets its own line. A
      // piece wider than the screen is retried in the small font.
      g_nMath = 0; g_nLines = 0;
      int y = 0;
      int p = 0;
      while (p <= len && g_nMath < kMaxMathLines) {
        int q = p; int cut = 0;
        while (q < len) {
          if (s[q] == '\\' && q + 1 < len && s[q + 1] == '\\') { cut = 2; break; }
          if (s[q] == '\\' && q + 5 <= len && strncmp(s + q, "\\quad", 5) == 0 && (q + 5 == len || !((s[q + 5] >= 'a' && s[q + 5] <= 'z')))) { cut = 5; break; }
          if (s[q] == '\\' && q + 6 <= len && strncmp(s + q, "\\qquad", 6) == 0) { cut = 6; break; }
          q++;
        }
        int pieceLen = q - p;
        if (cut != 0) {  // drop the comma and spaces left before a \quad cut
          while (pieceLen > 0 && (s[p + pieceLen - 1] == ' ' || s[p + pieceLen - 1] == ',')) pieceLen--;
        }
        if (!isBlank(s + p, pieceLen)) {
          int node = Math::parse(s + p, pieceLen, Math::Large);
          Math::Metrics m = Math::metrics(node);
          if (m.w > width) { Math::reset(); for (int k = 0; k < g_nMath; k++) g_mathNodes[k] = Math::parse(g_items[k].text, g_items[k].len, (Math::Font)g_items[k].style); node = Math::parse(s + p, pieceLen, Math::Small); m = Math::metrics(node); g_items[g_nMath].style = Math::Small; }
          else g_items[g_nMath].style = Math::Large;
          g_items[g_nMath].text = s + p; g_items[g_nMath].len = pieceLen;
          g_mathNodes[g_nMath] = node;
          Line& L = g_lines[g_nLines]; L.y = y; L.h = m.h + 6; L.asc = m.asc + 3;
          y += L.h; g_nLines++; g_nMath++;
        }
        if (cut == 0) break;
        p = q + cut;
      }
      if (g_nMath == 0) { g_blockH = 0; return 0; }
      g_blockH = y + 2;
      return g_blockH;
    }
    default: {
      g_indent = 0;
      tokenize(s, len, bodyFont, 0);
      breakLines(bodyFont, 0, width);
      g_blockH = g_lines[g_nLines - 1].y + g_lines[g_nLines - 1].h + 4;
      return g_blockH;
    }
  }
}

namespace {

void drawTextItem(const Item& it, int x, int base, EADK::Color bg) {
  char buf[256];
  int L = it.len; if (L > 255) L = 255;
  memcpy(buf, it.text, L); buf[L] = 0;
  Math::Font f = g_font;
  int ty = base - Math::textAsc(f);
  EADK::Color fg = UI::Black;
  EADK::Color back = bg;
  bool hl = g_hl && it.text < g_hl + g_hlLen && g_hl < it.text + it.len;
  if (hl) back = UI::Highlight;
  if (it.style & 4) { if (!hl) back = UI::GrayBright; UI::fill(x, ty, it.w, Math::glyphH(f), back); x += 2; }
  else if (hl) UI::fill(x, ty, it.w, Math::glyphH(f), back);
  if (it.style & 2) fg = UI::AppColor;
  UI::text(buf, x, ty, f == Math::Large, fg, back);
  if (it.style & 1) UI::text(buf, x + 1, ty, f == Math::Large, fg, back);  // faux bold
}

}  // namespace

namespace {

void drawMathItem(const Item& it, int x, int base, EADK::Color bg) {
  bool hl = g_hl && it.text < g_hl + g_hlLen && g_hl < it.text + it.len;
  if (hl) { UI::fill(x, base - it.asc, it.w, it.h, UI::Highlight); bg = UI::Highlight; }
  Math::draw(it.node, x, base, UI::Black, bg);
}

int drawTable(int x0, int y0, int maxY, EADK::Color bg, int firstLine) {
  int right = g_colX[g_nCols]; if (right > g_width) right = g_width;
  int shift = firstLine < g_nLines ? g_lines[firstLine].y - 1 : 0;
  int drawn = 0;
  for (int l = firstLine; l < g_nLines; l++) {
    const Line& L = g_lines[l];
    int y = y0 + L.y - shift;
    if (y + L.h > maxY) break;
    if (l == firstLine) UI::fill(x0, y - 1, right + 1, 1, UI::GrayMiddle);       // top border
    EADK::Color rowBg = l < g_headerRows ? UI::GrayWhite : bg;
    UI::fill(x0 + 1, y, right - 1, L.h - 1, rowBg);
    UI::fill(x0, y + L.h - 1, right + 1, 1, l + 1 == g_headerRows ? UI::GrayDark : UI::GrayMiddle);
    for (int c = 0; c <= g_nCols; c++) {
      int cx = x0 + g_colX[c];
      if (g_colX[c] <= g_width) UI::fill(cx, y, 1, L.h - 1, UI::GrayMiddle);
    }
    int base = y + L.asc;
    for (int k = 0; k < g_nItems; k++) {
      const Item& it = g_items[k];
      if (it.line != l) continue;
      int c = it.col;
      int colW = g_colX[c + 1] - g_colX[c] - 1 - 2 * kCellPad;
      int off = g_align[c] == 1 ? (colW - g_cellW[l][c]) / 2 : (g_align[c] == 2 ? colW - g_cellW[l][c] : 0);
      int x = g_colX[c] + 1 + kCellPad + off + it.x;
      if (x + it.w > g_width) continue;  // clipped column
      if (it.isMath) drawMathItem(it, x0 + x, base, rowBg);
      else drawTextItem(it, x0 + x, base, rowBg);
    }
    if (g_tableClipped) UI::text(">", x0 + g_width - UI::SmallW, base - Math::textAsc(Math::Small), false, UI::GrayDark, rowBg);
    drawn++;
  }
  return drawn;
}

}  // namespace

int lineForOffset(int off) {
  const char* p = g_src + off;
  const Block& b = g_blocks[g_cur];
  if (b.kind == kRule || b.kind == kCode) return 0;
  if (b.kind == kMath) {
    for (int k = g_nMath - 1; k >= 0; k--) if (p >= g_items[k].text) return k;
    return 0;
  }
  int best = 0;
  for (int k = 0; k < g_nItems; k++) {
    const Item& it = g_items[k];
    if (p >= it.text && p < it.text + it.len) return it.line;
    if (it.text <= p) best = it.line;
  }
  return best;
}

int lineCount() {
  const Block& b = g_blocks[g_cur];
  if (b.kind == kRule || b.kind == kCode) return 1;
  return g_nLines;
}

int lineY(int line) { return line < g_nLines ? g_lines[line].y : 0; }

int draw(int x0, int y0, int maxY, EADK::Color bg, int firstLine) {
  const Block& b = g_blocks[g_cur];
  if (b.kind == kRule) {
    if (y0 + g_blockH > maxY) return 0;
    UI::fill(x0, y0 + 4, g_width, 1, UI::GrayMiddle);
    return 1;
  }
  if (b.kind == kMath) {
    int drawn = 0;
    int shift = lineY(firstLine);
    for (int l = firstLine; l < g_nLines; l++) {
      const Line& L = g_lines[l];
      int y = y0 + L.y - shift;
      if (y + L.h > maxY) return drawn;
      Math::Metrics m = Math::metrics(g_mathNodes[l]);
      int x = x0 + (g_width - m.w) / 2; if (x < x0) x = x0;
      Math::draw(g_mathNodes[l], x, y + L.asc, UI::Black, bg);
      if (g_hl && g_items[l].text < g_hl + g_hlLen && g_hl < g_items[l].text + g_items[l].len) UI::fill(x0, y + 2, 3, L.h - 4, UI::HighlightBar);
      drawn++;
    }
    return drawn;
  }
  if (b.kind == kTable) return drawTable(x0, y0, maxY, bg, firstLine);
  if (b.kind == kCode) {
    if (y0 + g_blockH > maxY) return 0;
    UI::fill(x0, y0, g_width, g_blockH, UI::GrayBright);
    if (g_nItems > 0) {
      char buf[256]; int L = g_items[0].len; if (L > 255) L = 255;
      memcpy(buf, g_items[0].text, L); buf[L] = 0;
      UI::textClipped(buf, x0 + 4, y0 + 1, g_width - 8, g_font == Math::Large, UI::Black, UI::GrayBright);
    }
    return 1;
  }
  int drawn = 0;
  int shift = lineY(firstLine);
  for (int l = firstLine; l < g_nLines; l++) {
    const Line& L = g_lines[l];
    int y = y0 + L.y - shift;
    if (y + L.h > maxY) return drawn;
    drawn++;
    int base = y + L.asc;
    if (l == 0) {
      if (b.kind == kBullet) UI::fill(x0 + b.level * 12 + 3, base - 6, 4, 4, UI::Black);
      if (b.kind == kNumbered) {
        char num[8]; int n = b.numLen; if (n > 7) n = 7;
        memcpy(num, g_src + b.numStart, n); num[n] = 0;
        UI::text(num, x0 + b.level * 12, base - Math::textAsc(g_font), g_font == Math::Large, UI::Black, bg);
      }
    }
    if (b.kind == kQuote) UI::fill(x0, y, 2, L.h, UI::AppColor);
    for (int k = 0; k < g_nItems; k++) {
      const Item& it = g_items[k];
      if (it.line != l) continue;
      if (it.isMath) drawMathItem(it, x0 + it.x, base, bg);
      else drawTextItem(it, x0 + it.x, base, bg);
    }
  }
  if (b.kind == kHeading && b.level == 1) {
    int y = y0 + g_lines[g_nLines - 1].y - shift + g_lines[g_nLines - 1].h + 2;
    UI::fill(x0, y, g_width, 2, UI::AppColor);
  }
  return drawn;
}

}  // namespace Doc
