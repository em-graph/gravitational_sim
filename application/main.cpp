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
#include <ctime>

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
    for (size_t i = 1; i < bodies.size(); ++i)
        accel[i] = gravityAccel(bodies, i, G, soften2);

    const double halfDt2 = 0.5 * dt * dt;
    for (size_t i = 1; i < bodies.size(); ++i)
    {
        const Point2D next = bodies[i].getPosition() + bodies[i].getVelocity() * dt + accel[i] * halfDt2;
        bodies[i].setPosition(next);
    }

    for (size_t i = 1; i < bodies.size(); ++i)
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

struct CalendarDate
{
    int year = 2000;
    int month = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    int second = 0;
};

CalendarDate readClock(bool utc)
{
    const std::time_t now = std::time(nullptr);
    const std::tm* parts = utc ? std::gmtime(&now) : std::localtime(&now);
    if (parts == nullptr)
        return {};
    return {
        parts->tm_year + 1900,
        parts->tm_mon + 1,
        parts->tm_mday,
        parts->tm_hour,
        parts->tm_min,
        parts->tm_sec
    };
}

// Gregorian calendar to Julian Date. `day` includes the fraction of the UT day.
double julianDate(int year, int month, double day)
{
    const double y = static_cast<double>(year);
    const double m = static_cast<double>(month);
    return 367.0 * y
        - std::floor(7.0 * (y + std::floor((m + 9.0) / 12.0)) / 4.0)
        + std::floor(275.0 * m / 9.0)
        + day + 1721013.5;
}

double wrap180(double deg)
{
    deg = std::fmod(deg, 360.0);
    if (deg > 180.0)
        deg -= 360.0;
    else if (deg <= -180.0)
        deg += 360.0;
    return deg;
}

// JPL Keplerian elements, mean ecliptic and equinox of J2000.
// Rates are per Julian century. Valid about 1800–2050.
// a [AU], e, I/L/varpi/node [deg]. Pluto keeps the former JPL fit of the same kind.
struct KeplerOrbit
{
    const char* name;
    double a0, adot;
    double e0, edot;
    double i0, idot;
    double L0, Ldot;
    double w0, wdot;
    double o0, odot;
    double radiusKm;
    double massEarth;
};

// Heliocentric ecliptic coordinates in AU. x toward the vernal equinox, z toward ecliptic north.
Point3D heliocentricEcliptic(const KeplerOrbit& orbit, double century)
{
    const double a = orbit.a0 + orbit.adot * century;
    const double e = orbit.e0 + orbit.edot * century;
    const double incl = orbit.i0 + orbit.idot * century;
    const double meanLong = orbit.L0 + orbit.Ldot * century;
    const double peri = orbit.w0 + orbit.wdot * century;
    const double node = orbit.o0 + orbit.odot * century;

    const double mean = wrap180(meanLong - peri);
    const double eDeg = e * 180.0 / pi;
    double E = mean + eDeg * std::sin(deg2rad(mean));
    for (int n = 0; n < 30; ++n)
    {
        const double sinE = std::sin(deg2rad(E));
        const double cosE = std::cos(deg2rad(E));
        const double dE = (mean - (E - eDeg * sinE)) / (1.0 - e * cosE);
        E += dE;
        if (std::fabs(dE) < 1e-8)
            break;
    }

    const double cosE = std::cos(deg2rad(E));
    const double sinE = std::sin(deg2rad(E));
    const double xp = a * (cosE - e);
    const double yp = a * std::sqrt(std::max(0.0, 1.0 - e * e)) * sinE;

    const double arg = deg2rad(peri - node);
    const double Om = deg2rad(node);
    const double inc = deg2rad(incl);
    const double cosArg = std::cos(arg);
    const double sinArg = std::sin(arg);
    const double cosOm = std::cos(Om);
    const double sinOm = std::sin(Om);
    const double cosInc = std::cos(inc);
    const double sinInc = std::sin(inc);

    const double x = (cosArg * cosOm - sinArg * sinOm * cosInc) * xp
        + (-sinArg * cosOm - cosArg * sinOm * cosInc) * yp;
    const double y = (cosArg * sinOm + sinArg * cosOm * cosInc) * xp
        + (-sinArg * sinOm + cosArg * cosOm * cosInc) * yp;
    const double z = (sinArg * sinInc) * xp + (cosArg * sinInc) * yp;
    return Point3D(x, y, z);
}

constexpr double kAuKm = 149597870.7;
constexpr double kSunRadiusKm = 695700.0;
constexpr double kSunMassEarth = 332900.0;

// Screen y grows downward, so ecliptic +Y is flipped into world Y.
Point2D worldFromEcliptic(const Point3D& ecliptic, const Point2D& sun)
{
    return Point2D(sun.x() + ecliptic.x(), sun.y() - ecliptic.y());
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
    const double step = 0.000001; // 每个 timestamp 对应的积分步长
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

    const CalendarDate localNow = readClock(false);
    const CalendarDate utcNow = readClock(true);
    const double utcDay = utcNow.day
        + (utcNow.hour + utcNow.minute / 60.0 + utcNow.second / 3600.0) / 24.0;
    const double jd = julianDate(utcNow.year, utcNow.month, utcDay);
    const double century = (jd - 2451545.0) / 36525.0;

    // Elements: a0, adot, e0, edot, I0, Idot, L0, Ldot, varpi0, varpidot, node0, nodedot, radius km, Earth masses.
    const KeplerOrbit planets[] = {
        {"Mercury", 0.38709927,  0.00000037, 0.20563593,  0.00001906,  7.00497902, -0.00594749, 252.25032350, 149472.67411175,  77.45779628,  0.16047689,  48.33076593, -0.12534081,  2439.7,   0.055},
        {"Venus",   0.72333566,  0.00000390, 0.00677672, -0.00004107,  3.39467605, -0.00078890, 181.97909950,  58517.81538729, 131.60246718,  0.00268329,  76.67984255, -0.27769418,  6051.8,   0.815},
        {"Earth",   1.00000261,  0.00000562, 0.01671123, -0.00004392, -0.00001531, -0.01294668, 100.46457166,  35999.37244981, 102.93768193,  0.32327364,   0.0,         0.0,        6378.1,   1.000},
        {"Mars",    1.52371034,  0.00001847, 0.09339410,  0.00007882,  1.84969142, -0.00813131,  -4.55343205,  19140.30268499, -23.94362959,  0.44441088,  49.55953891, -0.29257343,  3396.2,   0.107},
        {"Jupiter", 5.20288700, -0.00011607, 0.04838624, -0.00013253,  1.30439695, -0.00183714,  34.39644051,   3034.74612775,  14.72847983,  0.21252668, 100.47390909,  0.20469106, 71492.0, 318.000},
        {"Saturn",  9.53667594, -0.00125060, 0.05386179, -0.00050991,  2.48599187,  0.00193609,  49.95424423,   1222.49362201,  92.59887831, -0.41897216, 113.66242448, -0.28867794, 60268.0,  95.200},
        {"Uranus", 19.18916464, -0.00196176, 0.04725744, -0.00004397,  0.77263783, -0.00242939, 313.23810451,    428.48202785, 170.95427630,  0.40805281,  74.01692503,  0.04240589, 25559.0,  14.500},
        {"Neptune",30.06992276,  0.00026291, 0.00859048,  0.00005105,  1.77004347,  0.00035372, -55.12002969,    218.45945325,  44.96476227, -0.32241464, 131.78422574, -0.00508664, 24764.0,  17.100},
        {"Pluto",  39.48211675, -0.00031596, 0.24882730,  0.00005170, 17.14001206,  0.00004818, 238.92903833,    145.20780515, 224.06891629, -0.04062942, 110.30393684, -0.01183482,  1188.3,   0.00218},
    };

    const Point2D sunPos(0.5, 0.5);
    canvas->body.emplace_back(sunPos.x(), sunPos.y(), kSunRadiusKm / kAuKm, kSunMassEarth);
    std::printf("%04d-%02d-%02d local  JD %.5f  heliocentric ecliptic (AU)\n",
                localNow.year, localNow.month, localNow.day, jd);
    for (const KeplerOrbit& planet : planets)
    {
        const Point3D ecliptic = heliocentricEcliptic(planet, century);
        const Point2D world = worldFromEcliptic(ecliptic, sunPos);
        canvas->body.emplace_back(world.x(), world.y(), planet.radiusKm / kAuKm, planet.massEarth);
        std::printf("  %-8s %+11.5f %+11.5f %+11.5f\n", planet.name, ecliptic.x(), ecliptic.y(), ecliptic.z());
    }

    assignCircularVelocity(canvas->body, G);
    canvas->body.front().setVelocity(Point2D::Zero());

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

        for (size_t i = 0; i < canvas->body.size(); ++i)
        {
            const auto& p = canvas->body[i];
            const int pos_x = static_cast<int>(std::lround(x0 + p.x() * scale_w));
            const int pos_y = static_cast<int>(std::lround(y0 + p.y() * scale_h));
            // World unit is 1 AU, so the pixel radius is the true radius at this zoom.
            const int radiusPx = std::max(0, static_cast<int>(std::lround(p.getSize() * scale_w)));
            gv->set_color(i == 0 ? color_lux(COLOR_YELLOW) : color_lux(COLOR_WHITE));
            gv->fill_circle(pos_x, pos_y, radiusPx);
        }

        // Static sun mark. Pixel size stays fixed; it sits on the canvas center.
        const int sunX = static_cast<int>(std::lround(x0 + 0.5 * scale_w));
        const int sunY = static_cast<int>(std::lround(y0 + 0.5 * scale_h));
        constexpr int kSunArm = 12;
        constexpr int kSunRing = 5;
        gv->set_color(color_lux(COLOR_WHITE));
        gv->draw_circle(sunX, sunY, kSunRing);
        gv->draw_line(sunX - kSunArm, sunY, sunX + kSunArm, sunY);
        gv->draw_line(sunX, sunY - kSunArm, sunX, sunY + kSunArm);

        gv->set_color(color_lux(COLOR_GRAY));
        gv->draw_rect(static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)),
                      static_cast<int>(std::lround(scale_w)), static_cast<int>(std::lround(scale_h)));

        float ek_total = 0;
        for (size_t i = 0; i < canvas->body.size(); ++i)
        {
            ek_total += canvas->body[i].getVelocity().squaredNorm() * canvas->body[i].getMass() / 2;
        }

        char hud[160];
        std::snprintf(hud, sizeof(hud), "%04d-%02d-%02d pos: (%d %d) zoom: %.2f t: %llu, Ek = %.2f",
                      localNow.year, localNow.month, localNow.day,
                      static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)), canvas->getZoom(),
                      static_cast<unsigned long long>(timestamp), ek_total);
        gv->draw_text(8, 8, hud, color_lux(COLOR_WHITE), 2);
        gv->update();

        SDL_Delay(1);
    }
    delete gv;
    return 0;
}
