#include "ui/PreviewBar.hpp"
#include "graphics/Color.hpp"
#include "core/Defaults.hpp"

namespace LE
{
    void PreviewBar::init(float x, float y, float cellSize, float gap, int count)
    {
        m_x = x;
        m_y = y;
        m_cellSize = cellSize;
        m_gap = gap;
        m_count = count;
        
        m_cells.clear();
        m_balls.clear();
        m_colors.clear();
        
        for (int i = 0; i < count; ++i)
        {
            float cellX = x + i * (cellSize + gap);
            float cellY = y;
            
            // 创建格子背景
            sf::RectangleShape cell({cellSize, cellSize});
            cell.setPosition({cellX, cellY});
            cell.setFillColor(sf::Color(0, 0, 0, 50));
            cell.setOutlineThickness(1.f);
            cell.setOutlineColor(COLORS[21]);
            m_cells.push_back(cell);
            
            // 创建 Ball 对象（使用默认颜色，后续会更新）
            // 使用虚拟的网格位置 (i, 0)，实际位置通过 setPixelPosition 设置
            auto ball = std::make_unique<Ball>(sf::Vector2i{i, 0}, BallColor::Red);
            ball->setPixelPosition({cellX + cellSize / 2.f, cellY + cellSize / 2.f});
            ball->finishSpawn();  // 跳过生成动画，直接显示完整大小
            m_balls.push_back(std::move(ball));
        }
        
        m_initialized = true;
    }
    
    void PreviewBar::setColors(const std::vector<BallColor>& colors)
    {
        m_colors = colors;
        
        // 更新每个 Ball 的颜色
        for (int i = 0; i < m_count && i < static_cast<int>(colors.size()); ++i)
        {
            if (m_balls[i])
            {
                m_balls[i]->setBallColor(colors[i]);
            }
        }
    }
    
    void PreviewBar::update(float deltaTime)
    {
        // 更新所有球的动画（彩虹球渐变等）
        for (auto& ball : m_balls)
        {
            if (ball)
            {
                ball->updateSpawn(deltaTime);  // 更新生成动画（如果有的话）
            }
        }
    }
    
    void PreviewBar::draw(sf::RenderTarget& target) const
    {
        if (!m_initialized) return;
        
        for (int i = 0; i < m_count; ++i)
        {
            // 绘制格子背景
            target.draw(m_cells[i]);
            
            // 绘制球（复用 Ball::draw）
            if (i < static_cast<int>(m_colors.size()) && m_balls[i])
            {
                m_balls[i]->draw(target);
            }
        }
    }
    
    sf::FloatRect PreviewBar::getBounds() const
    {
        float width = m_count * m_cellSize + (m_count - 1) * m_gap;
        return sf::FloatRect({m_x, m_y}, {width, m_cellSize});
    }
}
