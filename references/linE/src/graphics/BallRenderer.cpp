#include "graphics/BallRenderer.hpp"
#include "graphics/Color.hpp"
#include "core/Defaults.hpp"
#include <cmath>
#include <iostream>

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // 静态成员初始化
    // ═══════════════════════════════════════════════════════════
    
    float BallRenderer::s_globalRainbowPhase = 0.0f;
    
    // 7色彩虹调色板
    const sf::Color BallRenderer::RAINBOW_PALETTE[7] = {
        sf::Color(231, 76, 60),    // 红 #E74C3C
        sf::Color(230, 126, 34),   // 橙 #E67E22
        sf::Color(241, 196, 15),   // 黄 #F1C40F
        sf::Color(46, 204, 113),   // 绿 #2ECC71
        sf::Color(0, 188, 212),    // 青 #00BCD4
        sf::Color(52, 152, 219),   // 蓝 #3498DB
        sf::Color(155, 89, 182),   // 紫 #9B59B6
    };
    
    // ═══════════════════════════════════════════════════════════
    // 彩虹相位管理
    // ═══════════════════════════════════════════════════════════
    
    void BallRenderer::updateRainbowPhase(float deltaTime)
    {
        s_globalRainbowPhase += deltaTime * RAINBOW_SPEED;
        if (s_globalRainbowPhase >= 1.0f)
        {
            s_globalRainbowPhase -= 1.0f;
        }
    }
    
    float BallRenderer::getGlobalRainbowPhase()
    {
        return s_globalRainbowPhase;
    }
    
    // ═══════════════════════════════════════════════════════════
    // 彩虹颜色计算
    // ═══════════════════════════════════════════════════════════
    
    sf::Color BallRenderer::getRainbowColor(float phase)
    {
        // 确保相位在 [0, 1) 范围内
        while (phase < 0.0f) phase += 1.0f;
        while (phase >= 1.0f) phase -= 1.0f;
        
        float scaledPhase = phase * 7.0f;
        int idx1 = static_cast<int>(scaledPhase) % 7;
        int idx2 = (idx1 + 1) % 7;
        float t = scaledPhase - static_cast<int>(scaledPhase);
        
        // 平滑插值
        float smoothT = t * t * (3.0f - 2.0f * t);  // smoothstep
        
        return sf::Color(
            static_cast<uint8_t>(RAINBOW_PALETTE[idx1].r + (RAINBOW_PALETTE[idx2].r - RAINBOW_PALETTE[idx1].r) * smoothT),
            static_cast<uint8_t>(RAINBOW_PALETTE[idx1].g + (RAINBOW_PALETTE[idx2].g - RAINBOW_PALETTE[idx1].g) * smoothT),
            static_cast<uint8_t>(RAINBOW_PALETTE[idx1].b + (RAINBOW_PALETTE[idx2].b - RAINBOW_PALETTE[idx1].b) * smoothT)
        );
    }
    
    sf::Color BallRenderer::getRainbowOutline(const sf::Color& fill)
    {
        return sf::Color(
            static_cast<uint8_t>(fill.r * 0.7f),
            static_cast<uint8_t>(fill.g * 0.7f),
            static_cast<uint8_t>(fill.b * 0.7f)
        );
    }
    
    // ═══════════════════════════════════════════════════════════
    // 球样式应用
    // ═══════════════════════════════════════════════════════════
    
    void BallRenderer::applyBallStyle(sf::CircleShape& shape, BallColor color, float phaseOffset)
    {
        if (color == BallColor::Rainbow)
        {
            float phase = s_globalRainbowPhase + phaseOffset;
            sf::Color rainbowColor = getRainbowColor(phase);
            shape.setFillColor(rainbowColor);
            shape.setOutlineColor(getRainbowOutline(rainbowColor));
        }
        else if (color == BallColor::Chameleon)
        {
            shape.setFillColor(Chameleon::FILL_NORMAL);
            shape.setOutlineColor(Chameleon::OUTLINE_NORMAL);
        }
        else
        {
            int colorIdx = static_cast<int>(color) * 2;
            shape.setFillColor(COLORS[colorIdx]);
            shape.setOutlineColor(COLORS[colorIdx + 1]);
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // 便捷绘制方法
    // ═══════════════════════════════════════════════════════════
    
    void BallRenderer::drawBall(sf::RenderTarget& target, sf::Vector2f position, float radius,
                                BallColor color, float phaseOffset)
    {
        sf::CircleShape shape(radius);
        shape.setOrigin({radius, radius});
        shape.setPosition(position);
        shape.setOutlineThickness(BALL_BORDER);
        
        applyBallStyle(shape, color, phaseOffset);
        
        target.draw(shape);
    }
}
