#include <SFML/Graphics.hpp>
#include "board/GridCell.hpp"

using namespace LE;

GridCell ::GridCell(sf::Vector2i gridPosition, float sideLength,
                    const sf::Color &fillColor,
                    const sf::Color &outlineColor)
    : fillColor(fillColor),                        // 初始化成员 fillColor
      outlineColor(outlineColor),                  // 初始化成员 outlineColor
      shape(sf::Vector2f(sideLength, sideLength)), // 初始化成员 shape
      m_isSelected(false),                         // 初始化成员 m_isSelected
      gridPosition(gridPosition)
{
    shape.setOrigin({sideLength / 2.f, sideLength / 2.f}); // 使用.f确保浮点运算
    shape.setPosition({FIRST_GRID_CENTER + gridPosition.x * GRID_LENGTH, FIRST_GRID_CENTER + gridPosition.y * GRID_LENGTH});
    shape.setFillColor(this->fillColor); // 使用成员变量
    shape.setOutlineThickness(GRID_BORDER);
    shape.setOutlineColor(this->outlineColor); // 使用成员变量
}

GridCell ::GridCell(sf::Vector2i gridPosition)
    : shape(sf::Vector2f(GRID_SIDE, GRID_SIDE)), // 使用常量而非未初始化的成员变量
      m_isSelected(false),                         // 初始化成员 m_isSelected
      gridPosition(gridPosition)
{
    shape.setOrigin({GRID_SIDE / 2.f, GRID_SIDE / 2.f}); // 使用常量
    shape.setPosition({FIRST_GRID_CENTER + gridPosition.x * GRID_LENGTH, FIRST_GRID_CENTER + gridPosition.y * GRID_LENGTH});
    shape.setFillColor(this->fillColor); // 使用成员变量
    shape.setOutlineThickness(GRID_BORDER);
    shape.setOutlineColor(this->outlineColor); // 使用成员变量
}

GridCell ::GridCell() {}

// 绘制格子方法的实现
void GridCell::draw(sf::RenderTarget &target) const // <-- 注意这里！
{
    // 如果你有m_isSelected属性，可以在这里根据selected状态改变颜色或边框
    // if (m_isSelected) {
    // 例如，如果被选中，改变其轮廓颜色或填充颜色
    // shape.setOutlineColor(sf::Color::Yellow);
    // 或绘制一个表示选中的额外图形
    //}
    target.draw(this->shape); // 绘制私有成员 shape
}

// 获取格子的全局边界方法的实现
sf::FloatRect GridCell::getGlobalBounds() const // <-- 注意这里！
{
    return shape.getGlobalBounds(); // 返回私有成员 shape 的全局边界
}

// 获取格子的位置方法的实现
sf::Vector2i GridCell::getGridPosition() const // <-- 注意这里！
{
    return gridPosition; // 返回私有成员 shape 的位置
}

// 设置格子的填充颜色方法的实现
void GridCell::setFillColor(const sf::Color &color) // <-- 注意这里！
{
    this->fillColor = color;   // 更新私有成员 fillColor
    shape.setFillColor(color); // 同时更新 sf::RectangleShape 的填充颜色
}

// 设置格子的描边颜色方法的实现
void GridCell::setOutlineColor(const sf::Color &color) // <-- 注意这里！
{
    this->outlineColor = color;   // 更新私有成员 outlineColor
    shape.setOutlineColor(color); // 同时更新 sf::RectangleShape 的描边颜色
}

// 设置格子是否被选中方法的实现
void GridCell::setSelected(bool selected) // <-- 注意这里！
{
    m_isSelected = selected; // 更新私有成员 m_isSelected
    // 可以在这里根据选中状态即时改变 shape 的外观
    if (m_isSelected)
    {
        shape.setOutlineColor(LE::COLORS[19]);
        shape.setOutlineThickness(11);
        shape.setFillColor(LE::COLORS[17]);
        shape.setScale({0.8, 0.8});
    }
    else
    {
        shape.setOutlineColor(this->outlineColor); // 未选中时恢复为原始边框颜色
        shape.setOutlineThickness(GRID_BORDER);
        shape.setFillColor(this->fillColor); // 未选中时恢复为原始填充颜色
        shape.setScale({1.0, 1.0}); // 恢复到初始大小
    }
}

// 判断格子是否被选中方法的实现
bool GridCell::isSelected() const // <-- 注意这里！
{
    return m_isSelected; // 返回私有成员 m_isSelected 的值
}

// 设置格子位置（像素坐标，中心点）
void GridCell::setPosition(sf::Vector2f position)
{
    shape.setPosition(position);
}

// 设置格子尺寸
void GridCell::setSize(float size)
{
    sideLength = static_cast<int>(size);
    shape.setSize({size, size});
    shape.setOrigin({size / 2.f, size / 2.f});
}
