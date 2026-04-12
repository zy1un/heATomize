#pragma once
#include <SFML/Graphics.hpp>

namespace LE
{
    bool isInGrids(sf::Vector2f pixelPosition);
    bool isInGrids(sf::Vector2i gridPosition);
    sf::Vector2i pixelToGrid(sf::Vector2f pixelPosition);
}