#include "widgets/Button.hpp"
#include <cmath>

namespace LE
{
    Button::Button(float x, float y, float w, float h, const std::string& label,
                   const sf::Font& font, std::function<void()> callback)
        : m_label(font, label, static_cast<unsigned int>(h * 0.45f)), m_onClick(callback)
    {
        m_baseCharSize = static_cast<unsigned int>(h * 0.45f);
        m_shape.setFillColor(COLORS[16]);
        m_shape.setOutlineThickness(2.f);
        m_shape.setOutlineColor(COLORS[21]);
        m_label.setFillColor(COLORS[21]);
        m_label.setStyle(sf::Text::Bold);
        m_label.setLetterSpacing(1.08f);
        m_label.setOutlineColor(sf::Color(18, 16, 14, 120));
        m_label.setOutlineThickness(1.0f);
        setFrame(x, y, w, h);
    }

    void Button::updateTextScale(float zoomFactor)
    {
        unsigned int newSize = static_cast<unsigned int>(m_baseCharSize * zoomFactor);
        m_label.setCharacterSize(newSize);
        m_label.setScale({1.f / zoomFactor, 1.f / zoomFactor});
        updateLabelLayout();
    }

    void Button::setFrame(float x, float y, float w, float h)
    {
        m_bounds = sf::FloatRect({x, y}, {w, h});
        m_shape.setSize({w, h});
        m_shape.setPosition({x, y});
        updateLabelLayout();
    }

    void Button::updateLabelLayout()
    {
        sf::FloatRect labelBounds = m_label.getLocalBounds();
        m_label.setOrigin({labelBounds.position.x + labelBounds.size.x / 2.f,
                           labelBounds.position.y + labelBounds.size.y / 2.f});
        m_label.setPosition({std::floor(m_bounds.position.x + m_bounds.size.x / 2.f),
                             std::floor(m_bounds.position.y + m_bounds.size.y / 2.f)});
    }

    void Button::draw(sf::RenderWindow& window)
    {
        if (m_isHovered)
        {
            m_shape.setFillColor(COLORS[21]);
            m_label.setFillColor(COLORS[16]);
        }
        else
        {
            m_shape.setFillColor(COLORS[16]);
            m_label.setFillColor(COLORS[21]);
        }

        window.draw(m_shape);
        window.draw(m_label);
    }

    void Button::updateHover(sf::Vector2f mousePos)
    {
        m_isHovered = m_bounds.contains(mousePos);
    }

    bool Button::contains(sf::Vector2f pos) const
    {
        return m_bounds.contains(pos);
    }

    void Button::click()
    {
        if (m_onClick) m_onClick();
    }
}
