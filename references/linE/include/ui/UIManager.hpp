#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include <string>
#include "widgets/Button.hpp"
#include "graphics/Color.hpp"
#include "ui/PreviewBar.hpp"
#include "systems/Shop.hpp" // For Shop access or shared types

namespace LE
{
    // Interface to delegate logic back to Game
    struct UIActions
    {
        std::function<void()> onShopOpen;
        std::function<void()> onSave;
        std::function<void()> onLoad;
        std::function<void()> onReset;
        std::function<void()> onExit;
        
        // Slot system callbacks
        std::function<bool(int)> onSaveSlot; // returns success
        std::function<bool(int)> onLoadSlot; // returns success
        
        // Shop callbacks
        std::function<void(ItemType)> onShopPurchase;
    };
    
    // UI Manager - Handles all HUD, Panels and Overlay rendering
    class UIManager
    {
    public:
        UIManager();
        ~UIManager() = default;

        // Initialization
        void init(const UIActions& actions);
        void handleResize(unsigned int width, unsigned int height);
        
        // Updates
        void update(float deltaTime, sf::Vector2f mousePos);
        void updateScore(int score); // Sets target score for animation
        void updatePreview(const std::vector<BallColor>& colors);
        void updateTime(float playTime);
        
        // Input Handling - returns true if consumed
        bool handleMouseMove(sf::Vector2f mousePos);
        bool handleClick(sf::Vector2f mousePos);

        // Drawing
        void draw(sf::RenderWindow& window);
        void drawOverlay(sf::RenderWindow& window); // Draw panels on top (Shop, Slots, Exit)

        // Panel Control
        void showSlotPanel(bool isSave);
        void showExitConfirm();
        void setShopVisible(bool visible);
        bool isShopVisible() const;
        bool isAnyPanelOpen() const; // Returns true if Shop, Slot, or Exit panel is open
        
        // Shop Integration
        Shop& getShop() { return m_shop; }
        const sf::Font& getFont() const { return m_font; }

    private:
        // Internal Helpers
        void initSidebar();
        void alignToPixel(sf::Transformable& obj);
        void resetButtonLayout(Button& button, float x, float y, float w, float h);
        void closeTransientPanels();
        
        // Sub-panels
        void drawSlotPanel(sf::RenderWindow& window);
        void drawExitConfirm(sf::RenderWindow& window);
        
        void hideSlotPanel();
        void hideExitConfirm();

    private:
        // Resources
        sf::Font m_font;
        UIActions m_actions;
        
        // Sidebar Elements
        sf::RectangleShape m_sidebarBg;
        
        // Preview
        std::unique_ptr<sf::Text> m_textNextTitle;
        PreviewBar m_previewBar;
        sf::RectangleShape m_previewSep;
        
        // Score
        std::unique_ptr<sf::Text> m_textScoreTitle;
        std::unique_ptr<sf::Text> m_textScoreValue;
        sf::RectangleShape m_scoreSep;
        
        // Score Animation State
        float m_displayScore = 0.f;
        int m_targetScore = 0;
        bool m_scoreAnimating = false;
        
        // Main Buttons
        std::vector<Button> m_buttons;
        
        // Shop System (UI part managed here, logic mostly in Shop class)
        Shop m_shop;
        
        // -----------------------------------------------------------
        // Panel States
        // -----------------------------------------------------------
        
        // Slot Panel
        enum class SlotPanelMode { NONE, SAVE, LOAD };
        SlotPanelMode m_slotPanelMode = SlotPanelMode::NONE;
        std::vector<Button> m_slotButtons;
        bool m_pendingExitAfterSave = false;
        
        // Exit Confirm
        bool m_showExitConfirm = false;
        std::vector<Button> m_exitButtons;
    };
}
