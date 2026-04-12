#pragma once

namespace LE
{
    // 球的颜色枚举
    enum class BallColor
    {
        Red,        // 0 -> COLORS[0,1]
        Orange,     // 1 -> COLORS[2,3]
        Yellow,     // 2 -> COLORS[4,5]
        Green,      // 3 -> COLORS[6,7]
        Cyan,       // 4 -> COLORS[8,9]
        Blue,       // 5 -> COLORS[10,11]
        Purple,     // 6 -> COLORS[12,13]
        Chameleon,  // 7 -> 变色球（拟态周围颜色）
        Rainbow,    // 8 -> 彩虹球（万能匹配，动态渐变色）
        
        Count       // 9
    };

    // 球的状态枚举
    enum class BallState
    {
        Emerging,
        Sleep,
        Awaken,
        count
    };

    // 颜色枚举转字符串（用于调试）
    inline const char* ballColorToString(BallColor c)
    {
        switch (c)
        {
            case BallColor::Red: return "Red";
            case BallColor::Orange: return "Orange";
            case BallColor::Yellow: return "Yellow";
            case BallColor::Green: return "Green";
            case BallColor::Cyan: return "Cyan";
            case BallColor::Blue: return "Blue";
            case BallColor::Purple: return "Purple";
            case BallColor::Chameleon: return "Chameleon";
            case BallColor::Rainbow: return "Rainbow";
            default: return "Unknown";
        }
    }
}

