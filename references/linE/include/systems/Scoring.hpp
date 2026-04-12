#pragma once
#include <string>
#include <sstream>
#include <vector>

namespace LE
{
    namespace Scoring
    {
        // ═══════════════════════════════════════════════════════════
        // 直线消除分数表 (非线性增长)
        // ═══════════════════════════════════════════════════════════
        
        inline int getLineScore(int length)
        {
            switch (length)
            {
                case 5:  return 15;
                case 6:  return 25;
                case 7:  return 50;
                case 8:  return 90;
                case 9:  return 150;
                default: return (length >= 9) ? 150 : 0;
            }
        }
        
        // ═══════════════════════════════════════════════════════════
        // 连通块消除分数 (线性，低回报)
        // ═══════════════════════════════════════════════════════════
        
        inline int getBlobScore(int count)
        {
            return count * 2;
        }
        
        // ═══════════════════════════════════════════════════════════
        // 交叉奖励 (多组消除同时发生)
        // ═══════════════════════════════════════════════════════════
        
        static constexpr int CROSS_BONUS = 20;
        
        // ═══════════════════════════════════════════════════════════
        // 连锁倍率 (波次越高，倍率越大)
        // ═══════════════════════════════════════════════════════════
        
        inline int getComboMultiplier(int wave)
        {
            if (wave <= 1) return 1;
            if (wave == 2) return 2;
            return 4;
        }
        
        // ═══════════════════════════════════════════════════════════
        // 详细日志辅助函数
        // ═══════════════════════════════════════════════════════════
        
        struct ScoreBreakdown
        {
            int baseScore = 0;
            int crossBonus = 0;
            int multiplier = 1;
            int finalScore = 0;
            std::string details;
        };
        
        inline ScoreBreakdown calculateScore(
            const std::vector<int>& lineLengths,
            const std::vector<int>& blobSizes,
            int leftoverBalls,
            int wave)
        {
            ScoreBreakdown result;
            std::ostringstream ss;
            bool hasContent = false;
            
            // 计算直线分数
            for (size_t i = 0; i < lineLengths.size(); ++i)
            {
                int len = lineLengths[i];
                int pts = getLineScore(len);
                result.baseScore += pts;
                if (hasContent) ss << " + ";
                ss << "Line" << len << ":" << pts;
                hasContent = true;
            }
            
            // 计算纯连通块分数
            for (size_t i = 0; i < blobSizes.size(); ++i)
            {
                int size = blobSizes[i];
                int pts = getBlobScore(size);
                result.baseScore += pts;
                if (hasContent) ss << " + ";
                ss << "Blob" << size << ":" << pts;
                hasContent = true;
            }
            
            // 计算散球分数 (每个2分)
            if (leftoverBalls > 0)
            {
                int pts = leftoverBalls * 2;
                result.baseScore += pts;
                if (hasContent) ss << " + ";
                ss << "Extra" << leftoverBalls << ":" << pts;
                hasContent = true;
            }
            
            // 交叉奖励：仅当有2条或以上直线时触发
            if (lineLengths.size() >= 2)
            {
                result.crossBonus = CROSS_BONUS;
                ss << " + Cross:" << CROSS_BONUS;
            }
            
            // 倍率
            result.multiplier = getComboMultiplier(wave);
            result.finalScore = (result.baseScore + result.crossBonus) * result.multiplier;
            result.details = ss.str();
            
            return result;
        }
    }
}
