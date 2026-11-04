#include <iostream>
#include "sdl_graphics.h"
#include "bitmap_image.hpp"
#include <cmath>
#include <fstream>
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
#include <filesystem>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

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

// Doppler ramp for speed: rest is the red end (650 nm), the fastest body is the blue end (430 nm).
Color dopplerSpeedColor(double speed, double speedMax)
{
    const double span = std::max(speedMax, 1e-8);
    const double t = std::sqrt(std::clamp(speed / span, 0.0, 1.0));
    const float nm = static_cast<float>(650.0 - t * (650.0 - 430.0));
    return wavelength(nm);
}

// One world unit is 1 AU, and `scale` is the pixels that 1 AU covers at the current zoom.
// Distance is the screen separation times that astronomical scale.
void evalFieldPhi(vector<PhiField>& field, const vector<RigidBody>& bodies, double G,
                  double originX, double originY, double scaleW, double scaleH)
{
    if (scaleW < 1e-8 || scaleH < 1e-8)
        return;

    const double auPerPxX = 1.0 / scaleW;
    const double auPerPxY = 1.0 / scaleH;
    const double soften2 = kSoften * kSoften;

    for (auto& gp : field)
    {
        double phi = 0.0;
        for (const auto& src : bodies)
        {
            const double bodyPx = originX + src.x() * scaleW;
            const double bodyPy = originY + src.y() * scaleH;
            const double dxAu = (gp.position.x() - bodyPx) * auPerPxX;
            const double dyAu = (gp.position.y() - bodyPy) * auPerPxY;
            const double dist2 = dxAu * dxAu + dyAu * dyAu + soften2;
            phi -= G * src.getMass() / std::sqrt(dist2);
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

    for (int ix = 0; ix < cols; ++ix)
    {
        for (int iy = 0; iy < rows; ++iy)
        {
            const double phi = field[static_cast<size_t>(ix * rows + iy)].phi;
            const int t = shadePotential(phi, range);
            const int wl = 380 + t * 400 / 1000;
            colors[static_cast<size_t>(iy * cols + ix)] = spectrumAt(wl).full();
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

struct OrbitFrame
{
    double a = 1;
    double e = 0;
    double cosArg = 1;
    double sinArg = 0;
    double cosOm = 1;
    double sinOm = 0;
    double cosInc = 1;
    double sinInc = 0;
};

OrbitFrame makeOrbitFrame(const KeplerOrbit& orbit, double century)
{
    OrbitFrame frame;
    frame.a = orbit.a0 + orbit.adot * century;
    frame.e = orbit.e0 + orbit.edot * century;
    const double peri = orbit.w0 + orbit.wdot * century;
    const double node = orbit.o0 + orbit.odot * century;
    const double arg = deg2rad(peri - node);
    const double Om = deg2rad(node);
    const double inc = deg2rad(orbit.i0 + orbit.idot * century);
    frame.cosArg = std::cos(arg);
    frame.sinArg = std::sin(arg);
    frame.cosOm = std::cos(Om);
    frame.sinOm = std::sin(Om);
    frame.cosInc = std::cos(inc);
    frame.sinInc = std::sin(inc);
    return frame;
}

// Heliocentric ecliptic coordinates for eccentric anomaly E, in degrees.
Point3D eclipticFromEccentric(const OrbitFrame& frame, double eccentricDeg)
{
    const double cosE = std::cos(deg2rad(eccentricDeg));
    const double sinE = std::sin(deg2rad(eccentricDeg));
    const double xp = frame.a * (cosE - frame.e);
    const double yp = frame.a * std::sqrt(std::max(0.0, 1.0 - frame.e * frame.e)) * sinE;
    const double x = (frame.cosArg * frame.cosOm - frame.sinArg * frame.sinOm * frame.cosInc) * xp
        + (-frame.sinArg * frame.cosOm - frame.cosArg * frame.sinOm * frame.cosInc) * yp;
    const double y = (frame.cosArg * frame.sinOm + frame.sinArg * frame.cosOm * frame.cosInc) * xp
        + (-frame.sinArg * frame.sinOm + frame.cosArg * frame.cosOm * frame.cosInc) * yp;
    const double z = (frame.sinArg * frame.sinInc) * xp + (frame.cosArg * frame.sinInc) * yp;
    return Point3D(x, y, z);
}

double solveEccentric(const OrbitFrame& frame, double meanDeg)
{
    const double eDeg = frame.e * 180.0 / pi;
    double E = meanDeg + eDeg * std::sin(deg2rad(meanDeg));
    for (int n = 0; n < 30; ++n)
    {
        const double sinE = std::sin(deg2rad(E));
        const double cosE = std::cos(deg2rad(E));
        const double dE = (meanDeg - (E - eDeg * sinE)) / (1.0 - frame.e * cosE);
        E += dE;
        if (std::fabs(dE) < 1e-8)
            break;
    }
    return E;
}

// Heliocentric ecliptic coordinates in AU. x toward the vernal equinox, z toward ecliptic north.
Point3D heliocentricEcliptic(const KeplerOrbit& orbit, double century)
{
    const OrbitFrame frame = makeOrbitFrame(orbit, century);
    const double mean = wrap180((orbit.L0 + orbit.Ldot * century) - (orbit.w0 + orbit.wdot * century));
    return eclipticFromEccentric(frame, solveEccentric(frame, mean));
}

constexpr double kAuKm = 149597870.7;
constexpr double kSunRadiusKm = 695700.0;
constexpr double kSunMassEarth = 332900.0;
constexpr int trails_count = 120;

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
    gv->set_color(fromIndex(Ansi::White));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    {
        ImFontConfig fontCfg;
        const char* fontPath = "C:/Windows/Fonts/msyh.ttc";
        if (std::filesystem::exists(fontPath))
            io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, &fontCfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    }
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(gv->sdl_window(), gv->sdl_renderer());
    ImGui_ImplSDLRenderer2_Init(gv->sdl_renderer());
    bool showField = false;
    bool showSidebar = false;
    bool showTrails = true;

    SDL_Event event;
    bool running = true;
    bool is_press = false;

    auto canvas = std::make_unique<GridCanvas>(height, height, 10);

    int mx, my;
    Point2D last_point(0, 0);

    const double G = 4.0 * M_PI * M_PI / 332900.0; // ~0.00011859
    float step = 1e-5f;
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
        for (double x = 0; x < width; x += canvas->block_size)
            xs.push_back(x);
        for (double y = 0; y < height; y += canvas->block_size)
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
    const float seed_max = 100.0f;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rand_pos(0.0,  seed_max);

    const CalendarDate localNow = readClock(false);
    const CalendarDate utcNow = readClock(true);
    const double utcDay = utcNow.day
        + (utcNow.hour + utcNow.minute / 60.0 + utcNow.second / 3600.0) / 24.0;
    const double jd = julianDate(utcNow.year, utcNow.month, utcDay);
    const double century = (jd - 2451545.0) / 36525.0;


    for (uint16_t i = 0; i < 10; i++)
    {
        canvas->body.emplace_back(rand_pos(gen)-seed_max/2, rand_pos(gen)-seed_max/2, rand_pos(gen) * 0.0001, rand_pos(gen) * kSunMassEarth);
        canvas->body.back().setVelocity(Point2D(rand_pos(gen)-seed_max/2, rand_pos(gen)-seed_max/2));

        canvas->body.emplace_back(rand_pos(gen)-seed_max/2, rand_pos(gen)-seed_max/2, rand_pos(gen) * 0.0001, rand_pos(gen) * kSunMassEarth);
        canvas->body.back().setVelocity(Point2D(rand_pos(gen)-seed_max/2, rand_pos(gen)-seed_max/2));
    }


    //canvas->body.emplace_back(sunPos.x(), sunPos.y()+10*1.414, kSunRadiusKm / kAuKm, kSunMassEarth);

    std::printf("%04d-%02d-%02d local  JD %.5f  heliocentric ecliptic (AU)\n",
                localNow.year, localNow.month, localNow.day, jd);

    std::vector<std::deque<Pixel>> trails(canvas->body.size());

    std::vector<uint32_t> fieldColor;
    // Fixed |phi| scale: softened solar surface down to 50 AU. Zoom then changes the pixel size of the well.
    PhiColorRange phiRange;
    phiRange.logMax = std::log(G * kSunMassEarth / kSoften);
    phiRange.logMin = std::log(G * kSunMassEarth / 50.0);
    phiRange.ready = true;

    while (running)
    {
        timestamp += 1;
        gv->clean();
        const Uint32 buttons = SDL_GetMouseState(&mx, &my);
        Point2D press(mx, my);

        static double drag_x = 0;
        static double drag_y = 0;

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            const bool captureMouse = ImGui::GetIO().WantCaptureMouse;
            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_MOUSEWHEEL:
                if (captureMouse)
                    break;
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
                if (!ImGui::GetIO().WantCaptureKeyboard)
                    std::cout << "Key pressed: " << static_cast<char>(event.key.keysym.sym) << std::endl;
                break;
            default:
                break;
            }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        const float margin = 8.0f;
        const float btnW = 76.0f;
        const float btnH = 40.0f;
        const ImGuiWindowFlags chrome = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(width) - btnW - margin, margin), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(btnW, btnH), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        ImGui::Begin("##settings", nullptr, chrome | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
        if (ImGui::Button("设置", ImVec2(-1.0f, -1.0f)))
            showSidebar = !showSidebar;
        ImGui::End();
        ImGui::PopStyleVar();

        if (showSidebar)
        {
            const float sideW = 196.0f;
            const float sideY = margin + btnH + 4.0f;
            static const float step_min = 1.0e-6;
            static const float step_max = 1.0;
            ImGui::SetNextWindowPos(ImVec2(static_cast<float>(width) - sideW - margin, sideY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(sideW, static_cast<float>(height) - sideY - margin), ImGuiCond_Always);
            if (ImGui::Begin("控制", &showSidebar, chrome))
            {
                ImGui::Checkbox("Field", &showField);
                ImGui::Checkbox("Trails", &showTrails);
                ImGui::Text("速度");
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::SliderScalar("##speed", ImGuiDataType_Float, &step,
                    &step_min,
                    &step_max,
                    "%.7f",
                    ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoRoundToFormat);
            }
            ImGui::End();
        }

        if ((buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) && !ImGui::GetIO().WantCaptureMouse)
        {
            drag_x += (press.x() - last_point.x());
            drag_y += (press.y() - last_point.y());
        }
        last_point = press;
        stepGravity(canvas->body, step, G);

        const double zoom = canvas->getZoom();
        const double scale_w = zoom * mapW;
        const double scale_h = zoom * mapH;
        const double x0 = canvas->pos.x() + drag_x;
        const double y0 = canvas->pos.y() + drag_y;

        if (showField)
        {
            evalFieldPhi(canvas->field, canvas->body, G, x0, y0, scale_w, scale_h);
            fieldColor = buildFieldColors(canvas->field, fieldCols, fieldRows, phiRange);
        }
        canvas->update();

        if (showField)
            gv->blit_bilinear(fieldColor.data(), fieldCols, fieldRows, 0, 0, width, height);

        double speedMax = 0.0;
        for (const auto& body : canvas->body)
            speedMax = std::max(speedMax, body.getVelocity().norm());

        for (size_t i = 0; i < canvas->body.size(); ++i)
        {
            const double speed = canvas->body[i].getVelocity().norm();
            trails[i].emplace_back(canvas->body[i].getPosition(), dopplerSpeedColor(speed, speedMax));
            if (trails[i].size() > trails_count)
                trails[i].pop_front();
        }

        if (showTrails)
        {
            for (size_t b = 0; b < trails.size(); ++b)
            {
                auto& trail = trails[b];
                for (size_t i = 1; i < trail.size(); ++i)
                {
                    const int x1 = static_cast<int>(std::lround(x0 + trail[i - 1].x() * scale_w));
                    const int y1 = static_cast<int>(std::lround(y0 + trail[i - 1].y() * scale_h));
                    const int x2 = static_cast<int>(std::lround(x0 + trail[i].x() * scale_w));
                    const int y2 = static_cast<int>(std::lround(y0 + trail[i].y() * scale_h));
                    gv->draw_line(x1, y1, x2, y2, trail[i].color());
                }
            }
        }

        for (size_t i = 0; i < canvas->body.size(); ++i)
        {
            const auto& p = canvas->body[i];
            const int pos_x = static_cast<int>(std::lround(x0 + p.x() * scale_w));
            const int pos_y = static_cast<int>(std::lround(y0 + p.y() * scale_h));
            // World unit is 1 AU, so the pixel radius is the true radius at this zoom.
            const int radiusPx = std::max(0, static_cast<int>(std::lround(p.getSize() * scale_w)));
            gv->fill_circle(pos_x, pos_y, radiusPx, dopplerSpeedColor(p.getVelocity().norm(), speedMax));
        }

        const double viewAu = scale_w > 1e-8 ? static_cast<double>(width) / scale_w : 0.0;
        ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.45f);
        ImGui::Begin("##hud", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::Text("%04d-%02d-%02d  pos: (%d %d)  %.2f AU  t: %llu",
                    localNow.year, localNow.month, localNow.day,
                    static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)), viewAu,
                    static_cast<unsigned long long>(timestamp));
        ImGui::End();
        gv->update();
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), gv->sdl_renderer());
        gv->present();

        SDL_Delay(1);
    }
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    delete gv;
    return 0;
}
