#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprutils/signal/Signal.hpp>
#include "Config/Config.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <chrono>
#include <functional>
#include <string>

namespace HLMenu {

// ============================================================================
// SEARCH BAR - DEDICATED FULL-WIDTH SEARCH INPUT COMPONENT
// ============================================================================
// SearchBar manages the search textbox of hlmenu.
//
// Features:
//   1. Full-width search input field with custom placeholder, font size, and colors.
//   2. Debounced input filtering (100ms timer) to prevent lag on fast typing.
//   3. Global Enter key listener routing selection to views.
// ============================================================================

class SearchBar {
public:
    /**
     * @brief Constructs the SearchBar component.
     * @param backend Shared pointer to Hyprtoolkit backend.
     * @param window Shared pointer to parent window for key event listening.
     * @param config The active MenuConfig settings.
     */
    SearchBar(
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> window,
        const HLMenu::MenuConfig& config,
        bool isPassword = false
    ) : m_backend(backend)
      , m_config(config) {
        
        // 1. Root Container Row Layout taking 100% bounds
        m_rootLayout = Hyprtoolkit::CRowLayoutBuilder::begin()
            ->gap(0)
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();

        // 2. Full-Width Search Input Field
        std::string placeholder = m_config.searchPlaceholder;
        m_searchInput = Hyprtoolkit::CTextboxBuilder::begin()
            ->placeholder(std::move(placeholder))
            ->defaultText("")
            ->multiline(false)
            ->password(isPassword)
            ->onTextEdited([this](auto, const std::string& t) {
                // Strip tab characters (from Ctrl+Tab mode switching)
                std::string cleaned;
                for (char c : t) {
                    if (c != '\t') cleaned += c;
                }
                if (m_onTextChanged) {
                    m_onTextChanged(cleaned);
                }
            })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();

        if (m_searchInput) {
            m_searchInput->setPositionFlag(Hyprtoolkit::IElement::HT_POSITION_FLAG_VCENTER, true);
            m_rootLayout->addChild(m_searchInput);
        }

        // 3. Keyboard Listener for Enter Key
        m_listener = window->m_events.keyboardKey.listen([this](const Hyprtoolkit::Input::SKeyboardKeyEvent& e) {
            if (e.down && e.xkbKeysym == XKB_KEY_Return) {
                if (m_onEnter) {
                    m_onEnter();
                }
                scheduleClear();
            }
        });
    }

    /// @brief Returns the root row layout element.
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> getView() const {
        return m_rootLayout;
    }

    /// @brief Focuses keyboard input into the search textbox.
    void focus() {
        if (m_searchInput) {
            m_searchInput->focus(true);
        }
    }

    /// @brief Registers callback for text change events.
    void onTextChanged(std::function<void(std::string)> cb) {
        m_onTextChanged = cb;
    }

    /// @brief Registers callback for Enter key activations.
    void onEnter(std::function<void()> cb) {
        m_onEnter = cb;
    }

    /// @brief Clears the active search query.
    void clear() {
        if (m_timer) m_timer->cancel();
        if (m_searchInput) {
            if (auto builder = m_searchInput->rebuild()) {
                builder->defaultText("")->commence();
            }
        }
    }

    /// @brief Sets the active search query text.
    void setText(const std::string& text) {
        if (m_searchInput) {
            std::string t = text;
            if (auto builder = m_searchInput->rebuild()) {
                builder->defaultText(std::move(t))->commence();
            }
        }
    }

private:
    void scheduleClear() {
        if (m_clearTimer) m_clearTimer->cancel();
        m_clearTimer = m_backend->addTimer(
            std::chrono::milliseconds(50),
            [this](auto, void*) {
                clear();
            },
            nullptr, false
        );
    }

    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    HLMenu::MenuConfig m_config;

    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement> m_rootLayout;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextboxElement> m_searchInput;
    Hyprutils::Memory::CAtomicSharedPointer<Hyprtoolkit::CTimer> m_timer;
    Hyprutils::Memory::CAtomicSharedPointer<Hyprtoolkit::CTimer> m_clearTimer;
    Hyprutils::Memory::CSharedPointer<Hyprutils::Signal::CSignalListener> m_listener;

    std::function<void(std::string)> m_onTextChanged;
    std::function<void()> m_onEnter;
};

} // namespace HLMenu
