#include "systems/Shop.hpp"
#include "graphics/Color.hpp"
#include "core/Defaults.hpp"
#include <algorithm>
#include <cmath>

namespace LE
{
    Shop::Shop()
    {
        m_items.clear();
    }

    void Shop::init(const sf::Font& font)
    {
        m_font = &font;
        createUI();
        m_initialized = true;
    }

    void Shop::createUI()
    {
        const float panelX = (WINDOW_WIDTH - SHOP_PANEL_WIDTH) / 2.f;
        const float panelY = (WINDOW_HEIGHT - SHOP_PANEL_HEIGHT) / 2.f;

        m_overlay.setSize({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
        m_overlay.setPosition({0.f, 0.f});
        m_overlay.setFillColor(sf::Color(0, 0, 0, SHOP_OVERLAY_ALPHA));

        m_panelBorder.setSize({static_cast<float>(SHOP_PANEL_WIDTH + 4), static_cast<float>(SHOP_PANEL_HEIGHT + 4)});
        m_panelBorder.setPosition({panelX - 2.f, panelY - 2.f});
        m_panelBorder.setFillColor(COLORS[21]);

        m_panel.setSize({static_cast<float>(SHOP_PANEL_WIDTH), static_cast<float>(SHOP_PANEL_HEIGHT)});
        m_panel.setPosition({panelX, panelY});
        m_panel.setFillColor(COLORS[21]);

        m_titleBar.setSize({static_cast<float>(SHOP_PANEL_WIDTH), static_cast<float>(SHOP_TITLE_HEIGHT)});
        m_titleBar.setPosition({panelX, panelY});
        m_titleBar.setFillColor(sf::Color(COLORS[21].r, COLORS[21].g, COLORS[21].b, 40));

        m_titleText = std::make_unique<sf::Text>(*m_font);
        m_titleText->setString("SHOP");
        m_titleText->setCharacterSize(SHOP_TITLE_FONT_SIZE + 4);
        m_titleText->setFillColor(sf::Color(246, 236, 214));
        m_titleText->setStyle(sf::Text::Bold);
        m_titleText->setLetterSpacing(1.12f);
        m_titleText->setOutlineColor(sf::Color(22, 18, 16, 180));
        m_titleText->setOutlineThickness(1.2f);
        sf::FloatRect titleBounds = m_titleText->getLocalBounds();
        m_titleText->setPosition({panelX + (SHOP_PANEL_WIDTH - titleBounds.size.x) / 2.f,
                                  panelY + (SHOP_TITLE_HEIGHT - titleBounds.size.y) / 2.f - 4.f});

        const float coinsY = panelY + SHOP_TITLE_HEIGHT + SHOP_PADDING;
        m_coinsBg.setSize({static_cast<float>(SHOP_PANEL_WIDTH - 2 * SHOP_PADDING), static_cast<float>(SHOP_COINS_HEIGHT)});
        m_coinsBg.setPosition({panelX + SHOP_PADDING, coinsY});
        m_coinsBg.setFillColor(sf::Color(0, 0, 0, 60));

        m_coinIcon.setRadius(12.f);
        m_coinIcon.setFillColor(sf::Color(255, 200, 50));
        m_coinIcon.setOutlineThickness(2.f);
        m_coinIcon.setOutlineColor(sf::Color(200, 150, 30));
        m_coinIcon.setPosition({panelX + SHOP_PADDING + 10.f, coinsY + (SHOP_COINS_HEIGHT - 24.f) / 2.f});

        m_coinsLabel = std::make_unique<sf::Text>(*m_font);
        m_coinsLabel->setString("SCORE");
        m_coinsLabel->setCharacterSize(15);
        m_coinsLabel->setFillColor(sf::Color(210, 202, 188));
        m_coinsLabel->setStyle(sf::Text::Bold);
        m_coinsLabel->setLetterSpacing(1.08f);
        m_coinsLabel->setPosition({panelX + SHOP_PADDING + 40.f, coinsY + 8.f});

        m_coinsValue = std::make_unique<sf::Text>(*m_font);
        m_coinsValue->setCharacterSize(28);
        m_coinsValue->setFillColor(sf::Color(255, 214, 78));
        m_coinsValue->setStyle(sf::Text::Bold);
        m_coinsValue->setOutlineColor(sf::Color(22, 18, 16, 170));
        m_coinsValue->setOutlineThickness(1.2f);

        const float sepY = coinsY + SHOP_COINS_HEIGHT + SHOP_PADDING;
        m_separator.setSize({static_cast<float>(SHOP_PANEL_WIDTH - 2 * SHOP_PADDING), 2.f});
        m_separator.setPosition({panelX + SHOP_PADDING, sepY});
        m_separator.setFillColor(sf::Color(COLORS[21].r, COLORS[21].g, COLORS[21].b, 100));

        const float gridOffsetX = panelX + (SHOP_PANEL_WIDTH - (2 * 80.f + 12.f)) / 2.f;
        const float gridOffsetY = sepY + SHOP_PADDING + 10.f;
        m_shopGrid.initShop(*m_font, gridOffsetX, gridOffsetY);

        std::vector<ShopItemData> items;

        ShopItemData item1;
        item1.id = static_cast<int>(ItemType::RAINBOW_PLACE);
        item1.name = "Rainbow Ball";
        item1.price = RAINBOW_PLACE_PRICE;
        item1.displayColor = BallColor::Rainbow;
        item1.isRainbow = true;
        item1.quantity = 1;
        items.push_back(item1);

        ShopItemData item2;
        item2.id = static_cast<int>(ItemType::RAINBOW_PREVIEW);
        item2.name = "Rainbow Preview";
        item2.price = RAINBOW_PREVIEW_PRICE;
        item2.displayColor = BallColor::Rainbow;
        item2.isRainbow = true;
        item2.quantity = 3;
        items.push_back(item2);

        m_shopGrid.setItems(items);
        m_shopGrid.setScore(m_score);
        m_shopGrid.setOnPurchase([this](int itemId) {
            if (itemId == -1)
            {
                showHint("Not enough points!");
                return;
            }

            const ItemType type = static_cast<ItemType>(itemId);
            const int price = (type == ItemType::RAINBOW_PLACE) ? RAINBOW_PLACE_PRICE : RAINBOW_PREVIEW_PRICE;

            if (m_score >= price)
            {
                if (m_onPurchase)
                {
                    m_onPurchase(type);
                }
            }
            else
            {
                showHint("Not enough points!");
            }
        });

        m_hintText = std::make_unique<sf::Text>(*m_font);
        m_hintText->setString("");
        m_hintText->setCharacterSize(15);
        m_hintText->setFillColor(sf::Color(255, 220, 150));
        m_hintText->setStyle(sf::Text::Bold);
        m_hintText->setOutlineColor(sf::Color(22, 18, 16, 160));
        m_hintText->setOutlineThickness(0.8f);
        const float hintY = gridOffsetY + 2 * (80.f + 12.f) + 10.f;
        m_hintText->setPosition({panelX + SHOP_PADDING, hintY});

        const float closeY = panelY + SHOP_PANEL_HEIGHT - SHOP_PADDING - SHOP_BUTTON_HEIGHT;
        const float closeWidth = 120.f;
        const float closeX = panelX + (SHOP_PANEL_WIDTH - closeWidth) / 2.f;

        m_closeButton.setSize({closeWidth, static_cast<float>(SHOP_BUTTON_HEIGHT)});
        m_closeButton.setPosition({closeX, closeY});
        m_closeButton.setFillColor(COLORS[21]);

        m_closeButtonText = std::make_unique<sf::Text>(*m_font);
        m_closeButtonText->setString("CLOSE");
        m_closeButtonText->setCharacterSize(17);
        m_closeButtonText->setFillColor(COLORS[16]);
        m_closeButtonText->setStyle(sf::Text::Bold);
        m_closeButtonText->setLetterSpacing(1.08f);
        sf::FloatRect closeBounds = m_closeButtonText->getLocalBounds();
        m_closeButtonText->setPosition({closeX + (closeWidth - closeBounds.size.x) / 2.f,
                                        closeY + (SHOP_BUTTON_HEIGHT - closeBounds.size.y) / 2.f - 2.f});

        updateLayout();
    }

    void Shop::show()
    {
        m_isVisible = true;
        updateLayout();
    }

    void Shop::hide()
    {
        m_isVisible = false;
        m_closeButtonHovered = false;
        m_shopGrid.handleMouseMove({-1.f, -1.f});
        if (m_onClose)
        {
            m_onClose();
        }
    }

    sf::FloatRect Shop::getBounds() const
    {
        return m_panel.getGlobalBounds();
    }

    void Shop::update(float deltaTime)
    {
        if (!m_isVisible)
        {
            return;
        }

        m_shopGrid.update(deltaTime);
        updateHint(deltaTime);
    }

    void Shop::handleMouseMove(sf::Vector2f mousePos)
    {
        if (!m_isVisible)
        {
            return;
        }

        const bool hovered = m_closeButton.getGlobalBounds().contains(mousePos);
        if (hovered != m_closeButtonHovered)
        {
            m_closeButtonHovered = hovered;
            if (m_closeButtonHovered)
            {
                m_closeButton.setFillColor(sf::Color(
                    static_cast<uint8_t>(std::min(255, COLORS[21].r + 30)),
                    static_cast<uint8_t>(std::min(255, COLORS[21].g + 30)),
                    static_cast<uint8_t>(std::min(255, COLORS[21].b + 30))));
            }
            else
            {
                m_closeButton.setFillColor(COLORS[21]);
            }
        }

        m_shopGrid.handleMouseMove(mousePos);
    }

    bool Shop::handleClick(sf::Vector2f mousePos)
    {
        if (!m_isVisible)
        {
            return false;
        }

        if (m_closeButton.getGlobalBounds().contains(mousePos))
        {
            hide();
            return true;
        }

        if (m_shopGrid.handleClick(mousePos))
        {
            return true;
        }

        if (!m_panel.getGlobalBounds().contains(mousePos))
        {
            hide();
            return true;
        }

        return true;
    }

    bool Shop::spendScore(int amount)
    {
        if (m_score >= amount)
        {
            m_score -= amount;
            updateLayout();
            return true;
        }
        return false;
    }

    void Shop::updateLayout()
    {
        if (!m_initialized || !m_coinsValue)
        {
            return;
        }

        m_coinsValue->setString(std::to_string(m_score));
        m_shopGrid.setScore(m_score);

        const sf::FloatRect bounds = m_coinsValue->getLocalBounds();
        const float x = m_coinsBg.getPosition().x + 40.f;
        const float y = m_coinsBg.getPosition().y + 22.f;
        m_coinsValue->setPosition({x, y});
        m_coinsValue->setOrigin({0.f, 0.f});
        (void)bounds;
    }

    void Shop::draw(sf::RenderWindow& window) const
    {
        if (!m_isVisible || !m_initialized)
        {
            return;
        }

        window.draw(m_overlay);
        window.draw(m_panelBorder);
        window.draw(m_panel);
        window.draw(m_titleBar);
        if (m_titleText) window.draw(*m_titleText);
        window.draw(m_coinsBg);
        window.draw(m_coinIcon);
        if (m_coinsLabel) window.draw(*m_coinsLabel);
        if (m_coinsValue) window.draw(*m_coinsValue);
        window.draw(m_separator);
        m_shopGrid.draw(window);
        if (m_showHint && m_hintText) window.draw(*m_hintText);
        window.draw(m_closeButton);
        if (m_closeButtonText) window.draw(*m_closeButtonText);
    }

    void Shop::showHint(const std::string& message)
    {
        if (m_hintText)
        {
            m_hintText->setString(message);
            m_hintText->setFillColor(sf::Color(255, 220, 150));
            const sf::FloatRect bounds = m_hintText->getLocalBounds();
            const float panelX = (WINDOW_WIDTH - SHOP_PANEL_WIDTH) / 2.f;
            m_hintText->setPosition({panelX + (SHOP_PANEL_WIDTH - bounds.size.x) / 2.f,
                                     m_hintText->getPosition().y});
        }
        m_showHint = true;
        m_hintTimer = 2.0f;
    }

    void Shop::updateHint(float deltaTime)
    {
        if (!m_showHint)
        {
            return;
        }

        m_hintTimer -= deltaTime;
        if (m_hintTimer <= 0.f)
        {
            m_showHint = false;
            return;
        }

        const uint8_t alpha = static_cast<uint8_t>(std::min(255.f, m_hintTimer * 255.f));
        if (m_hintText)
        {
            sf::Color color = m_hintText->getFillColor();
            color.a = alpha;
            m_hintText->setFillColor(color);
        }
    }
}
