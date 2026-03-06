#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/palette/Palette.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <chrono>
#include <functional>

class SearchBox {
public:
    SearchBox(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> backend,
              Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IWindow> window)
        : m_backend(backend) {
        
        auto palette = Hyprtoolkit::CPalette::palette();
        
        
        // Text input directly - no background rectangle
        m_input = Hyprtoolkit::CTextboxBuilder::begin()
            ->placeholder("🔍 Search...")
            ->defaultText("")
            ->multiline(false)
            
             
          
            ->onTextEdited([this](auto, const std::string& t) { onTextChanged(t); })
            ->size(Hyprtoolkit::CDynamicSize(
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                Hyprtoolkit::CDynamicSize::HT_SIZE_PERCENT,
                {1.0F, 1.0F}))
            ->commence();
        
        // Keyboard handler for Enter key
        m_listener = window->m_events.keyboardKey.listen([this](const Hyprtoolkit::Input::SKeyboardKeyEvent& e) {
            if (e.down && e.xkbKeysym == XKB_KEY_Return) {
                if (m_onEnter) {
                    m_onEnter();
                }
                scheduleClear();
            }
        });
    }
    
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> getView() const { return m_input; }
    
    void focus() { m_input->focus(true); }
    
    void onTextChanged(std::function<void(std::string)> cb) { 
        m_onTextChanged = cb; 
    }
    
    void onEnter(std::function<void()> cb) {
        m_onEnter = cb;
    }
    
    void clear() {
        if (auto builder = m_input->rebuild()) {
            builder->defaultText("")->commence();
        }
    }
    
private:
    void onTextChanged(const std::string& text) {
        if (m_timer) m_timer->cancel();
        
        std::string textCopy = text;
        m_timer = m_backend->addTimer(
            std::chrono::milliseconds(150),
            [this, textCopy](auto, void*) { 
                if (m_onTextChanged) m_onTextChanged(textCopy); 
            },
            nullptr, false
        );
    }
    
    void scheduleClear() {
        m_clearTimer = m_backend->addTimer(
            std::chrono::milliseconds(50),
            [this](auto, void*) {
                clear();
            },
            nullptr, false
        );
    }
    
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IBackend> m_backend;
    Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextboxElement> m_input;
    Hyprutils::Memory::CAtomicSharedPointer<Hyprtoolkit::CTimer> m_timer;
    Hyprutils::Memory::CAtomicSharedPointer<Hyprtoolkit::CTimer> m_clearTimer;
    Hyprutils::Memory::CSharedPointer<Hyprutils::Signal::CSignalListener> m_listener;
    
    std::function<void(std::string)> m_onTextChanged;
    std::function<void()> m_onEnter;
};
