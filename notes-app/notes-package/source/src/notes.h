#pragma once
// Where notes come from, in priority order:
//  1. external data attached with nwlink (notes separated by "=== Title" lines)
//  2. Python scripts in the calculator's storage whose name starts with "note"
//     (written in the NumWorks Workshop, synced like any script)
//  3. a built-in sample that doubles as the manual
namespace Notes {

constexpr int kMax = 32;

void scan();
int count();
const char* name(int i, int* len);         // file name without prefix/extension
const char* text(int i, int* len);
const char* sourceLabel(int i);            // "Workshop", "nwlink", "built-in"

}  // namespace Notes
