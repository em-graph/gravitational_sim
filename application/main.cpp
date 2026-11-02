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
#include <deque>
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
    constexpr double kSoften = 0.02;
};

// convert degree to radian
constexpr double deg2rad(double deg)
{
    return deg * M_PI / 180.0;
}

SDL_GraphicsClass* gv;

struct MassSample
{
    double x;
    double y;
    double gmass;
};

void evalFieldPhi(vector<PhiField>& field, const vector<RigidBody>& sources, double G)
{
    vector<MassSample> samples;
    samples.reserve(sources.size());
    for (const auto& src : sources)
        samples.push_back({src.x(), src.y(), G * src.getMass()});

    for (auto& gp : field)
    {
        const double x = gp.position.x();
        const double y = gp.position.y();
        double phi = 0.0;
        for (const auto& src : samples)
        {
            const double dx = x - src.x;
            const double dy = y - src.y;
            const double dist2 = dx * dx + dy * dy + kSoften * kSoften;
            phi -= src.gmass / std::sqrt(dist2);
        }
        gp.phi = phi;
    }
}

struct PhiColorRange
{
    double logMin = 0;
    double logMax = 1;
    bool ready = false;
};

void trackPhiRange(const vector<PhiField>& field, PhiColorRange& range)
{
    double absPhiMax = 1e-20;
    double absPhiMin = 1e20;
    for (const auto& gp : field)
    {
        const double absPhi = std::max(std::fabs(gp.phi), 1e-12);
        absPhiMax = std::max(absPhiMax, absPhi);
        absPhiMin = std::min(absPhiMin, absPhi);
    }

    const double logMax = std::log(absPhiMax);
    const double logMin = std::log(absPhiMin);
    if (!range.ready)
    {
        range.logMin = logMin;
        range.logMax = std::max(logMax, logMin + 1e-3);
        range.ready = true;
        return;
    }

    constexpr double follow = 0.02;
    range.logMin += (logMin - range.logMin) * follow;
    range.logMax += (logMax - range.logMax) * follow;
    if (range.logMax < range.logMin + 1e-3)
        range.logMax = range.logMin + 1e-3;
}

int shadePotential(double phi, const PhiColorRange& range)
{
    const double span = range.logMax - range.logMin;
    if (span < 1e-12)
        return 500;
    const double absPhi = std::max(std::fabs(phi), 1e-12);
    const double t = 1000.0 * (std::log(absPhi) - range.logMin) / span;
    return static_cast<int>(std::clamp(t, 0.0, 1000.0));
}

vector<uint32_t> buildFieldColors(const vector<PhiField>& field, int cols, int rows, PhiColorRange& range)
{
    vector<uint32_t> colors(static_cast<size_t>(cols) * static_cast<size_t>(rows), 0xFF000000);
    if (field.empty() || cols <= 0 || rows <= 0)
        return colors;

    trackPhiRange(field, range);
    for (int ix = 0; ix < cols; ++ix)
    {
        for (int iy = 0; iy < rows; ++iy)
        {
            const double phi = field[static_cast<size_t>(ix * rows + iy)].phi;
            const int t = shadePotential(phi, range);
            const uint16_t wl = static_cast<uint16_t>(380 + t * 400 / 1000);
            colors[static_cast<size_t>(iy * cols + ix)] = color_spectrum(wl).full;
        }
    }
    return colors;
}

Point2D gravityAccel(const vector<RigidBody>& bodies, size_t index, double G, double soften2)
{
    Point2D accel = Point2D::Zero();
    const Point2D origin = bodies[index].getPosition();
    for (size_t j = 0; j < bodies.size(); ++j)
    {
        if (j == index)
            continue;
        const Point2D delta = bodies[j].getPosition() - origin;
        const double dist2 = delta.squaredNorm() + soften2;
        const double inv = G * bodies[j].getMass() / (dist2 * std::sqrt(dist2));
        accel += delta * inv;
    }
    return accel;
}

void stepGravity(vector<RigidBody>& bodies, double dt, double G)
{
    const double soften2 = kSoften * kSoften;
    vector<Point2D> accel(bodies.size());
    for (size_t i = 0; i < bodies.size(); ++i)
        accel[i] = gravityAccel(bodies, i, G, soften2);

    const double halfDt2 = 0.5 * dt * dt;
    for (size_t i = 0; i < bodies.size(); ++i)
    {
        const Point2D next = bodies[i].getPosition() + bodies[i].getVelocity() * dt + accel[i] * halfDt2;
        bodies[i].setPosition(next);
    }

    for (size_t i = 0; i < bodies.size(); ++i)
    {
        const Point2D accelNext = gravityAccel(bodies, i, G, soften2);
        const Point2D next = bodies[i].getVelocity() + (accel[i] + accelNext) * (0.5 * dt);
        bodies[i].setVelocity(next);
    }
}

bool insideBoard(const Point2D& p)
{
    return p.x() >= 0.0 && p.x() <= 1.0 && p.y() >= 0.0 && p.y() <= 1.0;
}

void launchAroundCenter(vector<RigidBody>& bodies, size_t index, double G)
{
    Point2D com = Point2D::Zero();
    double totalMass = 0.0;
    for (const auto& body : bodies)
    {
        com += body.getPosition() * body.getMass();
        totalMass += body.getMass();
    }
    if (totalMass <= 0.0 || index >= bodies.size())
        return;
    com /= totalMass;

    auto& body = bodies[index];
    const Point2D rel = body.getPosition() - com;
    const double radius = rel.norm();
    if (radius < 1e-4)
    {
        body.setVelocity(Point2D::Zero());
        return;
    }
    const double others = std::max(totalMass - body.getMass(), 0.0);
    const double orbitR = std::max(radius, kSoften);
    const double speed = std::sqrt(G * others / orbitR);
    const Point2D tangent(-rel.y() / radius, rel.x() / radius);
    body.setVelocity(tangent * speed);
}

void assignCircularVelocity(vector<RigidBody>& bodies, double G)
{
    for (size_t i = 0; i < bodies.size(); ++i)
        launchAroundCenter(bodies, i, G);
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

    auto canvas = std::make_unique<GridCanvas>(height, height, 0.01);

    int mx, my;
    Point2D last_point(0, 0);
    const double G = 1.0; // 这里使用无量纲G，不用真实SI，方便可视化
    const double step = 0.0001; // 每个 timestamp 对应的积分步长
    uint64_t timestamp = 0;

    const auto mapW = static_cast<double>(canvas->getWidth());
    const auto mapH = static_cast<double>(canvas->getHeight());
    cout << mapH << ", " <<mapH << endl;
    int fieldCols = 0;
    int fieldRows = 0;
    if (canvas->block_size > 0.0)
    {
        std::vector<double> xs;
        std::vector<double> ys;
        for (double x = 0; x < 1.0; x += canvas->block_size)
            xs.push_back(x);
        for (double y = 0; y < 1.0; y += canvas->block_size)
            ys.push_back(y);
        fieldCols = static_cast<int>(xs.size());
        fieldRows = static_cast<int>(ys.size());
        canvas->field.reserve(xs.size() * ys.size());
        for (double x : xs)
        {
            for (double y : ys)
                canvas->field.emplace_back(x, y);
        }
    }

    //Generate rand seed
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rand_pos(0,  1.0f);
    std::uniform_real_distribution<float> rand_size(1.0,  10.0f);
    std::uniform_real_distribution<float> rand_mass(0.1,  10.0f);

    for (int t = 0; t < 10; t++)
    {
        canvas->body.emplace_back(rand_pos(gen), rand_pos(gen), rand_size(gen), rand_mass(gen));
    }
    assignCircularVelocity(canvas->body, G);

    std::vector<std::deque<Point2D>> trails(canvas->body.size());
    for (size_t i = 0; i < canvas->body.size(); ++i)
        trails[i].push_back(canvas->body[i].getPosition());

    std::vector<uint32_t> fieldColor;
    PhiColorRange phiRange;

    while (running)
    {
        timestamp += 1;
        gv->clean();
        const Uint32 buttons = SDL_GetMouseState(&mx, &my);
        Point2D press(mx, my);

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
        stepGravity(canvas->body, step, G);
        std::vector<size_t> respawned;
        for (size_t i = 0; i < canvas->body.size(); ++i)
        {
            if (insideBoard(canvas->body[i].getPosition()))
                continue;
            canvas->body[i].setPosition(Point2D(rand_pos(gen), rand_pos(gen)));
            trails[i].clear();
            respawned.push_back(i);
        }
        for (size_t index : respawned)
            launchAroundCenter(canvas->body, index, G);
        for (size_t i = 0; i < canvas->body.size(); ++i)
        {
            trails[i].push_back(canvas->body[i].getPosition());
            if (trails[i].size() > 240)
                trails[i].pop_front();
        }
        evalFieldPhi(canvas->field, canvas->body, G);
        fieldColor = buildFieldColors(canvas->field, fieldCols, fieldRows, phiRange);
        canvas->update();

        const double zoom = canvas->getZoom();
        const double scale_w = zoom * mapW;
        const double scale_h = zoom * mapH;

        const double x0 = canvas->pos.x() + drag_x;
        const double y0 = canvas->pos.y() + drag_y;

        gv->blit_bilinear(fieldColor.data(), fieldCols, fieldRows,
                         static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)),
                         static_cast<int>(std::lround(scale_w)), static_cast<int>(std::lround(scale_h)));

        gv->set_color(color_lux(COLOR_SILVER));
        for (const auto& trail : trails)
        {
            for (size_t i = 1; i < trail.size(); ++i)
            {
                const int x1 = static_cast<int>(std::lround(x0 + trail[i - 1].x() * scale_w));
                const int y1 = static_cast<int>(std::lround(y0 + trail[i - 1].y() * scale_h));
                const int x2 = static_cast<int>(std::lround(x0 + trail[i].x() * scale_w));
                const int y2 = static_cast<int>(std::lround(y0 + trail[i].y() * scale_h));
                gv->draw_line(x1, y1, x2, y2);
            }
        }

        gv->set_color(color_lux(COLOR_WHITE));
        for (const auto& p : canvas->body)
        {
            const int pos_x = static_cast<int>(std::lround(x0 + p.x() * scale_w));
            const int pos_y = static_cast<int>(std::lround(y0 + p.y() * scale_h));
            gv->fill_circle(pos_x, pos_y, 2);
        }

        gv->set_color(color_lux(COLOR_GRAY));
        gv->draw_rect(static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)),
                      static_cast<int>(std::lround(scale_w)), static_cast<int>(std::lround(scale_h)));

        char hud[96];
        std::snprintf(hud, sizeof(hud), "pos: (%d %d) zoom: %.2f t: %llu",
                      static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)), canvas->getZoom(),
                      static_cast<unsigned long long>(timestamp));
        gv->draw_text(8, 8, hud, color_lux(COLOR_WHITE), 2);
        gv->update();

        SDL_Delay(1);
    }
    delete gv;
    return 0;
}
