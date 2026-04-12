#pragma once
#include <cstdint>

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // Window dimensions
    // ═══════════════════════════════════════════════════════════
    static constexpr int WINDOW_WIDTH = 735;
    static constexpr int WINDOW_HEIGHT = 525;
    static constexpr int WINDOW_BORDER = 20;
    
    // 背景区域比例 (主区域 64%, 侧边栏 36%)
    static constexpr float MAIN_AREA_RATIO = 0.64f;
    static constexpr float SIDEBAR_RATIO = 0.36f;
    static constexpr int MAIN_AREA_WIDTH = static_cast<int>(WINDOW_WIDTH * MAIN_AREA_RATIO);
    static constexpr int MAIN_AREA_HEIGHT = WINDOW_HEIGHT;
    static constexpr int SIDEBAR_BG_WIDTH = WINDOW_WIDTH - MAIN_AREA_WIDTH;
    
    // ═══════════════════════════════════════════════════════════
    // Grid (game board) dimensions
    // ═══════════════════════════════════════════════════════════
    static constexpr int GRID_SIZE = 9;              // 9x9 grid
    static constexpr int GRID_SIDE = 44;
    static constexpr int GRID_BORDER = 5;
    static constexpr int GRID_LENGTH = GRID_SIDE + 2 * GRID_BORDER;
    static constexpr int FIRST_GRID_CENTER = WINDOW_BORDER + GRID_BORDER + GRID_SIDE / 2;
    static constexpr int TOTAL_GRID_LENGTH = GRID_BORDER * 2 * GRID_SIZE + GRID_SIDE * GRID_SIZE;
    
    // ═══════════════════════════════════════════════════════════
    // Ball dimensions
    // ═══════════════════════════════════════════════════════════
    static constexpr int BALL_RADIUS = 15;
    static constexpr int BALL_BORDER = 4;
    static constexpr int BALL_BORDER_SELECTED = 3;   // Extra border when selected
    
    // ═══════════════════════════════════════════════════════════
    // Selection breathing animation
    // ═══════════════════════════════════════════════════════════
    static constexpr float SELECTED_SCALE_MAX = 1.15f;   // Max scale when selected
    static constexpr float SELECTED_SCALE_MIN = 1.05f;   // Min scale when selected
    static constexpr float BREATH_SPEED = 2.0f;          // Breathing cycles per second
    
    // ═══════════════════════════════════════════════════════════
    // Color change animation
    // ═══════════════════════════════════════════════════════════
    static constexpr float COLOR_CHANGE_DURATION = 0.5f;  // seconds
    
    // ═══════════════════════════════════════════════════════════
    // Spawn (grow) animation
    // ═══════════════════════════════════════════════════════════
    static constexpr float SPAWN_DURATION = 0.75f;        // seconds (slower)
    
    // ═══════════════════════════════════════════════════════════
    // Ball movement animation (球移动动画)
    // ═══════════════════════════════════════════════════════════
    static constexpr float MOVE_SPEED_PER_CELL = 0.08f;   // 每格移动时间 (秒)
    
    // ═══════════════════════════════════════════════════════════
    // Angry bounce animation (愤怒跳动动画 - 无法到达时)
    // ═══════════════════════════════════════════════════════════
    static constexpr float ANGRY_BOUNCE_DURATION = 0.4f;  // 动画总时长 (秒)
    static constexpr float ANGRY_BOUNCE_HEIGHT = 8.0f;    // 跳动高度 (像素)
    
    // ═══════════════════════════════════════════════════════════
    // Chameleon ball priority system
    // ═══════════════════════════════════════════════════════════
    static constexpr int MAX_CHAMELEON_ITERATIONS = 81;   // Max iterations for color propagation
    static constexpr int PRIORITY_POSITION_WEIGHT = 10;   // Position weight in priority calculation
    
    // ═══════════════════════════════════════════════════════════
    // Line indicator animation (消除线指示器)
    // ═══════════════════════════════════════════════════════════
    static constexpr float LINE_INDICATOR_DURATION = 0.35f;   // 线出现动画时长 (秒)
    static constexpr float LINE_INDICATOR_THICKNESS = 8.0f;   // 线条粗细
    static constexpr uint8_t LINE_INDICATOR_ALPHA = 200;      // 线条透明度 (0-255)
    
    // ═══════════════════════════════════════════════════════════
    // Blob indicator animation (连通块轮廓指示器)
    // ═══════════════════════════════════════════════════════════
    static constexpr float BLOB_INDICATOR_DURATION = 0.4f;    // 轮廓出现动画时长 (秒)
    static constexpr float BLOB_INDICATOR_THICKNESS = 6.0f;   // 轮廓线粗细
    static constexpr float BLOB_INDICATOR_INSET = 4.0f;       // 轮廓内边距 (避免与深色边框重叠)
    static constexpr uint8_t BLOB_INDICATOR_ALPHA = 180;      // 轮廓透明度 (0-255)
    
    // ═══════════════════════════════════════════════════════════
    // Elimination phase timing (消除阶段时序 - 交错串行)
    // ═══════════════════════════════════════════════════════════
    // 线消阶段
    static constexpr float LINE_MARKING_DURATION = 0.3f;      // 线指示器展示时长
    static constexpr float LINE_FADING_DURATION = 0.45f;      // 线球消失后的间隔 (增大以拉开与块的间距)
    // 连通块阶段
    static constexpr float BLOB_MARKING_DURATION = 0.3f;      // 块指示器展示时长
    static constexpr float BLOB_FADING_DURATION = 0.6f;       // 块球消失时长
    
    // ═══════════════════════════════════════════════════════════
    // Score popup animation (计分弹窗动画)
    // ═══════════════════════════════════════════════════════════
    static constexpr float SCORE_POPUP_DURATION = 0.8f;       // 弹窗持续时长
    static constexpr float SCORE_POPUP_RISE = 30.0f;          // 上浮距离 (像素)
    static constexpr int SCORE_POPUP_FONT_SIZE = 24;          // 字体大小 (增大)
    // 金色 (RGB: 255, 215, 0 - 标准金色)
    static constexpr uint8_t SCORE_POPUP_COLOR_R = 255;
    static constexpr uint8_t SCORE_POPUP_COLOR_G = 200;
    static constexpr uint8_t SCORE_POPUP_COLOR_B = 50;
    
    // ═══════════════════════════════════════════════════════════
    // Swiss Style UI constants
    // ═══════════════════════════════════════════════════════════
    static constexpr int UI_UNIT = 8;
    static constexpr int UI_PADDING = 24;
    static constexpr int UI_SECTION_GAP = 24;
    static constexpr int UI_ELEMENT_GAP = 8;
    
    // Sidebar calculated values
    static constexpr int SIDEBAR_X = WINDOW_BORDER + TOTAL_GRID_LENGTH + UI_UNIT;
    static constexpr int SIDEBAR_WIDTH = WINDOW_WIDTH - SIDEBAR_X;
    static constexpr int PREVIEW_CELL_SIZE = 36;
    
    // ═══════════════════════════════════════════════════════════
    // Game Over UI constants (游戏结束界面)
    // ═══════════════════════════════════════════════════════════
    static constexpr uint8_t GAME_OVER_OVERLAY_ALPHA = 180;
    static constexpr int GAME_OVER_TITLE_SIZE = 48;
    static constexpr int GAME_OVER_SCORE_SIZE = 32;
    static constexpr int GAME_OVER_HINT_SIZE = 20;
    static constexpr float GAME_OVER_TITLE_Y = 180.f;
    static constexpr float GAME_OVER_SCORE_Y = 260.f;
    static constexpr float GAME_OVER_HINT_Y = 340.f;
    
    // ═══════════════════════════════════════════════════════════
    // Shop UI constants (商店界面)
    // ═══════════════════════════════════════════════════════════
    static constexpr int SHOP_PANEL_WIDTH = 400;
    static constexpr int SHOP_PANEL_HEIGHT = 450;
    static constexpr int SHOP_TITLE_HEIGHT = 50;
    static constexpr int SHOP_COINS_HEIGHT = 60;
    static constexpr int SHOP_BUTTON_HEIGHT = 40;
    static constexpr int SHOP_PADDING = 16;
    static constexpr int SHOP_ITEM_SIZE = 80;
    static constexpr int SHOP_ITEM_GAP = 12;
    static constexpr uint8_t SHOP_OVERLAY_ALPHA = 180;
    static constexpr int SHOP_TITLE_FONT_SIZE = 28;
    
    // ═══════════════════════════════════════════════════════════
    // Game initialization constants (游戏初始化)
    // ═══════════════════════════════════════════════════════════
    static constexpr int INITIAL_BALL_COUNT = 5;       // 初始球数量
    static constexpr int SPAWN_BALL_COUNT = 3;         // 每回合生成球数量
    static constexpr int PREVIEW_BALL_COUNT = 3;       // 预览球数量
}

