#pragma once

#include "utils.h"
#include "eigen3/Eigen/Dense"

inline float fastSqrt(float x) {
    if (x <= 0) {
        return 0;
    }

    float guess = x / 2;
    float error = x - guess * guess;
    while (error > 0.1) {
        guess = (guess + x / guess) / 2;
        error = x - guess * guess;
    }
    return guess;
}

class Calc {
  public:
    using coef_t = Eigen::Matrix<float, 6, 1>;
    Calc() {
        randVals();
        count++;
    }

    inline float getVal(coef_t const& v) const {
        float val = v.dot(betas_);
        return std::clamp(val, -1.05f, 1.05f);
    }

    void updateConst() {
        float invSize2 = 1.0 / (size * size);
        betas_[0] = (-bXX * 2 * _x - bXY * _y) * invSize2; // x
        betas_[1] = (-bYY * 2 * _y - bXY * _x) * invSize2; // y
        betas_[2] = bXX * invSize2;
        betas_[3] = bYY * invSize2;
        betas_[4] = bXY * invSize2;
        betas_[5] = (_x * _x * bXX + _y * _y * bYY + _x * _y * bXY) * invSize2 + offset;
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

    coef_t betas_;
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
    inline void drawImage(auto& image, auto& palette, float xBegin, float xStep, int IMAGE_SIZE_X, float yBegin, float yStep, int IMAGE_SIZE_Y)
    {
        float y = yBegin;
        int idx = 0;
        Calc::coef_t vals;
        vals[5] = 1.0f;
        for (int i=0; i<IMAGE_SIZE_Y; ++i) {
            float x = xBegin;
            vals[1] = y;
            vals[3] = y*y;
            for (int j=0; j<IMAGE_SIZE_X; ++j) {
                float val = 1;
                vals[0] = x;
                vals[2] = x*x;
                vals[4] = x*y;

                for (Calc const &calc : arrCalc) {
                    val *= calc.getVal(vals);
                }

                val = std::clamp(val, 0.0f, 1.0f);
                image[idx++] = palette(val);
                x += xStep;
            }
            y += yStep;
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

