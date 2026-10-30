#pragma once

#include <Eigen/Core>
#include <memory>

namespace grid_canvas
{
    using Rect = Eigen::Vector4d;
    using Point3D = Eigen::Vector3d;
    using Point2D = Eigen::Vector2d;
    using ScreenPoint = Eigen::Vector2i;

class GridBlock
{
public:
    GridBlock();
    ~GridBlock();
    void zoom();
    void draw();

private:
    Rect area;
    GridBlock* parent = nullptr;
    std::unique_ptr<GridBlock> leftTop;
    std::unique_ptr<GridBlock> leftBottom;
    std::unique_ptr<GridBlock> rightTop;
    std::unique_ptr<GridBlock> rightBottom;
};

class GridCanvas
{
public:
    GridCanvas(int _width, int _height)
    {
        this->width = _width;
        this->height = _height;
        this->pos = ScreenPoint(0, 0);
        this->zoom = 1;
    };
    void setZoom(float _zoom)
    {
        this->zoom = std::max(_zoom, 0.0f);
    };

    void update()
    {

    };
    float getZoom()
    {
        return this->zoom;
    }
    int getWidth() const
    {
        return this->width;
    };
    int getHeight() const
    {
        return this->height;
    };
    GridBlock* head;
    Rect window;
    ScreenPoint pos;
    std::vector<Point2D> map;

private:
    int width, height;
    float zoom;
    int blockSize;
};
};
