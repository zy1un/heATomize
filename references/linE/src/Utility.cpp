#include<iostream>
#include"core/Defaults.hpp"
#include"Utility.hpp"

bool LE :: isInGrids(sf::Vector2f pixelPosition){
    if((pixelPosition.x < TOTAL_GRID_LENGTH+WINDOW_BORDER && pixelPosition.x >=WINDOW_BORDER) && (pixelPosition.y < TOTAL_GRID_LENGTH+WINDOW_BORDER && pixelPosition.y >=WINDOW_BORDER))
        return true;
    return false;
}

bool LE :: isInGrids(sf::Vector2i gridPosition){
    if((gridPosition.x < 9 && gridPosition.x >=0) && (gridPosition.y< 9 && gridPosition.y >=0))
        return true;
    return false;
}

sf::Vector2i LE :: pixelToGrid(sf::Vector2f pixelPosition){
    if (LE :: isInGrids(pixelPosition)){
        return{static_cast<int>((pixelPosition.x-WINDOW_BORDER)/GRID_LENGTH), static_cast<int>((pixelPosition.y-WINDOW_BORDER)/GRID_LENGTH)};
    }
    return {-1,-1};
}