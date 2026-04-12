#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "entities/Ball.hpp"

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // PreviewBar - 预告栏组件
    // 显示即将生成的球的颜色预览（复用 Ball 类的绘制逻辑）
    // ═══════════════════════════════════════════════════════════
    class PreviewBar
    {
    public:
        PreviewBar() = default;
        
        // 初始化预告栏
        void init(float x, float y, float cellSize, float gap, int count = 3);
        
        // 设置要显示的颜色（会更新/重建 Ball 对象）
        void setColors(const std::vector<BallColor>& colors);
        
        // 更新动画（调用各个 Ball 的更新）
        void update(float deltaTime);
        
        // 绘制预告栏（复用 Ball::draw）
        void draw(sf::RenderTarget& target) const;
        
        // 获取预告栏的边界
        sf::FloatRect getBounds() const;
        
    private:
        float m_x = 0.f;
        float m_y = 0.f;
        float m_cellSize = 40.f;
        float m_gap = 5.f;
        int m_count = 3;
        
        std::vector<sf::RectangleShape> m_cells;
        std::vector<std::unique_ptr<Ball>> m_balls;  // 使用真正的 Ball 对象
        std::vector<BallColor> m_colors;
        
        bool m_initialized = false;
    };
}
