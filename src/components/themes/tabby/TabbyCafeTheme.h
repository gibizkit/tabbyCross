#pragma once

#include "components/themes/lyra/LyraTheme.h"

class GfxRenderer;

// Lyra screens with a cat-café home: the menu items are objects in one
// full-screen picture, and the selected object is outlined and labelled.
namespace TabbyCafeMetrics {
constexpr int sceneWidth = 480;
constexpr int sceneHeight = 800;

constexpr ThemeMetrics values = [] {
  ThemeMetrics v = LyraMetrics::values;
  v.headerClockCentered = false;  // keep the clock off the roof peak
  v.homeCoverTileHeight = 0;      // no cover tile; the scene fills the screen
  v.homeRecentBooksCount = 1;
  v.homeContinueReadingInMenu = true;
  return v;
}();
}  // namespace TabbyCafeMetrics

class TabbyCafeTheme : public LyraTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle = nullptr,
                  bool backButton = true) const override;
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           const int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
  bool homeMenuHotspot(const GfxRenderer& renderer, UIIcon icon, Rect& hotspot) const override;
};
