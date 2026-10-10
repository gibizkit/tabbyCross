#include <gtest/gtest.h>

#include "lib/EpdFont/EpdFontData.h"

// ============================================================================
// Anchor selection and placement for combining marks without GPOS tables.
//
// Hebrew niqqud whose identity depends on position must not use the default
// centre-and-raise heuristic: dagesh sits inside the letter body, the
// shin/sin dots sit over the letter's right/left arm, and holam hangs over
// the left corner (see PR #2541 review feedback).
// ============================================================================

using combiningMark::Anchor;
using combiningMark::anchorFor;
using combiningMark::anchorOver;
using combiningMark::anchorOverRotated90CW;
using combiningMark::raiseAboveBase;

TEST(AnchorFor, PositionSensitiveNiqqud) {
  EXPECT_EQ(anchorFor(0x05BC), Anchor::CenterNative);  // dagesh/mapiq
  EXPECT_EQ(anchorFor(0x05BA), Anchor::CenterNative);  // holam haser for vav
  EXPECT_EQ(anchorFor(0x05C1), Anchor::RightNative);   // shin dot
  EXPECT_EQ(anchorFor(0x05C2), Anchor::LeftNative);    // sin dot
  EXPECT_EQ(anchorFor(0x05B9), Anchor::LeftNative);    // holam
}

TEST(AnchorFor, EverythingElseKeepsCentreRaisedDefault) {
  EXPECT_EQ(anchorFor(0x05B0), Anchor::CenterRaised);  // Hebrew sheva
  EXPECT_EQ(anchorFor(0x05B7), Anchor::CenterRaised);  // Hebrew patach
  EXPECT_EQ(anchorFor(0x064E), Anchor::CenterRaised);  // Arabic fatha
  EXPECT_EQ(anchorFor(0x0651), Anchor::CenterRaised);  // Arabic shadda
  EXPECT_EQ(anchorFor(0x0301), Anchor::CenterRaised);  // combining acute
}

// Base glyph: cursor 100, left 1, width 12.  Mark: left 2, width 4.
TEST(AnchorOver, HorizontalPlacementPerAnchor) {
  // Centered: base bitmap spans [101, 113), centre 107; mark starts at 105.
  EXPECT_EQ(anchorOver(Anchor::CenterRaised, 100, 1, 12, 2, 4), 105 - 2);
  EXPECT_EQ(anchorOver(Anchor::CenterNative, 100, 1, 12, 2, 4), 105 - 2);
  // Left-aligned: mark bitmap starts at the base bitmap's left edge (101).
  EXPECT_EQ(anchorOver(Anchor::LeftNative, 100, 1, 12, 2, 4), 101 - 2);
  // Right-aligned: mark bitmap ends at the base bitmap's right edge (113).
  EXPECT_EQ(anchorOver(Anchor::RightNative, 100, 1, 12, 2, 4), 109 - 2);
}

// The rotated coordinate system inverts every left/width term, so the mark's
// offset from the base cursor must be the exact mirror of the unrotated one.
TEST(AnchorOverRotated90CW, MirrorsUnrotatedOffsets) {
  for (const Anchor anchor : {Anchor::CenterRaised, Anchor::CenterNative, Anchor::LeftNative, Anchor::RightNative}) {
    const int offset = anchorOver(anchor, 100, 1, 12, 2, 4) - 100;
    EXPECT_EQ(anchorOverRotated90CW(anchor, 100, 1, 12, 2, 4), 100 - offset);
  }
}

TEST(RaiseAboveBase, NativeAnchorsKeepFontDesignedHeight) {
  // A dagesh-like dot designed inside the letter body (top 8 of a base whose
  // top is 12) must NOT be hoisted above the letter.
  EXPECT_EQ(raiseAboveBase(Anchor::CenterNative, 8, 3, 12), 0);
  // Shin/sin dots overlapping the letter's top must not be pushed clear.
  EXPECT_EQ(raiseAboveBase(Anchor::RightNative, 13, 3, 12), 0);
  EXPECT_EQ(raiseAboveBase(Anchor::LeftNative, 13, 3, 12), 0);
}

TEST(RaiseAboveBase, CentreRaisedBehaviourUnchanged) {
  // Above-baseline mark colliding with the base: raised to restore a 1px gap.
  // gap = markTop - markHeight - baseTop = 10 - 3 - 12 = -5  ->  raise 6.
  EXPECT_EQ(raiseAboveBase(Anchor::CenterRaised, 10, 3, 12), 6);
  // Already clear of the base: no raise.
  EXPECT_EQ(raiseAboveBase(Anchor::CenterRaised, 16, 3, 12), 0);
  // Below-baseline mark (kasra, cedilla): stays at font-native position.
  EXPECT_EQ(raiseAboveBase(Anchor::CenterRaised, 2, 4, 12), 0);
}

// ============================================================================
// Thai marks: right-aligned on the consonant, moved off the stem of tall
// consonants, and a tone mark stacked above an upper vowel on the same base.
// ============================================================================

using combiningMark::raiseFor;
using combiningMark::stackTopAfter;

TEST(AnchorFor, ThaiMarks) {
  EXPECT_EQ(anchorFor(0x0E31), Anchor::ThaiAbove);     // mai han-akat
  EXPECT_EQ(anchorFor(0x0E34), Anchor::ThaiAbove);     // sara i
  EXPECT_EQ(anchorFor(0x0E37), Anchor::ThaiAbove);     // sara uee
  EXPECT_EQ(anchorFor(0x0E48), Anchor::ThaiAbove);     // mai ek
  EXPECT_EQ(anchorFor(0x0E4C), Anchor::ThaiAbove);     // thanthakhat
  EXPECT_EQ(anchorFor(0x0E4D), Anchor::ThaiAbove);     // nikhahit
  EXPECT_EQ(anchorFor(0x0E38), Anchor::ThaiBelow);     // sara u
  EXPECT_EQ(anchorFor(0x0E3A), Anchor::ThaiBelow);     // phinthu
  EXPECT_EQ(anchorFor(0x0E01), Anchor::CenterRaised);  // ko kai is not a mark
}

// Base glyph: cursor 100, left 1, width 12 -> bitmap [101, 113).  Mark: left -6, width 4.
TEST(AnchorOver, ThaiMarksAlignToConsonantRightEdge) {
  EXPECT_EQ(anchorOver(Anchor::ThaiAbove, 100, 1, 12, -6, 4, 0x0E01), 109 + 6);  // ก
  EXPECT_EQ(anchorOver(Anchor::ThaiBelow, 100, 1, 12, -6, 4, 0x0E1B), 109 + 6);  // below mark on ป: no shift
}

TEST(AnchorOver, ThaiAboveMarkAvoidsTallConsonantStem) {
  // ป: the mark moves left by a quarter of the base width (12 / 4 = 3).
  EXPECT_EQ(anchorOver(Anchor::ThaiAbove, 100, 1, 12, -6, 4, 0x0E1B), 106 + 6);
  EXPECT_EQ(anchorOverRotated90CW(Anchor::ThaiAbove, 100, 1, 12, -6, 4, 0x0E1B), 100 - (106 + 6 - 100));
}

TEST(RaiseFor, ThaiToneStacksAboveUpperVowel) {
  // First above mark on the base keeps its font-native height.
  EXPECT_EQ(raiseFor(Anchor::ThaiAbove, 15, 3, 10, 0), 0);
  // Tone (top 16, height 3 -> bottom 13) after a vowel whose top is 15:
  // gap = 13 - 15 = -2 -> raise 3 for a 1px gap.
  EXPECT_EQ(raiseFor(Anchor::ThaiAbove, 16, 3, 10, 15), 3);
  // Tone already designed above the vowel: no raise.
  EXPECT_EQ(raiseFor(Anchor::ThaiAbove, 20, 3, 10, 15), 0);
  // Below marks never move.
  EXPECT_EQ(raiseFor(Anchor::ThaiBelow, -1, 3, 10, 15), 0);
}

TEST(RaiseFor, NonThaiAnchorsMatchRaiseAboveBase) {
  EXPECT_EQ(raiseFor(Anchor::CenterRaised, 10, 3, 12, 99), raiseAboveBase(Anchor::CenterRaised, 10, 3, 12));
  EXPECT_EQ(raiseFor(Anchor::CenterNative, 8, 3, 12, 99), 0);
}

TEST(StackTopAfter, OnlyThaiAboveMarksGrowTheStack) {
  EXPECT_EQ(stackTopAfter(Anchor::ThaiAbove, 16, 3, 15), 19);
  EXPECT_EQ(stackTopAfter(Anchor::ThaiBelow, -1, 0, 15), 15);
  EXPECT_EQ(stackTopAfter(Anchor::CenterRaised, 16, 3, 0), 0);
}
