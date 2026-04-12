#pragma once
#include <SFML/Graphics.hpp>
#include "graphics/Color.hpp"
#include "core/Defaults.hpp"
namespace LE
{
    class GridCell
    {
    public:
        GridCell(sf::Vector2i gridPosition, float sideLength,
                 const sf::Color &fillColor,
                 const sf::Color &outlineColor);
        GridCell(sf::Vector2i gridPosition);
        GridCell();
        // 绘制格子
        // @param target: SFML 渲染目标 (通常是 sf::RenderWindow)
        void draw(sf::RenderTarget &target) const;
        // 获取格子的全局边界（用于鼠标点击检测等）
        sf::FloatRect getGlobalBounds() const;
        // 获取格子的位置
        sf::Vector2i getGridPosition() const;
        // 设置格子的填充颜色
        void setFillColor(const sf::Color &color);
        // 设置格子的描边颜色
        void setOutlineColor(const sf::Color &color);
        // 设置格子是否被选中，同时更新状态
        void setSelected(bool selected);
        // 判断格子是否被选中
        bool isSelected() const;
        // 设置格子位置（像素坐标，中心点）
        void setPosition(sf::Vector2f position);
        // 设置格子尺寸
        void setSize(float size);


    private:
        sf::RectangleShape shape; // SFML 矩形对象，用于实际绘制
        sf::Color fillColor = COLORS[18];      // 默认填充颜色
        sf::Color outlineColor = COLORS[17];   // 默认描边颜色
        sf::Vector2i gridPosition;
        //sf::Color selectedColor;  // 选中时的填充颜色 (可以与默认颜色不同)
        bool m_isSelected;          // 是否被选中
        int sideLength = GRID_SIDE;
    };
}
