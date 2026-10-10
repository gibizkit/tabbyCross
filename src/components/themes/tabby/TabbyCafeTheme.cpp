#include "TabbyCafeTheme.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <string>

#include "TabbyCafeScene.h"
#include "fontIds.h"

namespace {
constexpr int outlineWidth = 3;
constexpr int labelPadding = 8;
constexpr int labelGap = 6;
constexpr int labelRadius = 6;
constexpr int screenMargin = 4;

// Scene origin: centered, so panels near 480x800 still line up with the hotspots.
int sceneX(const GfxRenderer& renderer) { return (renderer.getScreenWidth() - TabbyCafeMetrics::sceneWidth) / 2; }
int sceneY(const GfxRenderer& renderer) { return (renderer.getScreenHeight() - TabbyCafeMetrics::sceneHeight) / 2; }

// Object rects in scene pixels, measured on the 480x800 picture.
bool sceneHotspot(const UIIcon icon, Rect& out) {
  switch (icon) {
    case Book:  // table and chair: Continue Reading
      out = Rect{262, 586, 138, 150};
      return true;
    case Folder:  // counter drawers: Browse Files
      out = Rect{22, 326, 92, 98};
      return true;
    case Library:  // bookshelf
      out = Rect{34, 486, 82, 154};
      return true;
    case Plugins:  // MENU board: OPDS / Plugins
      out = Rect{76, 70, 88, 108};
      return true;
    case Transfer:  // window
      out = Rect{330, 116, 96, 124};
      return true;
    case Settings:  // coffee machine and grinder
      out = Rect{86, 180, 108, 126};
      return true;
    default:
      return false;
  }
}
}  // namespace

void TabbyCafeTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle,
                                bool backButton) const {
  // Home uses a short band and passes the book title; keep the roof clear and
  // show only the battery and clock there.
  const bool homeBand = rect.height < TabbyCafeMetrics::values.headerHeight;
  LyraTheme::drawHeader(renderer, rect, homeBand ? nullptr : title, subtitle, backButton);
}

void TabbyCafeTheme::drawRecentBookCover(GfxRenderer& renderer, Rect, const std::vector<RecentBook>&, const int,
                                         bool&, bool&, bool&, std::function<bool()>) const {
  // The cover slot is where Home draws its background, so paint the scene here.
  // The screen is already white: plot only the black pixels.
  const int originX = sceneX(renderer);
  const int originY = sceneY(renderer);
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();
  constexpr int bytesPerRow = TabbyCafeMetrics::sceneWidth / 8;
  for (int y = 0; y < TabbyCafeMetrics::sceneHeight; ++y) {
    const int sy = originY + y;
    if (sy < 0 || sy >= screenH) continue;
    const uint8_t* row = TabbyCafeScene + y * bytesPerRow;
    for (int byteIndex = 0; byteIndex < bytesPerRow; ++byteIndex) {
      const uint8_t bits = row[byteIndex];
      if (bits == 0xFF) continue;
      for (int bit = 0; bit < 8; ++bit) {
        if (bits & (0x80 >> bit)) continue;
        const int sx = originX + byteIndex * 8 + bit;
        if (sx >= 0 && sx < screenW) renderer.drawPixel(sx, sy, true);
      }
    }
  }
}

void TabbyCafeTheme::drawButtonMenu(GfxRenderer& renderer, Rect, int buttonCount, int selectedIndex,
                                    const std::function<std::string(int index)>& buttonLabel,
                                    const std::function<UIIcon(int index)>& rowIcon) const {
  if (selectedIndex < 0 || selectedIndex >= buttonCount || !rowIcon) return;
  Rect spot;
  if (!homeMenuHotspot(renderer, rowIcon(selectedIndex), spot)) return;

  // Black outline with a white edge on both sides so it reads on any background.
  renderer.drawRect(spot.x - 1, spot.y - 1, spot.width + 2, spot.height + 2, 1, false);
  renderer.drawRect(spot.x, spot.y, spot.width, spot.height, outlineWidth, true);
  renderer.drawRect(spot.x + outlineWidth, spot.y + outlineWidth, spot.width - 2 * outlineWidth,
                    spot.height - 2 * outlineWidth, 1, false);

  // Label bubble below the object, or above it when it would cover the button hints.
  const std::string label = buttonLabel(selectedIndex);
  const int textWidth = renderer.getTextWidth(UI_10_FONT_ID, label.c_str());
  const int bubbleW = textWidth + 2 * labelPadding;
  const int bubbleH = renderer.getLineHeight(UI_10_FONT_ID) + labelPadding;
  const int screenW = renderer.getScreenWidth();
  const int bottomLimit = renderer.getScreenHeight() - TabbyCafeMetrics::values.buttonHintsHeight - screenMargin;

  int bubbleX = spot.x + (spot.width - bubbleW) / 2;
  bubbleX = std::max(screenMargin, std::min(bubbleX, screenW - screenMargin - bubbleW));
  int bubbleY = spot.y + spot.height + labelGap;
  if (bubbleY + bubbleH > bottomLimit) bubbleY = spot.y - labelGap - bubbleH;

  renderer.fillRoundedRect(bubbleX, bubbleY, bubbleW, bubbleH, labelRadius, Color::Black);
  renderer.drawRoundedRect(bubbleX - 1, bubbleY - 1, bubbleW + 2, bubbleH + 2, 1, labelRadius, false);
  renderer.drawText(UI_10_FONT_ID, bubbleX + labelPadding, bubbleY + labelPadding / 2, label.c_str(), false);
}

bool TabbyCafeTheme::homeMenuHotspot(const GfxRenderer& renderer, const UIIcon icon, Rect& hotspot) const {
  Rect local;
  if (!sceneHotspot(icon, local)) return false;
  hotspot = Rect{sceneX(renderer) + local.x, sceneY(renderer) + local.y, local.width, local.height};
  return true;
}
