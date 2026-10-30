//
// Created by Administrator on 2026/10/28.
//
#include "GridCanvas.h"

namespace grid_canvas
{
    GridBlock::GridBlock()
    = default;
    GridBlock::~GridBlock()
    = default;

    void GridBlock::zoom()
    {
        this->leftBottom = nullptr;
    }

    void GridBlock::draw()
    {
        this->leftBottom = nullptr;
    }

};
