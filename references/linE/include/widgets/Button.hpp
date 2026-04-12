#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include "graphics/Color.hpp"

namespace LE
{
    // Swiss Style Button - minimal, geometric
    class Button
    {
    public:
        Button(float x, float y, float w, float h, const std::string& label, 
               const sf::Font& font, std::function<void()> callback);
        
        void draw(sf::RenderWindow& window);
        void updateHover(sf::Vector2f mousePos);
        bool contains(sf::Vector2f pos) const;
        void click();
        void setFrame(float x, float y, float w, float h);
        
        // Update text scaling for high-DPI/Fullscreen
        void updateTextScale(float zoomFactor);

    private:
        void updateLabelLayout();

        sf::RectangleShape m_shape;
        sf::Text m_label;
        sf::FloatRect m_bounds;
        std::function<void()> m_onClick;
        bool m_isHovered = false;
        unsigned int m_baseCharSize = 0;
    };
}
