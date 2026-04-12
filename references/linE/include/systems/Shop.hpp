#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include <string>
#include <memory>
#include "board/ShopGrid.hpp"

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // 商品类型枚举
    // ═══════════════════════════════════════════════════════════
    enum class ItemType
    {
        // 彩虹球商品
        RAINBOW_PLACE,      // 直接放置一个彩虹球 (50分)
        RAINBOW_PREVIEW,    // 将预告区全部变为彩虹球 (100分)
        
        // 预留商品
        BOMB,           // 炸弹：消除指定区域
        COLOR_BRUSH,    // 颜色刷：改变球的颜色
        UNDO,           // 撤销：回退一步
        SHUFFLE,        // 洗牌：重新排列棋盘
        HINT,           // 提示：显示可消除位置
        EXTRA_MOVE,     // 额外移动：不生成新球
        
        Count
    };
    
    // 彩虹球商品价格
    static constexpr int RAINBOW_PLACE_PRICE = 50;    // 直接放置价格
    static constexpr int RAINBOW_PREVIEW_PRICE = 100; // 预告转换价格
    
    // ═══════════════════════════════════════════════════════════
    // 商品数据结构 (预留，后续扩展)
    // ═══════════════════════════════════════════════════════════
    struct ShopItem
    {
        ItemType type;
        std::string name;
        std::string description;
        int price = 0;
        int quantity = 0;      // 玩家拥有数量
        bool unlocked = true;  // 是否解锁
    };
    
    // ═══════════════════════════════════════════════════════════
    // 商店面板类
    // ═══════════════════════════════════════════════════════════
    class Shop
    {
    public:
        Shop();
        
        // 初始化商店 (需要字体引用)
        void init(const sf::Font& font);
        
        // 显示/隐藏商店
        void show();
        void hide();
        bool isVisible() const { return m_isVisible; }
        sf::FloatRect getBounds() const;
        
        // 更新商店状态 (鼠标悬停)
        void update(float deltaTime);
        void handleMouseMove(sf::Vector2f mousePos);
        
        // 处理点击事件，返回 true 表示事件被消费
        bool handleClick(sf::Vector2f mousePos);
        
        // 绘制商店界面
        void draw(sf::RenderWindow& window) const;
        
        // 分数管理（作为货币）
        void setScore(int score) { m_score = score; updateLayout(); }
        int getScore() const { return m_score; }
        bool spendScore(int amount);
        
        // 设置关闭回调
        void setOnClose(std::function<void()> callback) { m_onClose = callback; }
        
        // 设置购买回调 (预留)
        void setOnPurchase(std::function<void(ItemType)> callback) { m_onPurchase = callback; }
        
    private:
        // 创建UI元素
        void createUI();
        void updateLayout();
        
        // 状态
        bool m_isVisible = false;
        bool m_initialized = false;
        int m_score = 0;  // 玩家分数（作为货币）
        
        // 字体引用
        const sf::Font* m_font = nullptr;
        
        // 回调函数
        std::function<void()> m_onClose;
        std::function<void(ItemType)> m_onPurchase;
        
        // ═══════════════════════════════════════════════════════════
        // UI 元素 (使用unique_ptr延迟初始化sf::Text)
        // ═══════════════════════════════════════════════════════════
        
        // 遮罩层
        sf::RectangleShape m_overlay;
        
        // 主面板
        sf::RectangleShape m_panel;
        sf::RectangleShape m_panelBorder;
        
        // 标题栏
        sf::RectangleShape m_titleBar;
        std::unique_ptr<sf::Text> m_titleText;
        
        // 货币显示区
        sf::RectangleShape m_coinsBg;
        std::unique_ptr<sf::Text> m_coinsLabel;
        std::unique_ptr<sf::Text> m_coinsValue;
        sf::CircleShape m_coinIcon;
        
        // 商品区域（使用 ShopGrid）
        ShopGrid m_shopGrid;
        
        // 彩虹球商品1：直接放置 (50分)
        sf::RectangleShape m_rainbowPlaceBtn;
        std::unique_ptr<sf::Text> m_rainbowPlaceTitle;
        std::unique_ptr<sf::Text> m_rainbowPlaceDesc;
        std::unique_ptr<sf::Text> m_rainbowPlacePrice;
        sf::CircleShape m_rainbowPlaceIcon;
        bool m_rainbowPlaceHovered = false;
        
        // 彩虹球商品2：预告转换 (100分)
        sf::RectangleShape m_rainbowPreviewBtn;
        std::unique_ptr<sf::Text> m_rainbowPreviewTitle;
        std::unique_ptr<sf::Text> m_rainbowPreviewDesc;
        std::unique_ptr<sf::Text> m_rainbowPreviewPrice;
        sf::CircleShape m_rainbowPreviewIcon;
        std::unique_ptr<sf::Text> m_rainbowPreviewBadge;  // x3角标
        bool m_rainbowPreviewHovered = false;
        
        // 商品格子背景
        sf::RectangleShape m_itemCell1;
        sf::RectangleShape m_itemCell2;
        
        // 提示文字
        std::unique_ptr<sf::Text> m_hintText;
        float m_hintTimer = 0.f;
        bool m_showHint = false;
        
        // 分隔线
        sf::RectangleShape m_separator;
        
        // 关闭按钮
        sf::RectangleShape m_closeButton;
        std::unique_ptr<sf::Text> m_closeButtonText;
        bool m_closeButtonHovered = false;
        
        // 商品列表 (预留)
        std::vector<ShopItem> m_items;
        
    public:
        // 显示提示文字
        void showHint(const std::string& message);
        void updateHint(float deltaTime);
    };
}

