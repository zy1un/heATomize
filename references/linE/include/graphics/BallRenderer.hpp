#pragma once
#include <SFML/Graphics.hpp>
#include "entities/BallTypes.hpp"

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // BallRenderer - 统一的球渲染器
    // 用于在任何地方（棋盘、预告栏、商店等）渲染各种类型的球
    // ═══════════════════════════════════════════════════════════
    class BallRenderer
    {
    public:
        // 彩虹色调色板
        static const sf::Color RAINBOW_PALETTE[7];
        
        // 获取彩虹球当前颜色（基于相位）
        static sf::Color getRainbowColor(float phase);
        
        // 获取彩虹球描边颜色
        static sf::Color getRainbowOutline(const sf::Color& fill);
        
        // 更新全局彩虹相位（每帧调用一次）
        static void updateRainbowPhase(float deltaTime);
        
        // 获取当前全局彩虹相位
        static float getGlobalRainbowPhase();
        
        // 渲染球到CircleShape（设置颜色，不绘制）
        static void applyBallStyle(sf::CircleShape& shape, BallColor color, float phaseOffset = 0.0f);
        
        // 直接绘制球（便捷方法）
        static void drawBall(sf::RenderTarget& target, sf::Vector2f position, float radius, 
                            BallColor color, float phaseOffset = 0.0f);
        
    private:
        static float s_globalRainbowPhase;  // 全局彩虹相位 [0, 1)
        static constexpr float RAINBOW_SPEED = 0.2f;  // 约5秒一个周期（更快的变色）
    };
}
