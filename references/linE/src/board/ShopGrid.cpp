#include "board/ShopGrid.hpp"
#include "graphics/Color.hpp"
#include "core/Defaults.hpp"
#include <algorithm>

namespace LE
{
    namespace
    {
        sf::Color dimColor(const sf::Color& color, float factor)
        {
            return sf::Color(
                static_cast<uint8_t>(color.r * factor),
                static_cast<uint8_t>(color.g * factor),
                static_cast<uint8_t>(color.b * factor),
                color.a);
        }
    }

    ShopGrid::ShopGrid()
    {
    }

    void ShopGrid::initShop(const sf::Font& font, float offsetX, float offsetY)
    {
        m_font = &font;

        GridConfig config;
        config.cols = 2;
        config.rows = 1;
        config.cellSize = 80.f;
        config.cellGap = 12.f;
        config.offsetX = offsetX;
        config.offsetY = offsetY;
        config.fillColor = COLORS[18];
        config.outlineColor = COLORS[17];
        config.hoverColor = sf::Color(
            static_cast<uint8_t>(std::min(255, COLORS[18].r + 30)),
            static_cast<uint8_t>(std::min(255, COLORS[18].g + 30)),
            static_cast<uint8_t>(std::min(255, COLORS[18].b + 30)));

        init(config);
        createDisplayElements();
    }

    void ShopGrid::createDisplayElements()
    {
        m_items.resize(m_config.cols);
        m_displayBalls.resize(m_config.cols);
        m_priceLabels.resize(m_config.cols);
        m_quantityBadges.resize(m_config.cols);

        for (int col = 0; col < m_config.cols; ++col)
        {
            m_items[col].resize(m_config.rows);
            m_displayBalls[col].resize(m_config.rows);
            m_priceLabels[col].resize(m_config.rows);
            m_quantityBadges[col].resize(m_config.rows);
        }
    }

    void ShopGrid::setItems(const std::vector<ShopItemData>& items)
    {
        for (int col = 0; col < m_config.cols; ++col)
        {
            for (int row = 0; row < m_config.rows; ++row)
            {
                m_items[col][row] = ShopItemData{};
                m_displayBalls[col][row].reset();
                m_priceLabels[col][row].reset();
                m_quantityBadges[col][row].reset();
            }
        }

        int idx = 0;
        for (int row = 0; row < m_config.rows && idx < static_cast<int>(items.size()); ++row)
        {
            for (int col = 0; col < m_config.cols && idx < static_cast<int>(items.size()); ++col)
            {
                const auto& item = items[idx++];
                m_items[col][row] = item;

                const sf::Vector2f center = gridToPixel(col, row);
                auto ball = std::make_unique<Ball>(sf::Vector2i{col, row}, item.displayColor);
                ball->setPixelPosition({center.x, center.y - 10.f});
                ball->finishSpawn();
                m_displayBalls[col][row] = std::move(ball);

                if (m_font)
                {
                    auto priceLabel = std::make_unique<sf::Text>(*m_font);
                    priceLabel->setString(std::to_string(item.price) + " P");
                    priceLabel->setCharacterSize(12);
                    priceLabel->setFillColor(sf::Color(255, 200, 50));
                    priceLabel->setStyle(sf::Text::Bold);
                    const sf::FloatRect bounds = priceLabel->getLocalBounds();
                    priceLabel->setPosition({center.x - bounds.size.x / 2.f, center.y + m_config.cellSize / 2.f - 20.f});
                    m_priceLabels[col][row] = std::move(priceLabel);

                    if (item.quantity > 1)
                    {
                        auto badge = std::make_unique<sf::Text>(*m_font);
                        badge->setString("x" + std::to_string(item.quantity));
                        badge->setCharacterSize(11);
                        badge->setFillColor(sf::Color(255, 100, 100));
                        badge->setStyle(sf::Text::Bold);
                        badge->setPosition({center.x + m_config.cellSize / 2.f - 25.f, center.y - m_config.cellSize / 2.f + 5.f});
                        m_quantityBadges[col][row] = std::move(badge);
                    }
                }
            }
        }

        updateItemAvailability();
    }

    void ShopGrid::setScore(int score)
    {
        m_score = score;
        updateItemAvailability();
    }

    void ShopGrid::updateItemAvailability()
    {
        for (int col = 0; col < m_config.cols; ++col)
        {
            for (int row = 0; row < m_config.rows; ++row)
            {
                auto& item = m_items[col][row];
                item.available = (item.price > 0 && m_score >= item.price);

                if (item.price <= 0)
                {
                    continue;
                }

                sf::Color fill = item.available ? m_config.fillColor : dimColor(m_config.fillColor, 0.6f);
                if (m_hoverCell.x == col && m_hoverCell.y == row)
                {
                    fill = item.available ? m_config.hoverColor : dimColor(m_config.hoverColor, 0.75f);
                }

                setCellColor(col, row, fill, m_config.outlineColor);
            }
        }
    }

    void ShopGrid::draw(sf::RenderTarget& target) const
    {
        GridSystem::draw(target);

        for (int col = 0; col < m_config.cols; ++col)
        {
            for (int row = 0; row < m_config.rows; ++row)
            {
                if (m_displayBalls[col][row])
                {
                    m_displayBalls[col][row]->draw(target);
                }
                if (m_priceLabels[col][row])
                {
                    target.draw(*m_priceLabels[col][row]);
                }
                if (m_quantityBadges[col][row])
                {
                    target.draw(*m_quantityBadges[col][row]);
                }
            }
        }
    }

    void ShopGrid::update(float deltaTime)
    {
        GridSystem::update(deltaTime);

        for (int col = 0; col < m_config.cols; ++col)
        {
            for (int row = 0; row < m_config.rows; ++row)
            {
                if (m_displayBalls[col][row])
                {
                    m_displayBalls[col][row]->updateSpawn(deltaTime);
                }
            }
        }
    }

    void ShopGrid::handleMouseMove(sf::Vector2f mousePos)
    {
        const sf::Vector2i gridPos = pixelToGrid(mousePos);
        if (isInGrid(gridPos))
        {
            m_hoverCell = gridPos;
        }
        else
        {
            m_hoverCell = {-1, -1};
        }

        updateItemAvailability();
    }

    bool ShopGrid::handleClick(sf::Vector2f mousePos)
    {
        const sf::Vector2i gridPos = pixelToGrid(mousePos);
        if (!isInGrid(gridPos))
        {
            return false;
        }

        const auto& item = m_items[gridPos.x][gridPos.y];
        if (item.price <= 0)
        {
            return false;
        }

        if (m_onPurchase)
        {
            m_onPurchase(item.available ? item.id : -1);
        }
        return true;
    }

    void ShopGrid::setOnPurchase(std::function<void(int itemId)> callback)
    {
        m_onPurchase = callback;
    }

    const ShopItemData* ShopGrid::getItemAt(int col, int row) const
    {
        if (!isInGrid(col, row))
        {
            return nullptr;
        }
        if (m_items[col][row].price <= 0)
        {
            return nullptr;
        }
        return &m_items[col][row];
    }
}
