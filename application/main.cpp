#include <iostream>
#include "sdl_graphics.h"
#include "bresenham.h"
#include "bitmap_image.hpp"
#include <cmath>
#include <fstream>
#include "lodepng.h"
#include <thread>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <Eigen/Core>
#include <algorithm>
#include <cstdio>
#include <map>
#include <numbers> // C++20 for std::numbers::pi
#include "GridCanvas.h"
#include <random>

using namespace std;
using namespace Eigen;
using namespace grid_canvas;

using Point3D = Vector3d;
using Point2D = Vector2d;

namespace
{
    int width = 640;
    int height = 480;
    const double pi = 3.14159265358979323846;
};

// convert degree to radian
constexpr double deg2rad(double deg)
{
    return deg * M_PI / 180.0;
}

uint32_t spectrumGradientUint32(int t)
{
    t = std::clamp(t, 0, 1000);
    const double wave_nm = 380.0 + (static_cast<double>(t) / 1000.0) * (780.0 - 380.0);
    double r = 0.0, g = 0.0, b = 0.0;
    const double gamma = 0.8;

    if (wave_nm >= 380 && wave_nm < 440)
    {
        r = -(wave_nm - 440.0) / (440.0 - 380.0);
        g = 0.0;
        b = 1.0;
    }
    else if (wave_nm >= 440 && wave_nm < 490)
    {
        r = 0.0;
        g = (wave_nm - 440.0) / (490.0 - 440.0);
        b = 1.0;
    }
    else if (wave_nm >= 490 && wave_nm < 510)
    {
        r = 0.0;
        g = 1.0;
        b = -(wave_nm - 510.0) / (510.0 - 490.0);
    }
    else if (wave_nm >= 510 && wave_nm < 580)
    {
        r = (wave_nm - 510.0) / (580.0 - 510.0);
        g = 1.0;
        b = 0.0;
    }
    else if (wave_nm >= 580 && wave_nm < 645)
    {
        r = 1.0;
        g = -(wave_nm - 645.0) / (645.0 - 580.0);
        b = 0.0;
    }
    else if (wave_nm >= 645 && wave_nm <= 780)
    {
        r = 1.0;
        g = 0.0;
        b = 0.0;
    }

    double factor = 1.0;
    if (wave_nm > 700)
        factor = 0.3 + 0.7 * (780.0 - wave_nm) / (780.0 - 700.0);
    else if (wave_nm < 420)
        factor = 0.3 + 0.7 * (wave_nm - 380.0) / (420.0 - 380.0);

    auto applyGamma = [&](double val) -> uint8_t
    {
        if (val <= 0.0) return 0;
        return static_cast<uint8_t>(255.0 * std::pow(val * factor, gamma));
    };

    const uint8_t R = applyGamma(r);
    const uint8_t G = applyGamma(g);
    const uint8_t B = applyGamma(b);
    // return 0x00RRGGBB
    uint32_t color = (static_cast<uint32_t>(R) << 16) | (static_cast<uint32_t>(G) << 8) | B;
    return color;
}

SDL_GraphicsClass* gv;

class PointMassSource
{
public:
    PointMassSource(double x, double y, double mass)
    {
        pos = Point2D(x, y);
        this->mass = mass;
    }

    Point2D pos;
    double mass;
};

// 网格采样点：存储该点算出的引力势Phi
struct GridPoint
{
    Point2D pos;
    double phi; // gravitational potential Φ
};

double calcGravPotential(const Point2D& r, const vector<PointMassSource>& sources, double G)
{
    double total_phi = 0.0;
    const double eps = 1e-6; // avoid division by zero singularity

    for (const auto& src : sources)
    {
        Point2D dr = r - src.pos;
        double dist = dr.norm();
        if (dist < eps) dist = eps;
        total_phi += (-G * src.mass) / dist;
    }
    return total_phi;
}

int normalizePotentialLog(double phi, double absPhiMax, double absPhiMin)
{
    const double eps = 1e-12;
    double absPhi = std::fabs(phi);
    absPhi = std::max(absPhi, eps);

    double logMax = std::log(absPhiMax);
    double logMin = std::log(absPhiMin);

    if (logMax - logMin < 1e-12)
        return 500;

    double t = 1000.0 * (std::log(absPhi) - logMin) / (logMax - logMin);
    return static_cast<int>(std::clamp(t, 0.0, 1000.0));
}


int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    gv = new SDL_GraphicsClass(width, height, "引力势标量场");
    gv->init();
    gv->set_color(0xFFFFFFFF);

    SDL_Event event;
    bool running = true;
    bool is_press = false;

    auto canvas = std::make_unique<GridCanvas>(height, height);

    // ========== 引力源 ==========
    vector<PointMassSource> sources;
    // 中心放置引力质点，可修改位置和质量
    // sources.emplace_back(320.0, 240.0, 500000.0);
    // sources.emplace_back(220.0, 240.0, 1000000.0);

    // ========== 初始化网格 ==========
    // vector<GridPoint> grid;
    // grid.reserve(width * height);
    // for (int x = 0; x < width; x++)
    // {
    //     for (int y = 0; y < height; y++)
    //     {
    //         GridPoint gp;
    //         gp.pos = Point2D(x, y);
    //         gp.phi = 0.0;
    //         grid.emplace_back(gp);
    //     }
    // }

    int mx, my;
    Point2D last_point(0, 0);
    const double G = 1.0; // 这里使用无量纲G，不用真实SI，方便可视化
    uint64_t timestamp = 0;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> dist01(0,  1.0f);

    //Generate rand seed
    for (int t = 0; t < 100; t++)
    {
        canvas->map.emplace_back(dist01(gen), dist01(gen));
    }

    while (running)
    {
        timestamp += 1;
        gv->clean();
        const Uint32 buttons = SDL_GetMouseState(&mx, &my);
        Point2D press(mx, my);
        // sources[1].pos[0] = sin(timestamp * 1.0f * pi / 50.0f) * 100 + width / 2;
        // sources[1].pos[1] = cos(timestamp * 1.0f * pi / 50.0f) * 100 + height / 2;

        // // ====== 1. 计算全部网格引力势 ======
        // for(auto& gp : grid)
        // {
        //     gp.phi = calcGravPotential(gp.pos, sources, G);
        // }
        //
        // double absPhiMax = 1e-20;
        // double absPhiMin = 1e20;
        // for(auto& gp : grid)
        // {
        //     double absP = std::fabs(gp.phi);
        //     if(absP > absPhiMax) absPhiMax = absP;
        //     if(absP < absPhiMin) absPhiMin = absP;
        // }
        //
        // // ======3. 渲染网格，对数映射光谱颜色 ======
        // for(auto& gp : grid)
        // {
        //     int t = normalizePotentialLog(gp.phi, absPhiMax, absPhiMin);
        //     if (t % 10 != 0)
        //         continue;
        //
        //     uint32_t rgb = spectrumGradientUint32(t);
        //     uint32_t colorARGB = 0xFF000000 | rgb;
        //     sdl_set_pixel((int)gp.pos.x(), (int)gp.pos.y(), colorARGB);
        // }
        static float drag_x = 0;
        static float drag_y = 0;
        // 鼠标画线逻辑不变
        if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            drag_x += static_cast<float>(press.x() - last_point.x());
            drag_y += static_cast<float>(press.y() - last_point.y());
        }
        last_point = press;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_MOUSEWHEEL:
                {
                    const float oldZoom = canvas->getZoom();
                    const float newZoom = std::max(oldZoom + event.wheel.y * 0.01f, 0.0f);
                    const float mapW = static_cast<float>(canvas->getWidth());
                    const float mapH = static_cast<float>(canvas->getHeight());
                    const float oldW = oldZoom * mapW;
                    const float oldH = oldZoom * mapH;
                    if (oldW > 1e-4f && oldH > 1e-4f)
                    {
                        const float x0 = static_cast<float>(canvas->pos.x()) + drag_x;
                        const float y0 = static_cast<float>(canvas->pos.y()) + drag_y;
                        const float u = (static_cast<float>(press.x()) - x0) / oldW;
                        const float v = (static_cast<float>(press.y()) - y0) / oldH;
                        drag_x = static_cast<float>(press.x()) - u * newZoom * mapW - static_cast<float>(canvas->pos.x());
                        drag_y = static_cast<float>(press.y()) - v * newZoom * mapH - static_cast<float>(canvas->pos.y());
                    }
                    canvas->setZoom(newZoom);
                }
                break;
            case SDL_KEYDOWN:
                std::cout << "Key pressed: " << (char)event.key.keysym.sym << std::endl;
                break;
            default:
                break;
            }
        }
        canvas->update();
        
        float zoom = canvas->getZoom();
        float scale_w = zoom * static_cast<float>(canvas->getWidth());
        float scale_h = zoom * static_cast<float>(canvas->getHeight());

        float x0 = static_cast<float>(canvas->pos.x()) + drag_x;
        float y0 = static_cast<float>(canvas->pos.y()) + drag_y;

        for (auto p : canvas->map)
        {
            int pos_x = static_cast<int>(std::lround(x0 + p.x() * scale_w));
            int pos_y = static_cast<int>(std::lround(y0 + p.y() * scale_h));
            gv->set_pixel(pos_x, pos_y);
        }

        gv->draw_rect(static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)),
                      static_cast<int>(std::lround(scale_w)), static_cast<int>(std::lround(scale_h)));

        char hud[64];
        std::snprintf(hud, sizeof(hud), "pos: (%d %d) zoom: %f",
                      static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)), canvas->getZoom());
        gv->draw_text(8, 8, hud, SDL_GraphicsClass::rgb(255, 255, 255), 2);
        gv->update();

        SDL_Delay(1);
    }
    delete gv;
    return 0;
}
