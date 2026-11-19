#include "notes.h"

#include <stdint.h>
#include <string.h>

#include "eadkpp.h"

#ifdef NOTES_WITH_DATA
extern "C" {
extern const char* eadk_external_data;
extern size_t eadk_external_data_size;
}
#endif

namespace Notes {

namespace {

struct Entry { const char* name; int nameLen; const char* text; int len; uint8_t source; };
Entry g_entries[kMax];
int g_count;

const char kSample[] =
#include "sample_note.inc"
;

void add(const char* name, int nameLen, const char* text, int len, uint8_t source) {
  if (g_count >= kMax || len <= 0) return;
  // Strip a surrounding Python docstring if present
  int i = 0;
  while (i < len && (text[i] == ' ' || text[i] == '\n' || text[i] == '\r')) i++;
  if (len - i >= 3 && (strncmp(text + i, "\"\"\"", 3) == 0 || strncmp(text + i, "'''", 3) == 0)) {
    char q = text[i];
    int s = i + 3;
    int e = len;
    for (int k = len - 3; k >= s; k--) if (text[k] == q && text[k + 1] == q && text[k + 2] == q) { e = k; break; }
    if (s < len && text[s] == '\n') s++;
    text += s; len = e - s;
  }
  Entry& en = g_entries[g_count++];
  en.name = name; en.nameLen = nameLen; en.text = text; en.len = len; en.source = source;
}

#ifdef NOTES_WITH_DATA
void splitExternal(const char* data, int len, uint8_t source) {
  // Notes separated by lines starting with "=== "; otherwise one note.
  int i = 0;
  const char* curName = "Notes"; int curNameLen = 5; int curStart = 0;
  bool any = false;
  while (i <= len) {
    int e = i; while (e < len && data[e] != '\n') e++;
    if (e - i >= 4 && strncmp(data + i, "=== ", 4) == 0) {
      if (any || i > curStart) add(curName, curNameLen, data + curStart, i - curStart, source);
      curName = data + i + 4; curNameLen = e - i - 4;
      if (curNameLen > 0 && curName[curNameLen - 1] == '\r') curNameLen--;
      curStart = e + 1; any = true;
    }
    i = e + 1;
  }
  if (len > curStart) add(curName, curNameLen, data + curStart, len - curStart, source);
}
#endif

#if PLATFORM_DEVICE
constexpr uint32_t kStorageMagic = 0xEE0BDDBA;
constexpr int kStorageSize = 42 * 1024;

uint16_t rd16(const char* p) { return (uint16_t)((uint8_t)p[0] | ((uint8_t)p[1] << 8)); }

bool validStorage(const char* base, const char* limit) {
  const char* p = base + 4;
  int records = 0;
  while (p + 2 <= limit && p < base + kStorageSize) {
    uint16_t size = rd16(p);
    if (size == 0) return records >= 0;
    if (size < 4 || p + size > base + kStorageSize || p + size > limit) return false;
    const char* nm = p + 2;
    int nl = 0;
    bool dot = false;
    while (nm + nl < p + size && nm[nl] != 0) {
      char c = nm[nl];
      if (c < 0x20 || c > 0x7E) return false;
      if (c == '.') dot = true;
      nl++;
      if (nl > 64) return false;
    }
    if (!dot || nm + nl >= p + size) return false;
    p += size;
    if (++records > 300) return false;
  }
  return false;
}

const char* findStorage() {
  struct Range { uintptr_t start, end; };
  const Range ranges[] = {{0x24000000u, 0x24000000u + 0x40000u - 0x1000u},   // N0120 AXI SRAM (kernel area excluded)
                          {0x20000000u, 0x20000000u + 0x20000u}};            // DTCM / N0110 SRAM
  for (unsigned r = 0; r < 2; r++) {
    const char* p = (const char*)ranges[r].start;
    const char* end = (const char*)ranges[r].end;
    for (; p + 4 <= end; p++) {
      if ((uint8_t)p[0] == 0xBA && (uint8_t)p[1] == 0xDD && (uint8_t)p[2] == 0x0B && (uint8_t)p[3] == 0xEE) {
        if (validStorage(p, end)) return p;
      }
    }
  }
  return nullptr;
}

bool isNoteName(const char* nm, int nl) {
  // "note*.py" or "*_md.py" / "*.md.py", case-insensitive prefix
  if (nl < 4 || strncmp(nm + nl - 3, ".py", 3) != 0) return false;
  int base = nl - 3;
  if (base >= 4) {
    char a = nm[0] | 0x20, b = nm[1] | 0x20, c = nm[2] | 0x20, d = nm[3] | 0x20;
    if (a == 'n' && b == 'o' && c == 't' && d == 'e') return true;
  }
  if (base >= 3 && strncmp(nm + base - 3, "_md", 3) == 0) return true;
  return false;
}

void scanStorage() {
  const char* base = findStorage();
  if (!base) return;
  const char* p = base + 4;
  while (p + 2 < base + kStorageSize) {
    uint16_t size = rd16(p);
    if (size == 0) break;
    const char* nm = p + 2;
    int nl = strlen(nm);
    const char* body = nm + nl + 1;
    int bodyLen = size - 2 - nl - 1;
    if (isNoteName(nm, nl) && bodyLen > 1) {
      const char* txt = body + 1;  // first byte: script status flags
      int tl = bodyLen - 1;
      while (tl > 0 && txt[tl - 1] == 0) tl--;
      int baseLen = nl - 3;
      const char* shown = nm; int shownLen = baseLen;
      if (shownLen > 4 && (nm[0] | 0x20) == 'n' && (nm[1] | 0x20) == 'o') { shown += 4; shownLen -= 4; if (shownLen > 0 && shown[0] == '_') { shown++; shownLen--; } }
      if (shownLen <= 0) { shown = nm; shownLen = baseLen; }
      add(shown, shownLen, txt, tl, 1);
    }
    p += size;
  }
}
#endif

}  // namespace

void scan() {
  g_count = 0;
#if PLATFORM_DEVICE
  scanStorage();
#endif
#ifdef NOTES_WITH_DATA
  if (eadk_external_data != nullptr && eadk_external_data_size > 0) {
    splitExternal(eadk_external_data, (int)eadk_external_data_size, 2);
  }
#endif
  if (g_count == 0) add("Guide", 5, kSample, sizeof(kSample) - 1, 3);
}

int count() { return g_count; }
const char* name(int i, int* len) { *len = g_entries[i].nameLen; return g_entries[i].name; }
const char* text(int i, int* len) { *len = g_entries[i].len; return g_entries[i].text; }
const char* sourceLabel(int i) {
  switch (g_entries[i].source) { case 1: return "Workshop"; case 2: return "data file"; default: return "built-in"; }
}

}  // namespace Notes
