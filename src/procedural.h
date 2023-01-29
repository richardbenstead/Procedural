#pragma once

#include "utils.h"

#define FTYPE 0

#if FTYPE==0
    using ValType=float;
#endif

#if FTYPE==1
    #define FIXMATH_NO_OVERFLOW
    #define FIXMATH_NO_ROUNDING
    #include "libfixmath/fix16.hpp"
    #include "libfixmath/fix16.c"
    using ValType=Fix16;
#endif

#if FTYPE==2
    #include "fixed.h"
    using ValType=Fixed16;
#endif

class Calc {

  public:
    Calc() : lower(-1.05f), upper(1.05f) {
        randVals();
        count++;
    }
    Calc(const Calc&) = delete;
    Calc(Calc&&) = default;

    ValType xCache_[IMAGE_SIZE];
    ValType yPartial_;

    const ValType lower;
    const ValType upper;

    inline float getVal(int xIdx, ValType xy) const {
        ValType val = sqrt(xCache_[xIdx] + yPartial_ + xy * polyXY) + offset;
        val /= (size * size);
        return std::clamp(float(val), -1.05f, 1.05f);
    }

    void updateXCache(int i, ValType x, ValType xx) {
        xCache_[i] = x * polyX + xx * polyXX;
    }

    void updateCurY(ValType y, ValType yy) {
        yPartial_ = y * polyY + yy * polyYY;
    }

    void updateConst() {
        // val = bXX * (x-_x)^2 + bYY * (y-_y)^2 + c
        // b_x x^2 - 2b_x x0x + b_x x0^2 + b_y y^2 - 2b_y y0y + b_y y0^2 + b_xy xy - b_xy x0y - b_xy y0x + b_xy x0y0
        // -2b_x x0x - b_xy y0x
        // -2b_y y0y -b_xy x0y
        // b_x x^2
        // b_y y^2
        // b_xy xy
        // b_x x0^2 + b_y y0^2 + b_xy x0y0
        //
        float invSize2 = 1.0;// / size * size;
        polyX = (-2 * bXX * _x - bXY * _y) * invSize2; // x
        polyY = (-2 * bYY * _y - bXY * _x) * invSize2; // y
        polyXX = bXX * invSize2;
        polyYY = bYY * invSize2;
        polyXY = bXY * invSize2;
        polyC = (_x * _x * bXX + _y * _y * bYY + _x * _y * bXY) * invSize2;
    }

    void randVals() {
        // random initial location
        _x = randfc(0.5);
        _y = randfc(0.5);

        // initial velocity
        dx = randfc(0.05);
        dy = randfc(0.05);

        // params for this shape
        bXX = randfc(1) + 1.0;
        bYY = randfc(1) + 1.0;
        angle = randf(2 * M_PI);
        dAngle = 0;
        bXY = cos(angle);
        offset = -2; //randfc(2);

        size = 0.5 + randf(0.2);
        printVals();
        updateConst();
    }

    void setVals(float y, float x, float XX, float YY, float XY, float offs, float size) {
        x = x;
        y = y;
        bXX = XX;
        bYY = YY;
        bXY = XY;
        offset = offs;
        size = size;
    }

    void printVals() {
        std::cout << "x: " << _x << " y: " << _y << " dx: " << dx << " dy: " << dy << std::endl;
        std::cout << "bXX: " << bXX << " bYY: " << bYY << " bXY: " << bXY << std::endl;
        std::cout << "Size: " << size << " offset: " << offset << std::endl;
    }

    void updateFromOther(float x, float y, float wgt) {
        std::cout << "Update from other" << std::endl;
        const auto sqr = [](float v) { return std::min(5.0f, v * v) * (v > 0 ? 1.0f : -1.0f); };
        float maxAcc = 0.1;
        float updRate = 0.1;
        dx += clip(wgt * updRate / sqr(x - _x), -maxAcc, maxAcc) + randfc(0.05);
        dy += clip(wgt * updRate / sqr(y - _y), -maxAcc, maxAcc) + randfc(0.05);
    }

    void updateVel() {
        _x += dx;
        _y += dy;
        // dAngle += randfc(0.05);
        // dAngle = clip(dAngle, -0.05, 0.05);
        // angle += dAngle;
        // bXY = cos(angle);
        // size += randfc(0.01) + 0.01 * (0.5 - size);
        updateConst();
        printVals();
    }

    ValType polyX, polyY, polyXX, polyYY, polyXY, polyC;
    float bXX{}, bYY{}, bXY{};
    float angle{}, dAngle{};
    float offset{};
    float _x{}, _y{}, dx{}, dy{};
    float size{};
    int id = count;
    static int count;
};

int Calc::count = 0;

