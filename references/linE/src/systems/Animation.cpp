#include "systems/Animation.hpp"
#include "core/Defaults.hpp"
#include <cmath>

namespace LE
{
    std::unordered_map<Ball*, ChameleonAnimState> Animation::s_chameleonAnims;

    // ═══════════════════════════════════════════════════════════
    // 变色球动画管理
    // ═══════════════════════════════════════════════════════════
    
    void Animation::startColorChange(Ball* ball, sf::Color fromColor, sf::Color toColor)
    {
        if (!ball) return;
        
        auto& state = s_chameleonAnims[ball];
        
        // 如果正在动画中，从当前进度颜色开始
        if (state.isAnimating)
        {
            state.fromColor = lerpColor(state.fromColor, state.toColor, state.progress);
        }
        else
        {
            state.fromColor = fromColor;
        }
        
        state.toColor = toColor;
        state.progress = 0.f;
        state.isAnimating = true;
        state.duration = COLOR_CHANGE_DURATION;
    }
    
    void Animation::update(float deltaTime)
    {
        for (auto& [ball, state] : s_chameleonAnims)
        {
            if (state.isAnimating)
            {
                state.progress += deltaTime / state.duration;
                if (state.progress >= 1.0f)
                {
                    state.progress = 1.0f;
                    state.isAnimating = false;
                }
            }
        }
    }
    
    ChameleonAnimState* Animation::getAnimState(Ball* ball)
    {
        auto it = s_chameleonAnims.find(ball);
        if (it != s_chameleonAnims.end())
            return &it->second;
        return nullptr;
    }
    
    void Animation::removeBallAnim(Ball* ball)
    {
        s_chameleonAnims.erase(ball);
    }
    
    // ═══════════════════════════════════════════════════════════
    // 工具函数
    // ═══════════════════════════════════════════════════════════
    
    sf::Color Animation::lerpColor(const sf::Color& a, const sf::Color& b, float t)
    {
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        
        // ease-out 曲线
        float eased = 1.f - (1.f - t) * (1.f - t);
        
        return sf::Color(
            static_cast<uint8_t>(a.r + (b.r - a.r) * eased),
            static_cast<uint8_t>(a.g + (b.g - a.g) * eased),
            static_cast<uint8_t>(a.b + (b.b - a.b) * eased),
            static_cast<uint8_t>(a.a + (b.a - a.a) * eased)
        );
    }
    
    float Animation::getAnimScale(float progress)
    {
        // 缩放弹跳曲线:
        // 0.0 -> 1.0 (正常)
        // 0.3 -> 0.85 (收缩到最小)
        // 0.6 -> 1.1 (反弹过冲)
        // 1.0 -> 1.0 (回归正常)
        
        if (progress < 0.3f)
        {
            float t = progress / 0.3f;
            return 1.0f - 0.15f * Easing::smoothstep(t);
        }
        else if (progress < 0.6f)
        {
            float t = (progress - 0.3f) / 0.3f;
            return 0.85f + 0.25f * Easing::smoothstep(t);
        }
        else
        {
            float t = (progress - 0.6f) / 0.4f;
            return 1.1f - 0.1f * Easing::smoothstep(t);
        }
    }
}
