#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <cmath>
#include "core/Defaults.hpp"

// 前向声明
namespace LE { class Ball; }

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // 通用缓动函数 (Easing Functions)
    // ═══════════════════════════════════════════════════════════
    namespace Easing
    {
        // 线性
        inline float linear(float t) { return t; }
        
        // 正弦呼吸 (0→1→0 循环)
        inline float breathe(float phase) 
        { 
            return (std::sin(phase) + 1.0f) * 0.5f; 
        }
        
        // 平滑步进 (Smoothstep)
        inline float smoothstep(float t) 
        { 
            return t * t * (3.0f - 2.0f * t); 
        }
        
        // 缓出弹跳 (Ease-Out-Back)
        inline float easeOutBack(float t, float overshoot = 1.70158f)
        {
            float s = t - 1.0f;
            return s * s * ((overshoot + 1.0f) * s + overshoot) + 1.0f;
        }
        
        // 缓出弹性 (Ease-Out-Elastic)
        inline float easeOutElastic(float t)
        {
            if (t <= 0.0f) return 0.0f;
            if (t >= 1.0f) return 1.0f;
            float p = 0.3f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t - p/4.0f) * (2.0f * 3.14159f) / p) + 1.0f;
        }
        
        // 脉冲效果 (快速放大后缩回)
        inline float pulse(float t, float intensity = 0.3f)
        {
            return 1.0f + intensity * std::sin(t * 3.14159f);
        }
        
        // 映射值到范围
        inline float mapRange(float value, float minOut, float maxOut)
        {
            return minOut + value * (maxOut - minOut);
        }
    }
    
    // ═══════════════════════════════════════════════════════════
    // 变色球动画状态
    // ═══════════════════════════════════════════════════════════
    struct ChameleonAnimState
    {
        bool isAnimating = false;
        sf::Color fromColor;
        sf::Color toColor;
        float progress = 0.f;
        float duration = 0.5f;
    };
    
    // ═══════════════════════════════════════════════════════════
    // 动画管理器：负责变色球颜色过渡动画
    // ═══════════════════════════════════════════════════════════
    class Animation
    {
    public:
        // 变色球动画
        static void startColorChange(Ball* ball, sf::Color fromColor, sf::Color toColor);
        static void update(float deltaTime);
        static ChameleonAnimState* getAnimState(Ball* ball);
        static void removeBallAnim(Ball* ball);  // 清理已删除球的动画状态
        
        // 工具函数
        static sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t);
        static float getAnimScale(float progress);
        
    private:
        static std::unordered_map<Ball*, ChameleonAnimState> s_chameleonAnims;
    };
}
