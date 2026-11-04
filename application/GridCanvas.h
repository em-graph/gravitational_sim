#pragma once

#include <Eigen/Core>
#include <memory>

namespace grid_canvas
{
    using Rect = Eigen::Vector4d;
    using Point3D = Eigen::Vector3d;
    using Point2D = Eigen::Vector2d;
    using ScreenPoint = Eigen::Vector2i;
    using Point2I = Eigen::Vector2i;

    class PhiField
    {
    public:
        PhiField(double _x, double _y)
        {
            this->position = Point2D(_x, _y);
            this->phi = 0.0;
        }

        Point2D position;
        double phi = 0;
    };

class RigidBody
{
public:
    RigidBody(double _x, double _y)
    {
        this->position = Point2D(_x, _y);
        this->velocity = Point2D::Zero();
        this->size = 1.0;
        this->mass = 1.0;
        this->rotation = 0;
    }

    RigidBody(double _x, double _y, double _size)
    {
        this->position = Point2D(_x, _y);
        this->velocity = Point2D::Zero();
        this->size = _size;
        this->mass = 1.0;
        this->rotation = 0;
    }

    RigidBody(double _x, double _y, double _size, double _mass)
    {
        this->position = Point2D(_x, _y);
        this->velocity = Point2D::Zero();
        this->size = _size;
        this->mass = _mass;
        this->rotation = 0;
    }

    double getMass() const
    {
        return this->mass;
    }

    void setMass(double m)
    {
        this->mass = m;
    }

    // Radius in the same world unit as position. Here that unit is 1 AU.
    double getSize() const
    {
        return this->size;
    }

    void setSize(double radius)
    {
        this->size = radius;
    }

    Point2D getPosition() const
    {
        return this->position;
    }

    Point2D getVelocity() const
    {
        return this->velocity;
    }

    void setPosition(const Point2D& p)
    {
        this->position = p;
    }

    void setVelocity(const Point2D& v)
    {
        this->velocity = v;
    }

    double x() const
    {
        return this->position.x();
    }

    double y() const
    {
        return this->position.y();
    }

    double norm(RigidBody* body)
    {
        return (body->getPosition() - this->getPosition()).norm();
    }

    double squaredNorm(RigidBody* body)
    {
        return (body->getPosition() - this->getPosition()).squaredNorm();
    }

private:
    Point2D position = Point2D::Zero();
    Point2D velocity = Point2D::Zero();
    double size = 0;
    double rotation = 0;
    double mass = 0;
};


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
    GridCanvas(int _width, int _height, double _block_size)
    {
        this->width = _width;
        this->height = _height;
        this->pos = ScreenPoint(0, 0);
        this->zoom = 1;
        this->block_size = _block_size;
    }
    ;
    void setZoom(const double _zoom)
    {
        this->zoom = std::max(_zoom, 0.0);
    };

    void setOrigin(const int _x, const int _y)
    {
        this->pos = ScreenPoint(_x, _y);
    }

    void update()
    {

    };
    double getZoom()
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
    std::vector<PhiField> field;
    std::vector<RigidBody> body;
    double block_size;
private:
    int width, height;
    double zoom;
};
};
