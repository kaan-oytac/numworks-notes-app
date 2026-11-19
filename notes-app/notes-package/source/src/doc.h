#pragma once
#include "eadkpp.h"
#include "mathlayout.h"

// Markdown-ish document: splits a note into blocks, lays one block out at a
// time (so memory stays small) and draws it.
namespace Doc {

constexpr int kMaxBlocks = 400;

void open(const char* text, int len);
int blockCount();
// Lays out block i for the given body font and width. Returns its height.
int layout(int i, Math::Font bodyFont, int width);
// Number of lines of the block laid out by the last layout() call, and the
// vertical offset of one of them inside the block.
int lineCount();
int lineY(int line);
// Draws that block at (x, y) starting from line firstLine. Lines whose bottom
// would exceed maxY are not drawn. Returns the number of lines drawn.
int draw(int x, int y, int maxY, EADK::Color bg, int firstLine);
// Title of the note: first heading, or nullptr.
const char* title(int* len);

// Heading level (1-4) of block i, 0 when it is not a heading.
int headingLevel(int i);
const char* blockText(int i, int* len);
const char* source(int* len);
// Block containing (or following) a source offset, and, for the block laid
// out by the last layout() call, the line containing that offset.
int blockAtOffset(int off);
int lineForOffset(int off);
// Source range drawn with a highlight (search match); nullptr for none.
void setHighlight(const char* p, int len);

}  // namespace Doc
