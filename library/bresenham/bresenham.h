#pragma once

#ifndef BRESENHAM_H_
#define BRESENHAM_H_

typedef union
{
    struct{
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    };
    uint32_t rgba;
}color_t;

typedef void (*set_pixel_cb_t)(int x, int y, uint32_t color);

void bresenham_init(set_pixel_cb_t set_pixel_cb);

void bresenham_set_color(uint32_t color);

uint32_t bresenham_get_color();

void plotLine(int x0, int y0, int x1, int y1);

void plotLine3d(int x0, int y0, int z0, int x1, int y1, int z1);

void plotEllipse(int xm, int ym, int a, int b);

void plotOptimizedEllipse(int xm, int ym, int a, int b);

void plotCircle(int xm, int ym, int r);

void plotEllipseRect(int x0, int y0, int x1, int y1);

void plotQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2);

void plotQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2);

void plotQuadRationalBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w);

void plotQuadRationalBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w);

void plotRotatedEllipse(int x, int y, int a, int b, float angle);

void plotRotatedEllipseRect(int x0, int y0, int x1, int y1, long zd);

void plotCubicBezierSeg(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3);

void plotCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);

void plotLineAA(int x0, int y0, int x1, int y1);

void plotCircleAA(int xm, int ym, int r);

void plotEllipseRectAA(int x0, int y0, int x1, int y1);

void plotQuadBezierSegAA(int x0, int y0, int x1, int y1, int x2, int y2);

void plotQuadRationalBezierSegAA(int x0, int y0, int x1, int y1, int x2, int y2, float w);

void plotCubicBezierSegAA(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3);

void plotLineWidth(int x0, int y0, int x1, int y1, float wd);

void plotQuadSpline(int n, int x[], int y[]);

void plotCubicSpline(int n, int x[], int y[]);

#endif
