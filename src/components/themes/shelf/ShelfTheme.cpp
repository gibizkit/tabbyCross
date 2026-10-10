#include "ShelfTheme.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <string>

#include "fontIds.h"

namespace {
constexpr int cornerRadius = 6;
constexpr int cellIconSize = 24;
constexpr int cellPadding = 10;

void drawSectionLabel(const GfxRenderer& renderer, const int x, const int bottomY, const char* label) {
  const int y = bottomY - renderer.getLineHeight(UI_10_FONT_ID) - 4;
  renderer.drawText(UI_10_FONT_ID, x, y, label, true);
}
}  // namespace

void ShelfTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle,
                            bool backButton) const {
  // Home passes no title; show the firmware name like other readers' home bands.
  LyraTheme::drawHeader(renderer, rect, title ? title : tr(STR_APP_NAME), subtitle, backButton);
}

void ShelfTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                     const int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                     bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  const int side = ShelfMetrics::values.contentSidePadding;
  const int pad = ShelfMetrics::coverFramePadding;
  drawSectionLabel(renderer, rect.x + side, rect.y + ShelfMetrics::sectionLabelHeight, tr(STR_CONTINUE_READING));

  const Rect frame{rect.x + side - pad, rect.y + ShelfMetrics::sectionLabelHeight, rect.width - 2 * (side - pad),
                   rect.height - ShelfMetrics::sectionLabelHeight};
  renderer.drawRoundedRect(frame.x, frame.y, frame.width, frame.height, 1, cornerRadius, true);

  const Rect shelf{rect.x, frame.y + pad, rect.width, frame.height - 2 * pad};
  Lyra3CoversTheme::drawRecentBookCover(renderer, shelf, recentBooks, selectorIndex, coverRendered, coverBufferStored,
                                        bufferRestored, std::move(storeCoverBuffer));
}

void ShelfTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                const std::function<std::string(int index)>& buttonLabel,
                                const std::function<UIIcon(int index)>& rowIcon) const {
  const auto& m = ShelfMetrics::values;
  const int columns = homeMenuColumns();
  const int left = rect.x + m.contentSidePadding;
  const int cellWidth = (rect.width - 2 * m.contentSidePadding - (columns - 1) * m.menuSpacing) / columns;
  const int lineHeight = renderer.getLineHeight(UI_10_FONT_ID);

  drawSectionLabel(renderer, left, rect.y - 4, tr(STR_HOME_TOOLS));

  for (int i = 0; i < buttonCount; ++i) {
    const int x = left + (i % columns) * (cellWidth + m.menuSpacing);
    const int y = rect.y + (i / columns) * (m.menuRowHeight + m.menuSpacing);

    if (i == selectedIndex) {
      renderer.fillRoundedRect(x, y, cellWidth, m.menuRowHeight, cornerRadius, Color::LightGray);
    }
    renderer.drawRoundedRect(x, y, cellWidth, m.menuRowHeight, 1, cornerRadius, true);

    int textX = x + cellPadding;
    const uint8_t* icon = rowIcon ? iconFor(rowIcon(i)) : nullptr;
    if (icon != nullptr) {
      renderer.drawIcon(icon, textX, y + (m.menuRowHeight - cellIconSize) / 2, cellIconSize);
      textX += cellIconSize + cellPadding;
      renderer.drawLine(textX, y + cellPadding, textX, y + m.menuRowHeight - cellPadding, true);
      textX += cellPadding;
    }

    const std::string label = buttonLabel(i);
    const int maxTextWidth = x + cellWidth - cellPadding - textX;
    const auto lines = renderer.wrappedText(UI_10_FONT_ID, label.c_str(), maxTextWidth, 2);
    int textY = y + (m.menuRowHeight - static_cast<int>(lines.size()) * lineHeight) / 2;
    for (const auto& line : lines) {
      renderer.drawText(UI_10_FONT_ID, textX, textY, line.c_str(), true);
      textY += lineHeight;
    }
  }
}
