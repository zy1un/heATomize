#pragma once
#include "board/GridSystem.hpp"
#include "entities/Ball.hpp"
#include <functional>
#include <memory>

namespace LE
{
    // ═══════════════════════════════════════════════════════════
    // 商品数据
    // ═══════════════════════════════════════════════════════════
    struct ShopItemData
    {
        int id = 0;
        std::string name;
        std::string description;
        int price = 0;
        BallColor displayColor = BallColor::Rainbow;  // 用球颜色表示商品
        bool isRainbow = true;    // 是否显示彩虹动画
        int quantity = 1;         // 购买数量（用于x3角标等）
        bool available = true;    // 是否可购买
    };
    
    // ═══════════════════════════════════════════════════════════
    // ShopGrid - 基于GridSystem的商店网格
    // ═══════════════════════════════════════════════════════════
    class ShopGrid : public GridSystem
    {
    public:
        ShopGrid();
        
        // 初始化商店网格
        void initShop(const sf::Font& font, float offsetX, float offsetY);
        
        // 设置商品数据
        void setItems(const std::vector<ShopItemData>& items);
        
        // 设置玩家分数（用于判断是否可购买）
        void setScore(int score);
        
        // 重写绘制（添加球和价格标签）
        void draw(sf::RenderTarget& target) const override;
        
        // 重写更新（球动画）
        void update(float deltaTime) override;
        void handleMouseMove(sf::Vector2f mousePos) override;
        
        // 设置购买回调
        void setOnPurchase(std::function<void(int itemId)> callback);
        
        // 获取指定位置的商品
        const ShopItemData* getItemAt(int col, int row) const;
        
        // 重写点击处理
        bool handleClick(sf::Vector2f mousePos) override;
        
    private:
        const sf::Font* m_font = nullptr;
        int m_score = 0;
        
        // 商品数据（按格子位置存储）
        std::vector<std::vector<ShopItemData>> m_items;
        
        // 预览球（用于商品展示）
        std::vector<std::vector<std::unique_ptr<Ball>>> m_displayBalls;
        
        // 价格标签
        std::vector<std::vector<std::unique_ptr<sf::Text>>> m_priceLabels;
        
        // 数量角标（如x3）
        std::vector<std::vector<std::unique_ptr<sf::Text>>> m_quantityBadges;
        
        // 购买回调
        std::function<void(int itemId)> m_onPurchase;
        
        // 辅助方法
        void createDisplayElements();
        void updateItemAvailability();
    };
}
