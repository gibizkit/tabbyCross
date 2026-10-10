#pragma once

#include "components/themes/lyra/Lyra3CoversTheme.h"

class GfxRenderer;

// Lyra 3 Covers laid out as a bookshelf: a labelled, framed "Continue reading"
// row of three covers above a labelled grid of menu buttons.
namespace ShelfMetrics {
constexpr int sectionLabelHeight = 32;
constexpr int coverFramePadding = 6;

constexpr ThemeMetrics values = [] {
  ThemeMetrics v = Lyra3CoversMetrics::values;
  v.homeTopPadding = 64;
  v.headerClockCentered = true;
  v.homeCoverTileHeight = Lyra3CoversMetrics::values.homeCoverTileHeight + sectionLabelHeight + 2 * coverFramePadding;
  v.homeMenuTopOffset = sectionLabelHeight + 8;
  v.menuRowHeight = 60;
  v.menuSpacing = 10;
  return v;
}();
}  // namespace ShelfMetrics

class ShelfTheme : public Lyra3CoversTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle = nullptr,
                  bool backButton = true) const override;
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           const int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  int getMenuRowHeight(const GfxRenderer&) const override { return ShelfMetrics::values.menuRowHeight; }
  int homeMenuColumns() const override { return 3; }
};
