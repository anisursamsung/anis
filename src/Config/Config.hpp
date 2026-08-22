#pragma once

#include <string>
#include <hyprtoolkit/palette/Color.hpp>
#include <hyprutils/math/Vector2D.hpp>

namespace HLMenu {

    struct MenuConfig {
        // --- Window & Layout ---
        Hyprtoolkit::CHyprColor background              = Hyprtoolkit::CHyprColor(0.12f, 0.12f, 0.18f, 0.95f);
        Hyprtoolkit::CHyprColor borderColor             = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f);
        int borderThickness                             = 2;
        int cornerRadiusBig                             = 12;
        int cornerRadiusSmall                           = 6;
        int windowPadding                               = 16;
        std::string fontFamily                          = "Sans";
        std::string monospaceFont                       = "monospace";
        std::string iconPack                            = "Papirus";
        std::string defaultMode                         = "grid";

        Hyprutils::Math::Vector2D windowSize            = Hyprutils::Math::Vector2D(680, 480);
        Hyprutils::Math::Vector2D marginTopLeft         = Hyprutils::Math::Vector2D(0, 0);
        Hyprutils::Math::Vector2D marginBottomRight     = Hyprutils::Math::Vector2D(0, 0);
        uint32_t anchorMask                             = 0; // 0 = Center

        // --- Title Bar (Header / Mode Switcher Tabs) ---
        bool showTitleBar                               = true;
        int titleBarHeight                              = 32;
        int titleBarGap                                 = 8;
        bool showModeTabs                               = true;
        std::string topbarTitle                         = "Applications";
        int topbarTitleFontSize                         = 15;
        Hyprtoolkit::CHyprColor topbarTitleFontColor    = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f); // #89b4fa

        std::string menuTitle                           = "Applications";
        int titleFontSize                               = 15;
        Hyprtoolkit::CHyprColor titleFontColor          = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f);

        // --- Search Bar ---
        bool showSearchBar                              = true;
        int searchBarHeight                             = 38;
        int searchBarGap                                = 8;
        Hyprtoolkit::CHyprColor searchBackground        = Hyprtoolkit::CHyprColor(0.19f, 0.20f, 0.27f, 0.80f); // #313244cc
        Hyprtoolkit::CHyprColor searchBorderColor       = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f);
        int searchBorderSize                            = 1;
        int searchCornerRadius                          = 8;
        int searchFontSize                              = 13;
        Hyprtoolkit::CHyprColor searchFontColor         = Hyprtoolkit::CHyprColor(0.80f, 0.84f, 0.96f, 1.00f); // #cdd6f4
        std::string searchPlaceholder                   = "Type to search...";
        Hyprtoolkit::CHyprColor searchPlaceholderColor  = Hyprtoolkit::CHyprColor(0.65f, 0.68f, 0.78f, 0.70f);
        double searchWidthPercent                       = 1.0;
        bool searchWidthIsPercent                       = true;
        int searchHeight                                = 38;
        int topbarHeight                                = 46;
        float topbarTitleRatio                          = 0.28f;

        // --- Subtitles Toggle (Applies to all modes in List & Grid) ---
        bool showSubtitles                              = true;

        // --- Grid View & Items ---
        int gridItemWidth                               = 100;
        int gridItemHeight                              = 88;
        int gridItemHorizontalGap                       = 10;
        int gridItemVerticalGap                         = 10;
        int gridItemCornerRadius                        = 8;
        int gridItemIconSize                            = 44;
        int gridItemFontSize                            = 11;
        int gridItemDescFontSize                        = 9;
        int gridItemPadding                             = 6;
        
        Hyprtoolkit::CHyprColor gridItemBackground        = Hyprtoolkit::CHyprColor(0.0f, 0.0f, 0.0f, 0.0f);
        Hyprtoolkit::CHyprColor gridItemBorderColor       = Hyprtoolkit::CHyprColor(0.0f, 0.0f, 0.0f, 0.0f);
        int gridItemBorderSize                          = 0;
        Hyprtoolkit::CHyprColor gridItemActiveBackground  = Hyprtoolkit::CHyprColor(0.19f, 0.20f, 0.27f, 0.80f); // #313244cc
        Hyprtoolkit::CHyprColor gridItemActiveBorderColor = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f); // #89b4fa
        int gridItemActiveBorderSize                      = 1;
        Hyprtoolkit::CHyprColor gridItemFontColor         = Hyprtoolkit::CHyprColor(0.80f, 0.84f, 0.96f, 1.00f);
        Hyprtoolkit::CHyprColor gridItemActiveFontColor   = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f);
        Hyprtoolkit::CHyprColor gridItemDescFontColor     = Hyprtoolkit::CHyprColor(0.65f, 0.68f, 0.78f, 0.80f);

        // --- List View & Items ---
        int listItemHeight                              = 58;
        int listItemsVerticalGap                        = 6;
        int listItemCornerRadius                        = 8;
        int listItemIconSize                            = 36;
        int listItemTitleFontSize                       = 13;
        int listItemDescFontSize                        = 11;
        int listItemPadding                             = 8;

        Hyprtoolkit::CHyprColor listItemBackground        = Hyprtoolkit::CHyprColor(0.0f, 0.0f, 0.0f, 0.0f);
        Hyprtoolkit::CHyprColor listItemBorderColor       = Hyprtoolkit::CHyprColor(0.0f, 0.0f, 0.0f, 0.0f);
        int listItemBorderSize                          = 0;
        Hyprtoolkit::CHyprColor listItemActiveBackground  = Hyprtoolkit::CHyprColor(0.19f, 0.20f, 0.27f, 0.80f);
        Hyprtoolkit::CHyprColor listItemActiveBorderColor = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f);
        int listItemActiveBorderSize                      = 1;
        Hyprtoolkit::CHyprColor listItemTitleFontColor    = Hyprtoolkit::CHyprColor(0.80f, 0.84f, 0.96f, 1.00f);
        Hyprtoolkit::CHyprColor listItemActiveTitleFontColor = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f);
        Hyprtoolkit::CHyprColor listItemDescFontColor     = Hyprtoolkit::CHyprColor(0.65f, 0.68f, 0.78f, 1.00f);

        // --- Help View ---
        int helpHeaderFontSize                          = 15;
        Hyprtoolkit::CHyprColor helpHeaderFontColor     = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f);
        int helpTextFontSize                            = 12;
        Hyprtoolkit::CHyprColor helpTextFontColor       = Hyprtoolkit::CHyprColor(0.80f, 0.84f, 0.96f, 1.00f);

        // Legacy / Fallback values
        Hyprtoolkit::CHyprColor foreground              = Hyprtoolkit::CHyprColor(0.80f, 0.84f, 0.96f, 1.00f);
        Hyprtoolkit::CHyprColor primary                 = Hyprtoolkit::CHyprColor(0.54f, 0.71f, 0.98f, 1.00f);
        Hyprtoolkit::CHyprColor onPrimary               = Hyprtoolkit::CHyprColor(0.07f, 0.07f, 0.11f, 1.00f);
        Hyprtoolkit::CHyprColor secondary               = Hyprtoolkit::CHyprColor(0.19f, 0.20f, 0.27f, 0.80f);
        Hyprtoolkit::CHyprColor onSecondary             = Hyprtoolkit::CHyprColor(0.80f, 0.84f, 0.96f, 1.00f);
        Hyprtoolkit::CHyprColor tertiary                = Hyprtoolkit::CHyprColor(0.27f, 0.28f, 0.35f, 0.80f);
        Hyprtoolkit::CHyprColor onTertiary              = Hyprtoolkit::CHyprColor(0.65f, 0.68f, 0.78f, 1.00f);
        Hyprtoolkit::CHyprColor accent                  = Hyprtoolkit::CHyprColor(0.79f, 0.65f, 0.97f, 1.00f);
        Hyprtoolkit::CHyprColor onAccent                = Hyprtoolkit::CHyprColor(0.07f, 0.07f, 0.11f, 1.00f);
        int fontSize                                    = 12;
        int h1FontSize                                  = 20;
        int h2FontSize                                  = 16;
        int h3FontSize                                  = 14;
        bool italic                                     = false;
        int itemBorderSize                              = 1;
    };

}
