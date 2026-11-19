#include "mathlayout.h"

#include <string.h>

#include "ui.h"

namespace Math {

namespace {

enum Kind : uint8_t { kText, kRow, kFrac, kSqrt, kScript, kParen, kBigOp, kAccent, kSpace };

struct Node {
  uint8_t kind;
  uint8_t font;
  uint8_t len;      // text bytes
  uint8_t flag;     // paren chars / accent type / special text (1 = "not equal")
  int16_t w, h, asc;
  int16_t a, b, c;  // children (-1 if none); for kRow: a = first child
  int16_t next;     // sibling in a row
  const char* text;
};

constexpr int kMaxNodes = 384;
Node g_nodes[kMaxNodes];
int g_count;

constexpr int kTextAsc[2] = {11, 14};
constexpr int kAxis[2] = {4, 5};
constexpr int kSupShift[2] = {5, 7};
constexpr int kSubShift[2] = {3, 4};
constexpr int kRadW[2] = {7, 9};
constexpr int kParenW = 5;

int newNode(Kind k, Font f) {
  if (g_count >= kMaxNodes) return -1;
  Node& n = g_nodes[g_count];
  n.kind = k; n.font = f; n.len = 0; n.flag = 0;
  n.w = n.h = n.asc = 0; n.a = n.b = n.c = -1; n.next = -1; n.text = nullptr;
  return g_count++;
}

int textNode(const char* s, int len, Font f) {
  int n = newNode(kText, f);
  if (n < 0) return -1;
  g_nodes[n].text = s; g_nodes[n].len = len > 255 ? 255 : len;
  return n;
}

int utf8Count(const char* s, int len) {
  int c = 0;
  for (int i = 0; i < len; i++) if (((unsigned char)s[i] & 0xC0) != 0x80) c++;
  return c;
}

// --------------------------------------------------------------- measuring
void measure(int id);

void measureRow(Node& n) {
  int w = 0, asc = textAsc((Font)n.font), desc = glyphH((Font)n.font) - asc;
  bool any = false;
  for (int c = n.a; c >= 0; c = g_nodes[c].next) {
    measure(c);
    Node& ch = g_nodes[c];
    w += ch.w;
    if (!any) { asc = ch.asc; desc = ch.h - ch.asc; any = true; }
    else { if (ch.asc > asc) asc = ch.asc; if (ch.h - ch.asc > desc) desc = ch.h - ch.asc; }
  }
  n.w = w; n.asc = asc; n.h = asc + desc;
}

void measure(int id) {
  if (id < 0) return;
  Node& n = g_nodes[id];
  Font f = (Font)n.font;
  switch (n.kind) {
    case kText:
      n.w = utf8Count(n.text, n.len) * glyphW(f);
      n.h = glyphH(f); n.asc = textAsc(f);
      break;
    case kSpace:
      n.w = n.flag; n.h = glyphH(f); n.asc = textAsc(f);
      break;
    case kRow:
      measureRow(n);
      break;
    case kFrac: {
      measure(n.a); measure(n.b);
      Node& num = g_nodes[n.a]; Node& den = g_nodes[n.b];
      int gap = 1;
      n.asc = kAxis[f] + gap + num.h;
      int desc = 1 + gap + den.h - kAxis[f];
      n.h = n.asc + desc;
      n.w = (num.w > den.w ? num.w : den.w) + 4;
      break;
    }
    case kSqrt: {
      measure(n.a);
      Node& body = g_nodes[n.a];
      n.asc = body.asc + 3;
      n.h = body.h + 3 + 1;
      n.w = kRadW[f] + body.w + 2;
      if (n.b >= 0) {
        measure(n.b);
        Node& idx = g_nodes[n.b];
        int extra = idx.w - kRadW[f] / 2;
        if (extra > 0) n.w += extra;
        if (idx.h + 2 > n.asc) { n.h += idx.h + 2 - n.asc; n.asc = idx.h + 2; }
      }
      break;
    }
    case kScript: {
      measure(n.a);
      Node& base = g_nodes[n.a];
      int asc = base.asc, desc = base.h - base.asc, sw = 0;
      if (n.b >= 0) {
        measure(n.b);
        Node& sup = g_nodes[n.b];
        int a = kSupShift[f] + sup.asc;
        if (a > asc) asc = a;
        if (sup.w > sw) sw = sup.w;
      }
      if (n.c >= 0) {
        measure(n.c);
        Node& sub = g_nodes[n.c];
        int d = kSubShift[f] + (sub.h - sub.asc);
        if (d > desc) desc = d;
        if (sub.w > sw) sw = sub.w;
      }
      n.asc = asc; n.h = asc + desc; n.w = base.w + (sw > 0 ? sw + 1 : 0);
      break;
    }
    case kParen: {
      measure(n.a);
      Node& body = g_nodes[n.a];
      n.asc = body.asc + 1; n.h = body.h + 2;
      n.w = body.w + 2 * (kParenW + 1);
      break;
    }
    case kBigOp: {
      measure(n.a);
      Node& sym = g_nodes[n.a];
      int w = sym.w, asc = sym.asc, desc = sym.h - sym.asc;
      if (n.flag == 1) {  // integral: limits sit to the right like scripts
        int sw = 0;
        if (n.b >= 0) { measure(n.b); Node& lo = g_nodes[n.b]; int d = kSubShift[f] + 2 + (lo.h - lo.asc); if (d > desc) desc = d; if (lo.w > sw) sw = lo.w; }
        if (n.c >= 0) { measure(n.c); Node& up = g_nodes[n.c]; int a = kSupShift[f] + 2 + up.asc; if (a > asc) asc = a; if (up.w > sw) sw = up.w; }
        n.w = sym.w + 1 + (sw > 0 ? sw + 1 : 0) + 1; n.asc = asc; n.h = asc + desc;
        break;
      }
      if (n.b >= 0) { measure(n.b); Node& lo = g_nodes[n.b]; desc += 1 + lo.h; if (lo.w > w) w = lo.w; }
      if (n.c >= 0) { measure(n.c); Node& up = g_nodes[n.c]; asc += 1 + up.h; if (up.w > w) w = up.w; }
      n.w = w + 2; n.asc = asc; n.h = asc + desc;
      break;
    }
    case kAccent: {
      measure(n.a);
      Node& body = g_nodes[n.a];
      n.w = body.w; n.asc = body.asc + 3; n.h = body.h + 3;
      break;
    }
  }
}

// ----------------------------------------------------------------- parsing
struct Parser {
  const char* s;
  int n;
  int i;
  Font font;
  bool stoppedAtRight;  // parseSeq stopped on \right
};

struct Symbol { const char* name; const char* utf8; };
const Symbol kSymbols[] = {
    {"alpha", "α"}, {"beta", "β"}, {"gamma", "γ"}, {"delta", "δ"}, {"epsilon", "ε"},
    {"varepsilon", "ε"}, {"zeta", "ζ"}, {"eta", "η"}, {"theta", "θ"}, {"lambda", "λ"},
    {"mu", "μ"}, {"pi", "π"}, {"rho", "ρ"}, {"sigma", "σ"}, {"chi", "χ"}, {"omega", "ω"},
    {"Gamma", "Γ"}, {"Delta", "Δ"}, {"Sigma", "Σ"}, {"Omega", "Ω"},
    // letters the NumWorks font lacks fall back to their names
    {"phi", "phi"}, {"varphi", "phi"}, {"tau", "tau"}, {"kappa", "kappa"}, {"nu", "nu"},
    {"xi", "xi"}, {"psi", "psi"}, {"iota", "iota"}, {"upsilon", "ups"}, {"Theta", "Θ"},
    {"Lambda", "Lambda"}, {"Pi", "Pi"}, {"Phi", "Phi"}, {"Psi", "Psi"}, {"Xi", "Xi"},
    {"cdot", "·"}, {"times", "×"}, {"div", "÷"}, {"pm", "±"}, {"mp", "±"}, {"infty", "∞"},
    {"to", "→"}, {"rightarrow", "→"}, {"Rightarrow", "→"}, {"implies", "→"}, {"mapsto", "→"},
    {"leq", "≤"}, {"le", "≤"}, {"geq", "≥"}, {"ge", "≥"}, {"approx", "≈"}, {"sim", "~"},
    {"ldots", "..."}, {"cdots", "..."}, {"dots", "..."}, {"degree", "°"}, {"circ", "°"},
    {"langle", "⟨"}, {"rangle", "⟩"}, {"partial", "∂"}, {"nabla", "∇"}, {"prime", "'"},
    {"quad", "  "}, {"qquad", "    "}, {"lbrace", "{"}, {"rbrace", "}"}, {"vert", "|"},
    {"ln", "ln"}, {"log", "log"}, {"exp", "exp"}, {"sin", "sin"}, {"cos", "cos"}, {"tan", "tan"},
    {"arcsin", "asin"}, {"arccos", "acos"}, {"arctan", "atan"}, {"sinh", "sinh"}, {"cosh", "cosh"},
    {"tanh", "tanh"}, {"max", "max"}, {"min", "min"}, {"det", "det"}, {"gcd", "gcd"}, {"mod", "mod"},
};

bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool isDigit(char c) { return c >= '0' && c <= '9'; }

int parseSeq(Parser& p, char stop);
int parseAtom(Parser& p);

void skipSpaces(Parser& p) { while (p.i < p.n && (p.s[p.i] == ' ' || p.s[p.i] == '\n' || p.s[p.i] == '\t')) p.i++; }

// Parses `{...}` or a single atom (for \frac arguments and scripts).
int parseArg(Parser& p) {
  skipSpaces(p);
  if (p.i < p.n && p.s[p.i] == '{') {
    p.i++;
    int r = parseSeq(p, '}');
    if (p.i < p.n && p.s[p.i] == '}') p.i++;
    return r;
  }
  return parseAtom(p);
}

bool matchCommand(Parser& p, const char* name) {
  int L = strlen(name);
  if (p.i + 1 + L > p.n) return false;
  if (p.s[p.i] != '\\' || strncmp(p.s + p.i + 1, name, L) != 0) return false;
  if (p.i + 1 + L < p.n && isAlpha(p.s[p.i + 1 + L])) return false;
  return true;
}

int parenNode(int body, char open, char close, Font f) {
  int n = newNode(kParen, f);
  if (n < 0) return body;
  g_nodes[n].a = body; g_nodes[n].flag = (uint8_t)open; g_nodes[n].len = (uint8_t)close;
  return n;
}

int parseCommand(Parser& p) {
  // p.s[p.i] == '\\'
  int start = p.i + 1;
  if (start >= p.n) { p.i = p.n; return -1; }
  char c = p.s[start];
  if (!isAlpha(c)) {
    p.i = start + 1;
    if (c == ',' || c == ';' || c == ' ' || c == '!') { int n = newNode(kSpace, p.font); if (n >= 0) g_nodes[n].flag = c == '!' ? 0 : 3; return n; }
    if (c == '\\') { int n = newNode(kSpace, p.font); if (n >= 0) g_nodes[n].flag = 6; return n; }
    return textNode(p.s + start, 1, p.font);  // \{ \} \$ \% \_ \& \#
  }
  int j = start;
  while (j < p.n && isAlpha(p.s[j])) j++;
  int L = j - start;
  const char* name = p.s + start;
  p.i = j;
  Font f = p.font;
#define IS(x) (L == (int)strlen(x) && strncmp(name, x, L) == 0)
  if (IS("frac") || IS("dfrac") || IS("tfrac")) {
    int a = parseArg(p); int b = parseArg(p);
    int n = newNode(kFrac, f); if (n < 0) return -1;
    g_nodes[n].a = a >= 0 ? a : textNode("", 0, f); g_nodes[n].b = b >= 0 ? b : textNode("", 0, f);
    return n;
  }
  if (IS("sqrt")) {
    int idx = -1;
    skipSpaces(p);
    if (p.i < p.n && p.s[p.i] == '[') { p.i++; idx = parseSeq(p, ']'); if (p.i < p.n && p.s[p.i] == ']') p.i++; }
    int body = parseArg(p);
    int n = newNode(kSqrt, f); if (n < 0) return -1;
    g_nodes[n].a = body >= 0 ? body : textNode("", 0, f); g_nodes[n].b = idx;
    if (idx >= 0) g_nodes[idx].font = Small;
    return n;
  }
  if (IS("left")) {
    skipSpaces(p);
    char open = p.i < p.n ? p.s[p.i] : '(';
    if (open == '\\' && p.i + 1 < p.n) { p.i++; open = p.s[p.i]; }  // \{ \} \|
    p.i++;
    int body = parseSeq(p, 0);
    char close = ')';
    if (p.stoppedAtRight) {
      p.i += 6;  // "\right"
      skipSpaces(p);
      close = p.i < p.n ? p.s[p.i] : ')';
      if (close == '\\' && p.i + 1 < p.n) { p.i++; close = p.s[p.i]; }
      p.i++;
    }
    if (open == '.') open = 0;
    if (close == '.') close = 0;
    return parenNode(body, open, close, f);
  }
  if (IS("right")) { return -1; }
  if (IS("abs")) { int body = parseArg(p); return parenNode(body, '|', '|', f); }
  if (IS("sum") || IS("prod") || IS("int") || IS("oint") || IS("lim")) {
    int n = newNode(kBigOp, f); if (n < 0) return -1;
    const char* sym = IS("sum") ? "∑" : IS("prod") ? "Π" : IS("lim") ? "lim" : "∫";
    g_nodes[n].a = textNode(sym, strlen(sym), f);
    g_nodes[n].flag = IS("int") || IS("oint") ? 1 : 0;  // 1: limits as scripts
    return n;
  }
  if (IS("vec") || IS("bar") || IS("overline") || IS("hat") || IS("dot") || IS("tilde")) {
    int body = parseArg(p);
    int n = newNode(kAccent, f); if (n < 0) return body;
    g_nodes[n].a = body >= 0 ? body : textNode("", 0, f);
    g_nodes[n].flag = IS("vec") ? 1 : IS("hat") ? 2 : IS("dot") ? 3 : IS("tilde") ? 4 : 0;
    return n;
  }
  if (IS("text") || IS("mathrm") || IS("mathbf") || IS("textbf") || IS("textit") || IS("mathit") || IS("operatorname")) {
    skipSpaces(p);
    if (p.i < p.n && p.s[p.i] == '{') {
      int a = p.i + 1, b = a;
      while (b < p.n && p.s[b] != '}') b++;
      p.i = b < p.n ? b + 1 : b;
      return textNode(p.s + a, b - a, f);
    }
    return parseAtom(p);
  }
  if (IS("neq") || IS("ne")) { int n = textNode("=", 1, f); if (n >= 0) g_nodes[n].flag = 1; return n; }
  if (IS("binom")) {
    int a = parseArg(p); int b = parseArg(p);
    int fr = newNode(kFrac, f); if (fr < 0) return -1;
    g_nodes[fr].a = a; g_nodes[fr].b = b; g_nodes[fr].flag = 1;  // no bar
    return parenNode(fr, '(', ')', f);
  }
  if (IS("displaystyle") || IS("limits") || IS("nolimits") || IS("left") || IS("big") || IS("Big")) return parseAtom(p);
  for (unsigned k = 0; k < sizeof(kSymbols) / sizeof(kSymbols[0]); k++) {
    if (IS(kSymbols[k].name)) return textNode(kSymbols[k].utf8, strlen(kSymbols[k].utf8), f);
  }
#undef IS
  return textNode(name, L, f);  // unknown command: show its name
}

int parseAtom(Parser& p) {
  skipSpaces(p);
  if (p.i >= p.n) return -1;
  char c = p.s[p.i];
  Font f = p.font;
  if (c == '{') {
    p.i++;
    int r = parseSeq(p, '}');
    if (p.i < p.n && p.s[p.i] == '}') p.i++;
    return r;
  }
  if (c == '\\') return parseCommand(p);
  if (c == '(' || c == '[') {
    char close = c == '(' ? ')' : ']';
    p.i++;
    int body = parseSeq(p, close);
    if (p.i < p.n && p.s[p.i] == close) p.i++;
    return parenNode(body, c, close, f);
  }
  if (isDigit(c) || (c == '.' && p.i + 1 < p.n && isDigit(p.s[p.i + 1]))) {
    int j = p.i;
    while (j < p.n && (isDigit(p.s[j]) || p.s[j] == '.')) j++;
    int n = textNode(p.s + p.i, j - p.i, f);
    p.i = j;
    return n;
  }
  if (c == '*') { p.i++; return textNode("×", 2, f); }
  if (c == '~') { p.i++; int n = newNode(kSpace, f); if (n >= 0) g_nodes[n].flag = glyphW(f); return n; }
  // any other single byte (letters, operators) or a UTF-8 sequence
  int len = 1;
  if ((unsigned char)c >= 0xC0) { while (p.i + len < p.n && ((unsigned char)p.s[p.i + len] & 0xC0) == 0x80) len++; }
  int n = textNode(p.s + p.i, len, f);
  p.i += len;
  return n;
}

int parseSeq(Parser& p, char stop) {
  int row = newNode(kRow, p.font);
  if (row < 0) return -1;
  int last = -1;
  p.stoppedAtRight = false;
  while (true) {
    skipSpaces(p);
    if (p.i >= p.n) break;
    char c = p.s[p.i];
    if (stop && c == stop) break;
    if ((c == '}' || c == ']' || c == ')') && stop == 0 && c != ')') break;
    if (matchCommand(p, "right")) { p.stoppedAtRight = true; break; }
    if ((c == '^' || c == '_') && last >= 0) {
      p.i++;
      int arg = parseArg(p);
      if (arg < 0) continue;
      g_nodes[arg].font = Small;
      // scripts inside are measured as small: propagate to text children
      Node& prev = g_nodes[last];
      if (prev.kind == kBigOp) {
        if (c == '^') prev.c = arg; else prev.b = arg;
        continue;
      }
      if (prev.kind == kScript && ((c == '^' && prev.b < 0) || (c == '_' && prev.c < 0))) {
        if (c == '^') prev.b = arg; else prev.c = arg;
        continue;
      }
      int sc = newNode(kScript, p.font);
      if (sc < 0) break;
      g_nodes[sc].a = last;
      if (c == '^') g_nodes[sc].b = arg; else g_nodes[sc].c = arg;
      // replace `last` in the row chain by sc
      if (g_nodes[row].a == last) g_nodes[row].a = sc;
      else {
        int q = g_nodes[row].a;
        while (q >= 0 && g_nodes[q].next != last) q = g_nodes[q].next;
        if (q >= 0) g_nodes[q].next = sc;
      }
      g_nodes[sc].next = g_nodes[last].next;
      g_nodes[last].next = -1;
      last = sc;
      continue;
    }
    if (c == '^' || c == '_') { p.i++; continue; }
    int atom = parseAtom(p);
    if (atom < 0) { if (p.i < p.n && p.s[p.i] == stop) break; if (p.i < p.n && (p.s[p.i] == '}' || p.s[p.i] == ']')) { p.i++; } continue; }
    if (last < 0) g_nodes[row].a = atom; else g_nodes[last].next = atom;
    last = atom;
  }
  return row;
}

// Scripts/limits must render in the small font: set recursively.
void setFont(int id, Font f) {
  if (id < 0) return;
  Node& n = g_nodes[id];
  n.font = f;
  if (n.kind == kRow) { for (int c = n.a; c >= 0; c = g_nodes[c].next) setFont(c, f); return; }
  setFont(n.a, f); setFont(n.b, f); setFont(n.c, f);
}

void fixScriptFonts(int id) {
  if (id < 0) return;
  Node& n = g_nodes[id];
  if (n.kind == kRow) { for (int c = n.a; c >= 0; c = g_nodes[c].next) fixScriptFonts(c); return; }
  if (n.kind == kScript) { fixScriptFonts(n.a); setFont(n.b, Small); setFont(n.c, Small); return; }
  if (n.kind == kBigOp) { setFont(n.b, Small); setFont(n.c, Small); return; }
  if (n.kind == kSqrt) { fixScriptFonts(n.a); setFont(n.b, Small); return; }
  fixScriptFonts(n.a); fixScriptFonts(n.b); fixScriptFonts(n.c);
}

// ----------------------------------------------------------------- drawing
void px(int x, int y, EADK::Color c) { UI::fill(x, y, 1, 1, c); }

void line(int x0, int y0, int x1, int y1, EADK::Color c) {
  int dx = x1 - x0, dy = y1 - y0;
  int steps = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
  if (steps == 0) { px(x0, y0, c); return; }
  for (int i = 0; i <= steps; i++) px(x0 + dx * i / steps, y0 + dy * i / steps, c);
}

void drawParen(char ch, int x, int top, int h, EADK::Color c) {
  if (ch == 0) return;
  int bottom = top + h - 1;
  if (ch == '(' || ch == ')') {
    int xs = ch == '(' ? x + 1 : x + kParenW - 2;  // bar column
    int d = ch == '(' ? 1 : -1;
    if (h >= 8) {
      UI::fill(xs, top + 3, 1, h - 6, c);
      px(xs + d, top + 2, c); px(xs + d, bottom - 2, c);
      px(xs + 2 * d, top + 1, c); px(xs + 2 * d, bottom - 1, c);
      px(xs + 3 * d, top, c); px(xs + 3 * d, bottom, c);
    } else {
      UI::fill(xs, top + 1, 1, h - 2, c);
      px(xs + d, top, c); px(xs + d, bottom, c);
    }
  } else if (ch == '[' || ch == ']') {
    int xs = ch == '[' ? x + 1 : x + kParenW - 2;
    int d = ch == '[' ? 1 : -1;
    UI::fill(xs, top, 1, h, c);
    UI::fill(d > 0 ? xs : xs - 2, top, 3, 1, c);
    UI::fill(d > 0 ? xs : xs - 2, bottom, 3, 1, c);
  } else if (ch == '{' || ch == '}') {
    int xs = x + 2; int d = ch == '{' ? -1 : 1;
    UI::fill(xs, top + 1, 1, h - 2, c);
    px(xs - d, top, c); px(xs - d, bottom, c);
    px(xs + d, top + h / 2, c);
  } else {  // '|' and anything else: a bar
    UI::fill(x + 2, top, 1, h, c);
  }
}

void drawNode(int id, int x, int base, EADK::Color fg, EADK::Color bg);

void drawText(const Node& n, int x, int base, EADK::Color fg, EADK::Color bg) {
  char buf[256];
  int L = n.len;
  memcpy(buf, n.text, L); buf[L] = 0;
  Font f = (Font)n.font;
  UI::text(buf, x, base - textAsc(f), f == Large, fg, bg);
  if (n.flag == 1) {  // not equal: slash through "="
    int gw = glyphW(f);
    line(x + gw / 2 - 2, base - 1, x + gw / 2 + 2, base - textAsc(f) + 4, fg);
  }
}

void drawNode(int id, int x, int base, EADK::Color fg, EADK::Color bg) {
  if (id < 0) return;
  const Node& n = g_nodes[id];
  Font f = (Font)n.font;
  switch (n.kind) {
    case kText: drawText(n, x, base, fg, bg); break;
    case kSpace: break;
    case kRow:
      for (int c = n.a; c >= 0; c = g_nodes[c].next) { drawNode(c, x, base, fg, bg); x += g_nodes[c].w; }
      break;
    case kFrac: {
      const Node& num = g_nodes[n.a]; const Node& den = g_nodes[n.b];
      int yb = base - kAxis[f];
      if (n.flag == 0) UI::fill(x + 1, yb, n.w - 2, 1, fg);
      int gap = 1;
      drawNode(n.a, x + (n.w - num.w) / 2, yb - gap - (num.h - num.asc), fg, bg);
      drawNode(n.b, x + (n.w - den.w) / 2, yb + 1 + gap + den.asc, fg, bg);
      break;
    }
    case kSqrt: {
      const Node& body = g_nodes[n.a];
      int rw = kRadW[f];
      int offset = 0;
      if (n.b >= 0) { int extra = g_nodes[n.b].w - rw / 2; if (extra > 0) offset = extra; }
      int top = base - body.asc - 3;        // overbar row
      int bottom = base + (body.h - body.asc);
      int x0 = x + offset;
      // radical: small tick, down stroke, long up stroke, overbar
      int mid = (top + bottom) / 2 + 2;
      line(x0, mid, x0 + 2, mid - 1, fg);
      line(x0 + 2, mid - 1, x0 + rw / 2, bottom, fg);
      line(x0 + rw / 2, bottom, x0 + rw - 1, top, fg);
      UI::fill(x0 + rw - 1, top, body.w + 3, 1, fg);
      drawNode(n.a, x0 + rw + 1, base, fg, bg);
      if (n.b >= 0) { const Node& idx = g_nodes[n.b]; drawNode(n.b, x, mid - 2 - (idx.h - idx.asc), fg, bg); }
      break;
    }
    case kScript: {
      const Node& b = g_nodes[n.a];
      drawNode(n.a, x, base, fg, bg);
      if (n.b >= 0) drawNode(n.b, x + b.w + 1, base - kSupShift[f], fg, bg);
      if (n.c >= 0) drawNode(n.c, x + b.w + 1, base + kSubShift[f], fg, bg);
      break;
    }
    case kParen: {
      const Node& body = g_nodes[n.a];
      int top = base - n.asc;
      drawParen((char)n.flag, x, top, n.h, fg);
      drawNode(n.a, x + kParenW + 1, base, fg, bg);
      drawParen((char)n.len, x + kParenW + 1 + body.w + 1, top, n.h, fg);
      break;
    }
    case kBigOp: {
      const Node& sym = g_nodes[n.a];
      int cx = x + 1;
      if (n.flag == 1) {  // integral: limits as scripts at the right
        drawNode(n.a, cx, base, fg, bg);
        if (n.c >= 0) drawNode(n.c, cx + sym.w + 1, base - kSupShift[f] - 2, fg, bg);
        if (n.b >= 0) drawNode(n.b, cx + sym.w + 1, base + kSubShift[f] + 2, fg, bg);
        break;
      }
      drawNode(n.a, x + (n.w - sym.w) / 2, base, fg, bg);
      if (n.c >= 0) { const Node& up = g_nodes[n.c]; drawNode(n.c, x + (n.w - up.w) / 2, base - sym.asc - 1 - (up.h - up.asc), fg, bg); }
      if (n.b >= 0) { const Node& lo = g_nodes[n.b]; drawNode(n.b, x + (n.w - lo.w) / 2, base + (sym.h - sym.asc) + 1 + lo.asc, fg, bg); }
      break;
    }
    case kAccent: {
      const Node& body = g_nodes[n.a];
      drawNode(n.a, x, base, fg, bg);
      int y = base - body.asc - 2;
      int w = body.w;
      switch (n.flag) {
        case 1: UI::fill(x, y, w, 1, fg); px(x + w - 2, y - 1, fg); px(x + w - 2, y + 1, fg); break;          // vec
        case 2: line(x + 1, y + 1, x + w / 2, y - 1, fg); line(x + w / 2, y - 1, x + w - 2, y + 1, fg); break; // hat
        case 3: UI::fill(x + w / 2 - 1, y - 1, 2, 2, fg); break;                                             // dot
        case 4: line(x, y + 1, x + w / 3, y - 1, fg); line(x + w / 3, y - 1, x + 2 * w / 3, y + 1, fg); line(x + 2 * w / 3, y + 1, x + w - 1, y - 1, fg); break;
        default: UI::fill(x, y, w, 1, fg); break;                                                            // bar
      }
      break;
    }
  }
}

}  // namespace

int glyphW(Font f) { return f == Large ? UI::LargeW : UI::SmallW; }
int glyphH(Font f) { return f == Large ? UI::LargeH : UI::SmallH; }
int textAsc(Font f) { return kTextAsc[f]; }

void reset() { g_count = 0; }

int parse(const char* s, int len, Font font) {
  Parser p; p.s = s; p.n = len; p.i = 0; p.font = font; p.stoppedAtRight = false;
  int root = parseSeq(p, 0);
  if (root < 0) return -1;
  fixScriptFonts(root);
  measure(root);
  return root;
}

Metrics metrics(int node) {
  Metrics m = {0, 0, 0};
  if (node < 0) return m;
  m.w = g_nodes[node].w; m.h = g_nodes[node].h; m.asc = g_nodes[node].asc;
  return m;
}

void draw(int node, int x, int baselineY, EADK::Color fg, EADK::Color bg) { drawNode(node, x, baselineY, fg, bg); }

}  // namespace Math
