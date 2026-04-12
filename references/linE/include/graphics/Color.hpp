#pragma once
#include <SFML/Graphics/Color.hpp>
#include <array> // 需要包含 <array> 头文件

namespace LE
{
    // 定义一个静态的 constexpr std::array 来存储所有颜色
    // 数组的大小固定为 22，因为你的颜色索引从 0 到 21
    static constexpr std::array<sf::Color, 22> COLORS = {
        sf::Color(220, 60, 80),   // 0: Red Light (Ruby)
        sf::Color(160, 20, 40),   // 1: Red Dark (Crimson)
        sf::Color(230, 140, 60),  // 2: Orange Light (Amber)
        sf::Color(180, 90, 20),   // 3: Orange Dark (Bronze)
        sf::Color(255, 191, 0),   // 4: Yellow Light (Deep Gold)
        sf::Color(184, 134, 11),  // 5: Yellow Dark (Dark Goldenrod)
        sf::Color(46, 180, 100),  // 6: Green Light (Emerald)
        sf::Color(20, 100, 50),   // 7: Green Dark (Forest)
        sf::Color(64, 224, 208),  // 8: Cyan Light (Turquoise)
        sf::Color(0, 128, 128),   // 9: Cyan Dark (Teal)
        sf::Color(100, 149, 237), // 10: Blue Light (Sapphire)
        sf::Color(25, 25, 112),   // 11: Blue Dark (Navy)
        sf::Color(153, 50, 204),  // 12: Purple Light (Dark Orchid)
        sf::Color(75, 0, 130),    // 13: Purple Dark (Indigo)
        sf::Color(192, 192, 192), // 14: Silver
        sf::Color(1, 1, 1),       // 15: Black
        sf::Color(48, 44, 40),    // 16: UI Background (Deep Mocha)
        sf::Color(58, 58, 58, 128), // 17: UI Overlay
        sf::Color(245, 240, 235), // 18: UI Text (Off White)
        sf::Color(160, 130, 90),  // 19: Shadow
        sf::Color(0, 0, 0),       // 20: Black
        sf::Color(194, 168, 120)  // 21: UI Accent (Antique Gold)
    };
    // 辅助常量，表示颜色总数，方便后续扩展或循环
    static constexpr size_t ColorCount = COLORS.size();
    
    // ═══════════════════════════════════════════════════════════
    // 变色球专用颜色（避免硬编码）
    // ═══════════════════════════════════════════════════════════
    namespace Chameleon
    {
        // 未染色状态
        static constexpr sf::Color FILL_NORMAL = sf::Color(220, 220, 230, 180);    // 银白半透明
        static constexpr sf::Color OUTLINE_NORMAL = sf::Color(120, 120, 140, 255); // 深银边框
        
        // 选中状态
        static constexpr sf::Color FILL_SELECTED = sf::Color(180, 180, 195, 255);  // 银灰不透明
        static constexpr sf::Color OUTLINE_SELECTED = sf::Color(255, 255, 255, 255); // 亮白边框
        
        // 透明度
        static constexpr uint8_t DYED_ALPHA = 200; // 染色后的透明度
    }
}
