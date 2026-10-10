#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ThaiWordBreak.h"
#include "Utf8.h"

// ============================================================================
// Thai word segmentation for line breaking. Input is normalized the same way
// ParsedText::addWord does (sara am split) before segmentation.
// ============================================================================

namespace {

std::vector<std::string> segment(const std::string& text) {
  const std::string normalized = utf8SplitThaiSaraAm(text);
  std::vector<std::string> words;
  size_t start = 0;
  for (const size_t offset : thai::wordBreakByteOffsets(normalized)) {
    words.push_back(normalized.substr(start, offset - start));
    start = offset;
  }
  words.push_back(normalized.substr(start));
  return words;
}

std::vector<std::string> normalized(std::initializer_list<const char*> words) {
  std::vector<std::string> out;
  for (const char* w : words) out.push_back(utf8SplitThaiSaraAm(w));
  return out;
}

}  // namespace

TEST(ThaiWordBreak, NoBreaksWithoutThai) {
  EXPECT_TRUE(thai::wordBreakByteOffsets("").empty());
  EXPECT_TRUE(thai::wordBreakByteOffsets("hello").empty());
  EXPECT_TRUE(thai::wordBreakByteOffsets("\xE4\xB8\xAD\xE6\x96\x87").empty());  // 中文
}

TEST(ThaiWordBreak, SingleWordHasNoBreak) {
  EXPECT_TRUE(thai::wordBreakByteOffsets("บ้าน").empty());
}

TEST(ThaiWordBreak, SplitsSentenceIntoDictionaryWords) {
  EXPECT_EQ(segment("ฉันกินข้าวที่บ้าน"), normalized({"ฉัน", "กิน", "ข้าว", "ที่", "บ้าน"}));
  EXPECT_EQ(segment("สมชายไปโรงเรียนกับมานีและปิติ"),
            normalized({"สมชาย", "ไป", "โรงเรียน", "กับ", "มานี", "และ", "ปิติ"}));
}

TEST(ThaiWordBreak, HandlesSaraAmAfterNormalization) {
  EXPECT_EQ(segment("น้ำตกสวยงามมาก"), normalized({"น้ำตก", "สวยงาม", "มาก"}));
}

TEST(ThaiWordBreak, KeepsRepetitionAndAbbreviationMarksAttached) {
  EXPECT_EQ(segment("เด็กๆเล่นกัน"), normalized({"เด็กๆ", "เล่น", "กัน"}));
  EXPECT_EQ(segment("กรุงเทพฯเป็นเมืองหลวง"), normalized({"กรุงเทพฯ", "เป็น", "เมือง", "หลวง"}));
}

TEST(ThaiWordBreak, NeverSplitsCharacterClusters) {
  const std::string text = utf8SplitThaiSaraAm("ภาษาไทยเป็นภาษาที่ไม่มีการเว้นวรรคระหว่างคำเลยแม้แต่น้อย");
  for (const size_t offset : thai::wordBreakByteOffsets(text)) {
    const auto* after = reinterpret_cast<const unsigned char*>(text.c_str() + offset);
    const uint32_t next = utf8NextCodepoint(&after);
    EXPECT_FALSE(utf8IsCombiningMark(next)) << "break before a mark at " << offset;
    EXPECT_NE(next, 0x0E32u) << "break before sara aa at " << offset;
    // Previous codepoint: Thai is 3 bytes per codepoint.
    const auto* before = reinterpret_cast<const unsigned char*>(text.c_str() + offset - 3);
    const uint32_t prev = utf8NextCodepoint(&before);
    EXPECT_FALSE(prev >= 0x0E40 && prev <= 0x0E44) << "break after a leading vowel at " << offset;
  }
}

TEST(ThaiWordBreak, KeepsUnknownWordsWhole) {
  // "ซอฟต์แวร์" may or may not be listed; an unknown run must not split per cluster.
  const auto words = segment("โปรแกรมคริสพอยต์ดี");
  for (const auto& w : words) EXPECT_FALSE(w.empty());
  EXPECT_LE(words.size(), 4u);
}

TEST(ThaiWordBreak, BreaksOnlyInsideThaiRuns) {
  // Latin text glued to Thai is not split from it.
  const std::string text = "EPUBภาษาไทย";
  for (const size_t offset : thai::wordBreakByteOffsets(text)) EXPECT_GT(offset, 4u);
}

TEST(ThaiWordBreak, LongRunsAreChunkedSafely) {
  std::string text;
  for (int i = 0; i < 200; ++i) text += "ภาษาไทย";
  const auto offsets = thai::wordBreakByteOffsets(text);
  ASSERT_FALSE(offsets.empty());
  for (size_t i = 1; i < offsets.size(); ++i) EXPECT_LT(offsets[i - 1], offsets[i]);
  EXPECT_LT(offsets.back(), text.size());
}

TEST(ThaiDictionary, LooksUpNormalizedWords) {
  EXPECT_TRUE(thai::isDictionaryWord("บ้าน"));
  EXPECT_TRUE(thai::isDictionaryWord(utf8SplitThaiSaraAm("น้ำ")));
  EXPECT_FALSE(thai::isDictionaryWord("บ้"));
  EXPECT_FALSE(thai::isDictionaryWord("abc"));
  EXPECT_FALSE(thai::isDictionaryWord(""));
}
