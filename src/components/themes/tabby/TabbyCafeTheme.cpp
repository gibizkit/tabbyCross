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

struct Box {
  int x, y, w, h;
};

// Rects in scene pixels, measured on the 480x800 picture. A zero-width label
// means the picture has none, so the theme draws one.
struct SceneItem {
  UIIcon icon;
  Box object;
  Box label;
};

constexpr SceneItem sceneItems[] = {
    {Book, {262, 586, 138, 150}, {262, 531, 105, 61}},     // table: Continue Reading
    {Folder, {22, 326, 92, 98}, {82, 369, 141, 38}},       // counter drawers: Browse Files
    {Library, {34, 486, 82, 154}, {107, 557, 91, 40}},     // bookshelf
    {Plugins, {76, 70, 88, 108}, {0, 0, 0, 0}},            // MENU board: OPDS / Plugins
    {Transfer, {330, 116, 96, 124}, {310, 200, 147, 41}},  // window
    {Settings, {86, 180, 108, 126}, {176, 186, 103, 40}},  // coffee machine and grinder
};

const SceneItem* findItem(const UIIcon icon) {
  for (const auto& item : sceneItems) {
    if (item.icon == icon) return &item;
  }
  return nullptr;
}

// Scene origin: centered, so panels near 480x800 still line up with the rects.
int sceneX(const GfxRenderer& renderer) { return (renderer.getScreenWidth() - TabbyCafeMetrics::sceneWidth) / 2; }
int sceneY(const GfxRenderer& renderer) { return (renderer.getScreenHeight() - TabbyCafeMetrics::sceneHeight) / 2; }

Rect toScreen(const GfxRenderer& renderer, const Box& box) {
  return Rect{sceneX(renderer) + box.x, sceneY(renderer) + box.y, box.w, box.h};
}

bool sceneBitIsWhite(const int x, const int y) {
  constexpr int bytesPerRow = TabbyCafeMetrics::sceneWidth / 8;
  return TabbyCafeScene[y * bytesPerRow + x / 8] & (0x80 >> (x % 8));
}

// Redraws a scene region with black and white swapped (selected label).
void drawInvertedRegion(const GfxRenderer& renderer, const Box& box) {
  const int originX = sceneX(renderer);
  const int originY = sceneY(renderer);
  for (int y = box.y; y < box.y + box.h; ++y) {
    for (int x = box.x; x < box.x + box.w; ++x) {
      renderer.drawPixel(originX + x, originY + y, sceneBitIsWhite(x, y));
    }
  }
}

void drawLabelBubble(const GfxRenderer& renderer, const Rect& spot, const char* label) {
  // Below the object, or above it when it would cover the button hints.
  const int textWidth = renderer.getTextWidth(UI_10_FONT_ID, label);
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
  renderer.drawText(UI_10_FONT_ID, bubbleX + labelPadding, bubbleY + labelPadding / 2, label, false);
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
  const SceneItem* item = findItem(rowIcon(selectedIndex));
  if (item == nullptr) return;

  // Black outline with a white edge on both sides so it reads on any background.
  const Rect spot = toScreen(renderer, item->object);
  renderer.drawRect(spot.x - 1, spot.y - 1, spot.width + 2, spot.height + 2, 1, false);
  renderer.drawRect(spot.x, spot.y, spot.width, spot.height, outlineWidth, true);
  renderer.drawRect(spot.x + outlineWidth, spot.y + outlineWidth, spot.width - 2 * outlineWidth,
                    spot.height - 2 * outlineWidth, 1, false);

  // The picture's own label turns white; items without one get a drawn bubble.
  if (item->label.w > 0) {
    drawInvertedRegion(renderer, item->label);
    const Rect label = toScreen(renderer, item->label);
    renderer.drawRect(label.x, label.y, label.width, label.height, 2, true);
  } else {
    drawLabelBubble(renderer, spot, buttonLabel(selectedIndex).c_str());
  }
}

bool TabbyCafeTheme::homeMenuHotspot(const GfxRenderer& renderer, const UIIcon icon, Rect& hotspot) const {
  const SceneItem* item = findItem(icon);
  if (item == nullptr) return false;
  // Touch target: the object together with its label.
  Box area = item->object;
  if (item->label.w > 0) {
    const int right = std::max(area.x + area.w, item->label.x + item->label.w);
    const int bottom = std::max(area.y + area.h, item->label.y + item->label.h);
    area.x = std::min(area.x, item->label.x);
    area.y = std::min(area.y, item->label.y);
    area.w = right - area.x;
    area.h = bottom - area.y;
  }
  hotspot = toScreen(renderer, area);
  return true;
}
