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
using namespace sdl_graphics;

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

SDL_GraphicsClass* gv;

double calcGravPotential(const Point2D& r, const vector<RigidBody>& sources, double G)
{
    double total_phi = 0.0;
    const double eps = 1e-6; // avoid division by zero singularity

    for (const auto& src : sources)
    {
        Point2D dr = r - src.getPosition();
        double dist = dr.norm();
        if (dist < eps) dist = eps;
        total_phi += (-G * src.getMass()) / dist;
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
    gv->set_color(color_lux(COLOR_WHITE));

    SDL_Event event;
    bool running = true;
    bool is_press = false;

    auto canvas = std::make_unique<GridCanvas>(height, height, 0.05);

    int mx, my;
    Point2D last_point(0, 0);
    const double G = 1.0; // 这里使用无量纲G，不用真实SI，方便可视化
    uint64_t timestamp = 0;

    const auto mapW = static_cast<double>(canvas->getWidth());
    const auto mapH = static_cast<double>(canvas->getHeight());
    cout << mapH << ", " <<mapH << endl;
    for (double x = 0; x < 1.0; x += canvas->block_size)
    {
        for (double y = 0; y < 1.0; y += canvas->block_size)
        {
            canvas->field.emplace_back(x, y);
        }
    }

    //Generate rand seed
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist01(0,  1.0f);
    for (int t = 0; t < 100; t++)
    {
        canvas->body.emplace_back(dist01(gen), dist01(gen));
    }

    while (running)
    {
        timestamp += 1;
        gv->clean();
        const Uint32 buttons = SDL_GetMouseState(&mx, &my);
        Point2D press(mx, my);

        for(auto& gp : canvas->field)
        {
            gp.phi = calcGravPotential(gp.position, canvas->body, G);
        }

        static double drag_x = 0;
        static double drag_y = 0;
        // 鼠标画线逻辑不变
        if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            drag_x += (press.x() - last_point.x());
            drag_y += (press.y() - last_point.y());
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
                    const double oldZoom = canvas->getZoom();
                    const double newZoom = std::max(oldZoom + event.wheel.y * oldZoom / 100, 0.0);
                    const double oldW = oldZoom * mapW;
                    const double oldH = oldZoom * mapH;
                    if (oldW > 1e-4f && oldH > 1e-4f)
                    {
                        const double x0 = canvas->pos.x() + drag_x;
                        const double y0 = canvas->pos.y() + drag_y;
                        const double u = (press.x() - x0) / oldW;
                        const double v = (press.y() - y0) / oldH;
                        drag_x = press.x() - u * newZoom * mapW - canvas->pos.x();
                        drag_y = press.y() - v * newZoom * mapH - canvas->pos.y();
                    }
                    canvas->setZoom(newZoom);
                }
                break;
            case SDL_KEYDOWN:
                std::cout << "Key pressed: " << static_cast<char>(event.key.keysym.sym) << std::endl;
                break;
            default:
                break;
            }
        }
        canvas->update();

        const double zoom = canvas->getZoom();
        const double scale_w = zoom * mapW;
        const double scale_h = zoom * mapH;

        const double x0 = canvas->pos.x() + drag_x;
        const double y0 = canvas->pos.y() + drag_y;

        gv->set_color(color_lux(COLOR_PURPLE));
        for (auto p : canvas->field)
        {
            const int pos_x = static_cast<int>(std::lround(x0 + p.position.x() * scale_w));
            const int pos_y = static_cast<int>(std::lround(y0 + p.position.y() * scale_h));
            int spectrum = std::lround(p.phi);
            gv->set_pixel(pos_x, pos_y, color_spectrum(-spectrum));
        }

        gv->set_color(color_lux(COLOR_WHITE));
        for (auto p : canvas->body)
        {
            const int pos_x = static_cast<int>(std::lround(x0 + p.x() * scale_w));
            const int pos_y = static_cast<int>(std::lround(y0 + p.y() * scale_h));
            gv->set_pixel(pos_x, pos_y);
        }

        gv->set_color(color_lux(COLOR_GRAY));
        gv->draw_rect(static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)),
                      static_cast<int>(std::lround(scale_w)), static_cast<int>(std::lround(scale_h)));

        char hud[64];
        std::snprintf(hud, sizeof(hud), "pos: (%d %d) zoom: %f",
                      static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)), canvas->getZoom());
        gv->draw_text(8, 8, hud, color_lux(COLOR_WHITE), 2);
        gv->update();

        SDL_Delay(1);
    }
    delete gv;
    return 0;
}
