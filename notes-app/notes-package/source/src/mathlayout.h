#pragma once
#include "eadkpp.h"

// A small 2D maths layout engine for a LaTeX subset, drawn with the two
// NumWorks bitmap fonts: fractions, roots, scripts, sized brackets, big
// operators with limits, accents, Greek letters and common symbols.
namespace Math {

enum Font { Small = 0, Large = 1 };

struct Metrics { int w; int h; int asc; };

int glyphW(Font f);
int glyphH(Font f);
int textAsc(Font f);

void reset();                                   // empties the node pool
int parse(const char* s, int len, Font font);   // returns root node or -1
Metrics metrics(int node);
void draw(int node, int x, int baselineY, EADK::Color fg, EADK::Color bg);

}  // namespace Math
