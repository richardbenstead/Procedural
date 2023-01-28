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

    ValType xCache_[IMAGE_SIZE];
    ValType yPartial_;

    const ValType lower;
    const ValType upper;

    inline float getVal(int xIdx, ValType xy) const {
        ValType val = xCache_[xIdx] + yPartial_ + xy * polyXY;
        return std::clamp(float(val), -1.05f, 1.05f);
    }

    void updateXCache(int i, ValType x, ValType xx) {
        xCache_[i] = x * polyX + xx * polyXX;
    }

    void updateCurY(ValType y, ValType yy) {
        yPartial_ = y * polyY + yy * polyYY + polyXY;
    }

    void updateConst() {
        float invSize2 = 1.0 / (size * size);
        polyX = (-bXX * 2 * _x - bXY * _y) * invSize2; // x
        polyY = (-bYY * 2 * _y - bXY * _x) * invSize2; // y
        polyXX = bXX * invSize2;
        polyYY = bYY * invSize2;
        polyXY = bXY * invSize2;
        polyC = (_x * _x * bXX + _y * _y * bYY + _x * _y * bXY) * invSize2 + offset;
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
        offset = -1; //randfc(2);

        size = 0.5 + randf(0.2);
        printVals();
        updateConst();
    }

    void printVals() {
        std::cout << "x: " << _x << " y: " << _y << " dx: " << dx << " dy: " << dy << std::endl;
        std::cout << "bXX: " << bXX << " bYY: " << bYY << " bXY: " << bXY << std::endl;
        std::cout << "Size: " << size << " offset: " << offset << std::endl;
    }

    void updateFromOther(float x, float y, float wgt) {
        const auto sqr = [](float x) { return std::min(5.0f, x * x) * (x > 0 ? 1.0f : -1.0f); };
        float maxAcc = 0.001;
        float updRate = 0.001;
        dx += clip(wgt * updRate / sqr(x - _x), -maxAcc, maxAcc);
        dy += clip(wgt * updRate / sqr(y - _y), -maxAcc, maxAcc);
    }

    void updateVel() {
        _x += dx;
        _y += dy;
        dAngle += randfc(0.05);
        dAngle = clip(dAngle, -0.05, 0.05);
        angle += dAngle;
        bXY = cos(angle);
        // size += randfc(0.01) + 0.01 * (0.5 - size);
        updateConst();
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

class Scene {
  public:
    Scene() {
        [[maybe_unused]] auto setVals = [&](int id, float y, float x, float XX, float YY, float XY, float offs, float size) {
            Calc &c = arrCalc[id];
            c._x = x;
            c._y = y;
            c.bXX = XX;
            c.bYY = YY;
            c.bXY = XY;
            c.offset = offs;
            c.size = size;
            c.printVals();
        };

        // setVals(2, -4, 4, 2, 0.1, 0, -2, 1);
        // setVals(3, 4, 4, 0.1, 2, 0, -2, 1);
        // setVals(0, -4, -4, 1, 0.5, 0, -2, 1);
        // setVals(1, 4, -4, 0.5, 1, 0, -2, 1);
    }
    inline void drawImage(auto& image, auto& palette, ValType xBegin, ValType xStep, int IMAGE_SIZE_X, ValType yBegin, ValType yStep, int IMAGE_SIZE_Y)
    {
        int idx=0;
        ValType x,y;
        // ValType val;
        int i,j;
        for (x=xBegin, i=0; i<IMAGE_SIZE_X; ++i, x+=xStep) {
            for (Calc &calc : arrCalc) {
                calc.updateXCache(i, x, x*x);
            }
        }

        for (y=yBegin, i=0; i<IMAGE_SIZE_Y; ++i, y+=yStep) {
            for (Calc &calc : arrCalc) {
                calc.updateCurY(y, y*y);
            }
            for (x=xBegin, j=0; j<IMAGE_SIZE_X; ++j, x+=xStep) {
                ValType xy = y*x;

                // val.value = fix16_one;
                float val = 1.0f;
                for (Calc const &calc : arrCalc) {
                    val *= calc.getVal(j, xy);
                }

                float val2 = std::clamp(float(val), 0.0f, 1.0f);
                image[idx++] = palette(val2);
                // val = fix16_clamp(val+0.5, ValType{}, ValType{fix16_one});
                // image[idx++] = palette(float(val));
            }
        }
    }
    inline void rand() {
        for (Calc &calc : arrCalc) {
            calc.randVals();
        }
    }
    void updateState() {
        if (mPause) {
            return;
        }
        for (Calc &calc1 : arrCalc) { // orbiting behaviour
            for (Calc const &calc2 : arrCalc) {
                if (calc1.id != calc2.id) {
                    calc1.updateFromOther(calc2._x, calc2._y, 1);
                    calc1.updateFromOther(0, 0, 2);
                }
            }
            calc1.updateVel();
        }
    }
    std::array<Calc, 3> arrCalc;
    bool mPause = false;
};
int Calc::count = 0;

