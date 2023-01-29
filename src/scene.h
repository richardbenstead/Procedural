#pragma once

#include "procedural.h"
#include <vector>

class Scene {
  public:
    virtual ~Scene() {}
    inline void drawImage(auto& image, auto& palette, ValType xBegin, ValType xStep, int IMAGE_SIZE_X, ValType yBegin, ValType yStep, int IMAGE_SIZE_Y)
    {
        int idx=0;
        ValType x,y;
        int i,j;
        for (x=xBegin, i=0; i<IMAGE_SIZE_X; ++i, x+=xStep) {
            for (Calc *calc : vecCalc) {
                calc->updateXCache(i, x, x*x);
            }
        }

        for (y=yBegin, i=0; i<IMAGE_SIZE_Y; ++i, y+=yStep) {
            for (Calc *calc : vecCalc) {
                calc->updateCurY(y, y*y);
            }
            for (x=xBegin, j=0; j<IMAGE_SIZE_X; ++j, x+=xStep) {
                ValType xy = y*x;

                float val = 1.0f;
                for (Calc const *calc : vecCalc) {
                    val *= calc->getVal(j, xy);
                }

                float val2 = std::clamp(float(val), 0.0f, 1.0f);
                image[idx++] = palette(val2);
            }
        }
    }
    inline void rand() {
        for (Calc *calc : vecCalc) {
            calc->randVals();
        }
    }
    virtual void updateState() = 0;
    std::vector<Calc*> vecCalc;
    bool mPause = false;
};

class DefaultScene : public Scene {
  public:
    virtual void initScene() {
        std::cout << "init base scene" << std::endl;
        vecCalc.push_back(new Calc{});
        // vecCalc.push_back(new Calc{});
        // vecCalc.push_back(new Calc{});
    }

    virtual ~DefaultScene() {}
    virtual void updateState() {
        if (mPause) {
            return;
        }
        for (Calc *calc1 : vecCalc) { // orbiting behaviour
            for (Calc const *calc2 : vecCalc) {
                if (calc1->id != calc2->id) {
                    calc1->updateFromOther(calc2->_x, calc2->_y, 1);
                    calc1->updateFromOther(0, 0, 2);
                }
            }
            calc1->updateVel();
        }
    }
};

class Group {
    public:
    Group(float x, float y) : _x(x), _y(y) {
        calcs_.push_back(new Calc());
    }
    void updateState() {
        for (Calc *calc1 : calcs_) { // orbiting behaviour
            calc1->updateFromOther(_x, _y, 1);
            calc1->updateVel();
        }
    }
    std::vector<Calc*> calcs_;
    float const _x, _y;
};

class Scene2 : public Scene {
  public:
    Scene2() {
        std::cout << "init derived scene" << std::endl;
        Group g(-1,-1);
        for (Calc *calc1 : g.calcs_) {
            vecCalc.push_back(calc1);
        }
        vecGroups.push_back(g);
    }

    void updateState() {
        if (mPause) {
            return;
        }
        for (Group &g : vecGroups) {
            g.updateState();
        }
    }
    std::vector<Group> vecGroups;
};
