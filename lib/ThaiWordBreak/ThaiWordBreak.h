#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Dictionary-based Thai word segmentation for line breaking. Thai writes words
// without spaces, so the line breaker needs explicit break opportunities.
namespace thai {

// Byte offsets in `text` where a line may break between two Thai words, in
// ascending order. Empty when the text has no Thai run with more than one word.
// Never breaks inside a character cluster (before a mark, after a leading vowel).
std::vector<size_t> wordBreakByteOffsets(const std::string& text);

// True when the UTF-8 string is one dictionary word. Exposed for tests.
bool isDictionaryWord(const std::string& word);

}  // namespace thai
