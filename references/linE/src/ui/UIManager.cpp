#include "ui/UIManager.hpp"
#include "core/Defaults.hpp"
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace LE
{
    namespace
    {
        bool loadUIFont(sf::Font& font)
        {
            const std::array<std::filesystem::path, 7> candidates = {
                std::filesystem::path("C:/Windows/Fonts/timesbd.ttf"),
                std::filesystem::path("C:/Windows/Fonts/georgiab.ttf"),
                std::filesystem::path("C:/Windows/Fonts/arialbd.ttf"),
                std::filesystem::path("assets/fonts/tuffy.ttf"),
                std::filesystem::path("../assets/fonts/tuffy.ttf"),
                std::filesystem::path("../../assets/fonts/tuffy.ttf"),
                std::filesystem::path("C:/Windows/Fonts/arial.ttf"),
            };

            for (const auto& candidate : candidates)
            {
                if (std::filesystem::exists(candidate) && font.openFromFile(candidate))
                {
                    return true;
                }
            }

            return false;
        }
    }

    UIManager::UIManager()
    {
        if (!loadUIFont(m_font))
        {
            std::cerr << "Failed to load UI font from project or system fallbacks" << std::endl;
        }

        m_textNextTitle = std::make_unique<sf::Text>(m_font);
        m_textScoreTitle = std::make_unique<sf::Text>(m_font);
        m_textScoreValue = std::make_unique<sf::Text>(m_font);
    }

    void UIManager::init(const UIActions& actions)
    {
        m_actions = actions;

        m_shop.init(m_font);
        m_shop.setOnPurchase([this](ItemType item) {
            if (m_actions.onShopPurchase)
            {
                m_actions.onShopPurchase(item);
            }
        });

        initSidebar();
    }

    void UIManager::initSidebar()
    {
        m_sidebarBg.setSize(sf::Vector2f(SIDEBAR_WIDTH, MAIN_AREA_HEIGHT));
        m_sidebarBg.setFillColor(sf::Color(40, 40, 40));
        m_sidebarBg.setPosition(sf::Vector2f(static_cast<float>(SIDEBAR_X), 0));

        const float sidebarX = static_cast<float>(SIDEBAR_X);
        const float centerX = sidebarX + SIDEBAR_WIDTH / 2.0f;

        m_textNextTitle->setFont(m_font);
        m_textNextTitle->setString("NEXT");
        m_textNextTitle->setCharacterSize(26);
        m_textNextTitle->setFillColor(sf::Color(215, 210, 198));
        m_textNextTitle->setStyle(sf::Text::Bold);
        m_textNextTitle->setLetterSpacing(1.12f);
        m_textNextTitle->setOutlineColor(sf::Color(18, 16, 14, 180));
        m_textNextTitle->setOutlineThickness(1.0f);
        sf::FloatRect nextBounds = m_textNextTitle->getLocalBounds();
        m_textNextTitle->setOrigin(sf::Vector2f(nextBounds.position.x + nextBounds.size.x / 2.0f, nextBounds.position.y));
        m_textNextTitle->setPosition(sf::Vector2f(centerX, 40.f));
        alignToPixel(*m_textNextTitle);

        const float previewW = GRID_LENGTH * PREVIEW_BALL_COUNT + 5.f * (PREVIEW_BALL_COUNT - 1);
        const float previewX = centerX - previewW / 2.0f;
        m_previewBar.init(previewX, 80.f, GRID_LENGTH, 5.f, PREVIEW_BALL_COUNT);

        m_previewSep.setSize(sf::Vector2f(SIDEBAR_WIDTH - 40.f, 2.f));
        m_previewSep.setFillColor(sf::Color(60, 60, 60));
        m_previewSep.setOrigin(sf::Vector2f((SIDEBAR_WIDTH - 40.f) / 2.f, 0));
        m_previewSep.setPosition(sf::Vector2f(centerX, 150.f));

        m_textScoreTitle->setFont(m_font);
        m_textScoreTitle->setString("SCORE");
        m_textScoreTitle->setCharacterSize(26);
        m_textScoreTitle->setFillColor(sf::Color(215, 210, 198));
        m_textScoreTitle->setStyle(sf::Text::Bold);
        m_textScoreTitle->setLetterSpacing(1.12f);
        m_textScoreTitle->setOutlineColor(sf::Color(18, 16, 14, 180));
        m_textScoreTitle->setOutlineThickness(1.0f);
        sf::FloatRect scoreTitleBounds = m_textScoreTitle->getLocalBounds();
        m_textScoreTitle->setOrigin(sf::Vector2f(scoreTitleBounds.position.x + scoreTitleBounds.size.x / 2.0f,
                                                 scoreTitleBounds.position.y));
        m_textScoreTitle->setPosition(sf::Vector2f(centerX, 180.f));
        alignToPixel(*m_textScoreTitle);

        m_textScoreValue->setFont(m_font);
        m_textScoreValue->setCharacterSize(54);
        m_textScoreValue->setFillColor(sf::Color(255, 214, 78));
        m_textScoreValue->setStyle(sf::Text::Bold);
        m_textScoreValue->setLetterSpacing(1.04f);
        m_textScoreValue->setOutlineColor(sf::Color(24, 20, 18, 190));
        m_textScoreValue->setOutlineThickness(1.6f);
        m_textScoreValue->setPosition(sf::Vector2f(centerX, 220.f));
        updateScore(m_targetScore);
        m_displayScore = static_cast<float>(m_targetScore);
        m_scoreAnimating = false;

        m_scoreSep.setSize(sf::Vector2f(SIDEBAR_WIDTH - 40.f, 2.f));
        m_scoreSep.setFillColor(sf::Color(60, 60, 60));
        m_scoreSep.setOrigin(sf::Vector2f((SIDEBAR_WIDTH - 40.f) / 2.f, 0));
        m_scoreSep.setPosition(sf::Vector2f(centerX, 300.f));

        const float btnW = 140.f;
        const float btnH = 32.f;
        const float buttonsTop = 324.f;
        const float buttonsBottom = WINDOW_HEIGHT - 16.f;
        const float gapY = (buttonsBottom - buttonsTop - btnH * 5.f) / 4.f;
        const float buttonStep = btnH + gapY;
        const float startY = buttonsTop;
        const float btnX = centerX - btnW / 2.0f;

        if (m_buttons.empty())
        {
            m_buttons.emplace_back(btnX, startY, btnW, btnH, "SHOP", m_font,
                [this]() { if (m_actions.onShopOpen) m_actions.onShopOpen(); });
            m_buttons.emplace_back(btnX, startY + buttonStep, btnW, btnH, "LOAD", m_font,
                [this]() { if (m_actions.onLoad) m_actions.onLoad(); });
            m_buttons.emplace_back(btnX, startY + buttonStep * 2.f, btnW, btnH, "RESET", m_font,
                [this]() { if (m_actions.onReset) m_actions.onReset(); });
            m_buttons.emplace_back(btnX, startY + buttonStep * 3.f, btnW, btnH, "SAVE", m_font,
                [this]() { if (m_actions.onSave) m_actions.onSave(); });
            m_buttons.emplace_back(btnX, startY + buttonStep * 4.f, btnW, btnH, "EXIT", m_font,
                [this]() { if (!m_showExitConfirm) showExitConfirm(); });
        }
        else
        {
            for (std::size_t index = 0; index < m_buttons.size(); ++index)
            {
                resetButtonLayout(m_buttons[index], btnX, startY + buttonStep * static_cast<float>(index), btnW, btnH);
            }
        }
    }

    void UIManager::handleResize(unsigned int width, unsigned int height)
    {
        (void)width;
        (void)height;
        initSidebar();
    }

    void UIManager::update(float deltaTime, sf::Vector2f mousePos)
    {
        if (m_scoreAnimating)
        {
            const float diff = static_cast<float>(m_targetScore) - m_displayScore;
            if (std::abs(diff) < 0.5f)
            {
                m_displayScore = static_cast<float>(m_targetScore);
                m_scoreAnimating = false;
            }
            else
            {
                m_displayScore += diff * 5.0f * deltaTime;
            }

            m_textScoreValue->setString(std::to_string(static_cast<int>(m_displayScore)));
            sf::FloatRect bounds = m_textScoreValue->getLocalBounds();
            m_textScoreValue->setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.0f, bounds.position.y));
            m_textScoreValue->setPosition(sf::Vector2f(static_cast<float>(SIDEBAR_X) + SIDEBAR_WIDTH / 2.0f, 220.f));
            alignToPixel(*m_textScoreValue);
        }

        m_previewBar.update(deltaTime);

        if (m_shop.isVisible())
        {
            m_shop.handleMouseMove(mousePos);
            m_shop.update(deltaTime);
        }
    }

    void UIManager::updateScore(int score)
    {
        m_targetScore = score;
        m_scoreAnimating = true;
        m_shop.setScore(score);
    }

    void UIManager::updatePreview(const std::vector<BallColor>& colors)
    {
        m_previewBar.setColors(colors);
    }

    void UIManager::updateTime(float playTime)
    {
        (void)playTime;
    }

    bool UIManager::handleMouseMove(sf::Vector2f mousePos)
    {
        if (m_shop.isVisible())
        {
            m_shop.handleMouseMove(mousePos);
            return true;
        }

        if (m_slotPanelMode != SlotPanelMode::NONE)
        {
            for (auto& btn : m_slotButtons)
            {
                btn.updateHover(mousePos);
            }
            return true;
        }

        if (m_showExitConfirm)
        {
            for (auto& btn : m_exitButtons)
            {
                btn.updateHover(mousePos);
            }
            return true;
        }

        bool consumed = false;
        for (auto& btn : m_buttons)
        {
            btn.updateHover(mousePos);
            consumed = consumed || btn.contains(mousePos);
        }

        return consumed;
    }

    bool UIManager::handleClick(sf::Vector2f mousePos)
    {
        if (m_shop.isVisible())
        {
            m_shop.handleClick(mousePos);
            return true;
        }

        if (m_slotPanelMode != SlotPanelMode::NONE)
        {
            for (auto& btn : m_slotButtons)
            {
                if (btn.contains(mousePos))
                {
                    btn.click();
                    return true;
                }
            }
            hideSlotPanel();
            return true;
        }

        if (m_showExitConfirm)
        {
            for (auto& btn : m_exitButtons)
            {
                if (btn.contains(mousePos))
                {
                    btn.click();
                    return true;
                }
            }
            hideExitConfirm();
            return true;
        }

        for (auto& btn : m_buttons)
        {
            if (btn.contains(mousePos))
            {
                btn.click();
                return true;
            }
        }

        return false;
    }

    void UIManager::draw(sf::RenderWindow& window)
    {
        window.draw(m_sidebarBg);
        window.draw(*m_textNextTitle);
        m_previewBar.draw(window);
        window.draw(m_previewSep);
        window.draw(*m_textScoreTitle);
        window.draw(*m_textScoreValue);
        window.draw(m_scoreSep);

        for (auto& btn : m_buttons)
        {
            btn.draw(window);
        }
    }

    void UIManager::drawOverlay(sf::RenderWindow& window)
    {
        if (m_shop.isVisible())
        {
            m_shop.draw(window);
        }
        else if (m_slotPanelMode != SlotPanelMode::NONE)
        {
            drawSlotPanel(window);
        }
        else if (m_showExitConfirm)
        {
            drawExitConfirm(window);
        }
    }

    void UIManager::setShopVisible(bool visible)
    {
        if (visible)
        {
            hideSlotPanel();
            hideExitConfirm();
            m_shop.show();
        }
        else
        {
            m_shop.hide();
        }
    }

    bool UIManager::isShopVisible() const
    {
        return m_shop.isVisible();
    }

    bool UIManager::isAnyPanelOpen() const
    {
        return isShopVisible() || m_slotPanelMode != SlotPanelMode::NONE || m_showExitConfirm;
    }

    void UIManager::alignToPixel(sf::Transformable& obj)
    {
        sf::Vector2f pos = obj.getPosition();
        obj.setPosition(sf::Vector2f(std::round(pos.x), std::round(pos.y)));
    }

    void UIManager::resetButtonLayout(Button& button, float x, float y, float w, float h)
    {
        button.setFrame(x, y, w, h);
    }

    void UIManager::closeTransientPanels()
    {
        hideSlotPanel();
        hideExitConfirm();
        m_shop.hide();
    }

    void UIManager::showSlotPanel(bool isSave)
    {
        m_shop.hide();
        hideExitConfirm();
        m_slotPanelMode = isSave ? SlotPanelMode::SAVE : SlotPanelMode::LOAD;
        m_pendingExitAfterSave = false;
        m_slotButtons.clear();

        const float btnW = 200.f;
        const float btnH = 50.f;
        const float startX = (WINDOW_WIDTH - btnW) / 2.0f;
        const float startY = (WINDOW_HEIGHT - (4 * 70.f)) / 2.0f;

        for (int i = 0; i < 3; ++i)
        {
            const std::string label = "Slot " + std::to_string(i + 1);
            auto callback = [this, i, isSave]() {
                bool success = false;
                if (isSave)
                {
                    if (m_actions.onSaveSlot)
                    {
                        success = m_actions.onSaveSlot(i);
                    }
                }
                else
                {
                    if (m_actions.onLoadSlot)
                    {
                        success = m_actions.onLoadSlot(i);
                    }
                }

                if (success)
                {
                    if (m_pendingExitAfterSave)
                    {
                        if (m_actions.onExit)
                        {
                            m_actions.onExit();
                        }
                    }
                    else
                    {
                        hideSlotPanel();
                    }
                }
            };

            m_slotButtons.emplace_back(startX, startY + i * 70.f, btnW, btnH, label, m_font, callback);
        }

        m_slotButtons.emplace_back(startX, startY + 3 * 70.f, btnW, btnH, "Cancel", m_font,
            [this]() { hideSlotPanel(); });
    }

    void UIManager::hideSlotPanel()
    {
        m_slotPanelMode = SlotPanelMode::NONE;
        m_slotButtons.clear();
        m_pendingExitAfterSave = false;
    }

    void UIManager::drawSlotPanel(sf::RenderWindow& window)
    {
        sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)));
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        window.draw(overlay);

        sf::Text title(m_font);
        title.setString(m_slotPanelMode == SlotPanelMode::SAVE ? "SAVE GAME" : "LOAD GAME");
        title.setCharacterSize(42);
        title.setFillColor(sf::Color(245, 236, 216));
        title.setStyle(sf::Text::Bold);
        title.setLetterSpacing(1.08f);
        title.setOutlineColor(sf::Color(22, 18, 16, 210));
        title.setOutlineThickness(1.5f);

        sf::FloatRect bounds = title.getLocalBounds();
        title.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f, bounds.position.y));
        title.setPosition(sf::Vector2f(WINDOW_WIDTH / 2.f, 200.f));
        window.draw(title);

        for (auto& btn : m_slotButtons)
        {
            btn.draw(window);
        }
    }

    void UIManager::showExitConfirm()
    {
        m_shop.hide();
        hideSlotPanel();
        m_showExitConfirm = true;
        m_exitButtons.clear();

        const float btnW = 120.f;
        const float btnH = 45.f;
        const float cx = WINDOW_WIDTH / 2.0f;
        const float cy = WINDOW_HEIGHT / 2.0f;

        m_exitButtons.emplace_back(cx - 140.f, cy + 30.f, btnW, btnH, "Just Exit", m_font,
            [this]() { if (m_actions.onExit) m_actions.onExit(); });

        m_exitButtons.emplace_back(cx + 20.f, cy + 30.f, btnW, btnH, "Save & Exit", m_font,
            [this]() {
                showSlotPanel(true);
                m_pendingExitAfterSave = true;
                m_showExitConfirm = false;
            });
    }

    void UIManager::hideExitConfirm()
    {
        m_showExitConfirm = false;
        m_exitButtons.clear();
    }

    void UIManager::drawExitConfirm(sf::RenderWindow& window)
    {
        sf::RectangleShape overlay(sf::Vector2f(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)));
        overlay.setFillColor(sf::Color(0, 0, 0, 200));
        window.draw(overlay);

        sf::Text text(m_font);
        text.setString("Do you want to save before exiting?");
        text.setCharacterSize(30);
        text.setFillColor(sf::Color(245, 236, 216));
        text.setStyle(sf::Text::Bold);
        text.setOutlineColor(sf::Color(22, 18, 16, 170));
        text.setOutlineThickness(1.0f);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f,
                                    bounds.position.y + bounds.size.y / 2.f));
        text.setPosition(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f - 50.f));
        window.draw(text);

        for (auto& btn : m_exitButtons)
        {
            btn.draw(window);
        }
    }
}
