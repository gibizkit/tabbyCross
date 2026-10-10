#include "ThaiWordBreak.h"

#include <Utf8.h>

#include <algorithm>
#include <limits>

#include "ThaiDictData.h"

namespace thai {
namespace {

// Longer runs are segmented in independent chunks to bound the per-call buffers.
constexpr size_t MAX_RUN_CODEPOINTS = 512;
// Minimize unknown clusters first, then the number of words (maximal matching).
constexpr uint32_t WORD_COST = 1;
constexpr uint32_t UNKNOWN_COST = 1000;
// Dictionary words this short next to unknown text are treated as part of it.
constexpr uint16_t MAX_FOLDED_WORD_CODEPOINTS = 2;
constexpr uint32_t NO_PATH = std::numeric_limits<uint32_t>::max();

bool isThai(const uint32_t cp) { return cp >= 0x0E01 && cp <= 0x0E5B; }

// Thai letters map to one byte, matching the dictionary encoding.
uint8_t toKey(const uint32_t cp) { return static_cast<uint8_t>(cp - 0x0E00); }

uint32_t wordStart(const uint32_t index) {
  return dict::BLOCK_BASE[index >> dict::BLOCK_SHIFT] + dict::REL_OFFSET[index];
}

// Byte `pos` of dictionary word `index`, or -1 past its end (shorter words sort first).
int wordByteAt(const uint32_t index, const size_t pos) {
  const uint32_t start = wordStart(index);
  return pos < wordStart(index + 1) - start ? dict::BLOB[start + pos] : -1;
}

size_t wordLength(const uint32_t index) { return wordStart(index + 1) - wordStart(index); }

// Narrow [lo, hi), whose words all share key[0..pos), to the words whose byte `pos` is `value`.
void narrow(uint32_t& lo, uint32_t& hi, const size_t pos, const int value) {
  uint32_t first = lo, last = hi;
  while (first < last) {
    const uint32_t mid = first + (last - first) / 2;
    if (wordByteAt(mid, pos) < value) first = mid + 1;
    else last = mid;
  }
  uint32_t end = first;
  last = hi;
  while (end < last) {
    const uint32_t mid = end + (last - end) / 2;
    if (wordByteAt(mid, pos) <= value) end = mid + 1;
    else last = mid;
  }
  lo = first;
  hi = end;
}

// A break between two Thai codepoints must not split a character cluster.
bool canBreakBetween(const uint32_t prev, const uint32_t next) {
  if (utf8IsCombiningMark(next)) return false;
  if (prev >= 0x0E40 && prev <= 0x0E44) return false;  // leading vowels เ แ โ ใ ไ
  switch (next) {
    case 0x0E2F:  // paiyannoi ฯ
    case 0x0E30:  // sara a ะ
    case 0x0E32:  // sara aa า
    case 0x0E33:  // sara am ำ
    case 0x0E45:  // lakkhangyao ๅ
    case 0x0E46:  // mai yamok ๆ
      return false;
    default:
      return true;
  }
}

struct Codepoint {
  uint32_t cp;
  size_t endOffset;  // byte offset just past this codepoint
};

// Appends the chosen word boundaries inside cps[begin, end) to `out`.
void segmentRun(const std::vector<Codepoint>& cps, const size_t begin, const size_t end, std::vector<size_t>& out) {
  const size_t n = end - begin;
  if (n < 2) return;

  // Heap: three arrays of n + 1 entries, n <= MAX_RUN_CODEPOINTS, freed on return.
  std::vector<uint32_t> cost(n + 1, NO_PATH);
  std::vector<uint16_t> from(n + 1, 0);
  std::vector<uint8_t> isUnknownStep(n + 1, 0);
  const auto canBreakAt = [&](const size_t k) {
    return k == 0 || k == n || canBreakBetween(cps[begin + k - 1].cp, cps[begin + k].cp);
  };
  const auto relax = [&](const size_t start, const size_t stop, const uint32_t stepCost, const bool unknown) {
    const uint32_t candidate = cost[start] + stepCost;
    if (candidate < cost[stop]) {
      cost[stop] = candidate;
      from[stop] = static_cast<uint16_t>(start);
      isUnknownStep[stop] = unknown ? 1 : 0;
    }
  };

  cost[0] = 0;
  for (size_t i = 0; i < n; ++i) {
    if (cost[i] == NO_PATH || !canBreakAt(i)) continue;

    uint32_t lo = 0, hi = dict::WORD_COUNT;
    for (size_t len = 1; len <= dict::MAX_WORD_LEN && i + len <= n; ++len) {
      narrow(lo, hi, len - 1, toKey(cps[begin + i + len - 1].cp));
      if (lo == hi) break;
      if (wordLength(lo) == len && canBreakAt(i + len)) relax(i, i + len, WORD_COST, false);
    }

    size_t next = i + 1;
    while (!canBreakAt(next)) ++next;
    relax(i, next, UNKNOWN_COST, true);
  }

  // Walk back into segments (in reverse order).
  struct Segment {
    uint16_t start;
    uint16_t length;
    bool unknown;
  };
  std::vector<Segment> segments;
  for (size_t k = n; k > 0; k = from[k]) {
    segments.push_back({from[k], static_cast<uint16_t>(k - from[k]), isUnknownStep[k] != 0});
  }
  // A name missing from the dictionary often matches short words by accident
  // ("คริสพอยต์" -> ค|ริ|ส|พอ|ยต์). Fold short words next to unknown text into it.
  for (size_t pass = 0; pass < 2; ++pass) {
    for (size_t j = 0; j < segments.size(); ++j) {
      if (segments[j].unknown || segments[j].length > MAX_FOLDED_WORD_CODEPOINTS) continue;
      const bool unknownBefore = j + 1 < segments.size() && segments[j + 1].unknown;
      const bool unknownAfter = j > 0 && segments[j - 1].unknown;
      if (unknownBefore || unknownAfter) segments[j].unknown = true;
    }
  }
  // Emit boundaries, keeping adjacent unknown segments together.
  for (size_t j = 0; j + 1 < segments.size(); ++j) {
    if (segments[j].unknown && segments[j + 1].unknown) continue;
    out.push_back(cps[begin + segments[j].start - 1].endOffset);
  }
}

}  // namespace

std::vector<size_t> wordBreakByteOffsets(const std::string& text) {
  // Fast path: every Thai codepoint is encoded as E0 B8 xx or E0 B9 xx.
  bool hasThai = false;
  for (size_t i = 0; i + 1 < text.size(); ++i) {
    if (text[i] == '\xE0' && (text[i + 1] == '\xB8' || text[i + 1] == '\xB9')) {
      hasThai = true;
      break;
    }
  }
  if (!hasThai) return {};

  std::vector<Codepoint> cps;
  cps.reserve(text.size() / 3 + 1);
  const auto* const start = reinterpret_cast<const unsigned char*>(text.c_str());
  const auto* ptr = start;
  while (*ptr) {
    const uint32_t cp = utf8NextCodepoint(&ptr);
    if (cp == 0) break;
    cps.push_back({cp, static_cast<size_t>(ptr - start)});
  }

  std::vector<size_t> offsets;
  size_t i = 0;
  while (i < cps.size()) {
    if (!isThai(cps[i].cp)) {
      ++i;
      continue;
    }
    size_t runEnd = i;
    while (runEnd < cps.size() && isThai(cps[runEnd].cp)) ++runEnd;
    for (size_t chunk = i; chunk < runEnd;) {
      size_t chunkEnd = std::min(runEnd, chunk + MAX_RUN_CODEPOINTS);
      while (chunkEnd < runEnd && chunkEnd > chunk + 1 && !canBreakBetween(cps[chunkEnd - 1].cp, cps[chunkEnd].cp)) {
        --chunkEnd;
      }
      const size_t firstNew = offsets.size();
      segmentRun(cps, chunk, chunkEnd, offsets);
      // segmentRun appends back to front.
      std::reverse(offsets.begin() + static_cast<std::ptrdiff_t>(firstNew), offsets.end());
      if (chunkEnd < runEnd) offsets.push_back(cps[chunkEnd - 1].endOffset);
      chunk = chunkEnd;
    }
    i = runEnd;
  }
  return offsets;
}

bool isDictionaryWord(const std::string& word) {
  uint32_t lo = 0, hi = dict::WORD_COUNT;
  size_t len = 0;
  const auto* ptr = reinterpret_cast<const unsigned char*>(word.c_str());
  while (*ptr) {
    const uint32_t cp = utf8NextCodepoint(&ptr);
    if (!isThai(cp)) return false;
    narrow(lo, hi, len++, toKey(cp));
    if (lo == hi) return false;
  }
  return len > 0 && wordLength(lo) == len;
}

}  // namespace thai
