#pragma once
#include "utils.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <complex>
#include <cstring>
#include <iostream>
#include <math.h>

inline double fastSqrt(double x) {
  if (x <= 0) {
    return 0;
  }

  double guess = x / 2;
  double error = x - guess * guess;
  while (error > 0.1) {
    guess = (guess + x / guess) / 2;
    error = x - guess * guess;
  }
  return guess;
}

class GlWinImage {
  static constexpr uint16_t IMAGE_SIZE = 1200;
  using ImageType = Image<IMAGE_SIZE>;
  using FrameType = Frame<ImageType>;
  auto POS(auto x, auto y) { return ImageType::POS(x, y); }

public:
  void draw() {
    int width, height;
    glfwGetWindowSize(mpWindow, &width, &height);

    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glOrtho(0, width, 0, height, -1.0, 1.0);
    glViewport(0, 0, width, height);

    drawImage(width, height);

    glfwSwapBuffers(mpWindow);
    glfwPollEvents();
  }

  bool isFinished() { return mQuit || glfwWindowShouldClose(mpWindow); }

  void initialize(const std::string &title) {
    mpWindow = glfwCreateWindow(1200, 800, title.c_str(), nullptr, nullptr);
    if (!mpWindow) {
      throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwMakeContextCurrent(mpWindow);
    glfwSetWindowUserPointer(mpWindow, this);

    glfwSetMouseButtonCallback(
        mpWindow, [](GLFWwindow *window, int button, int action, int mods) {
          static_cast<GlWinImage *>(glfwGetWindowUserPointer(window))
              ->mouseEvent(window, button, action, mods);
        });

    glfwSetCursorPosCallback(
        mpWindow, [](GLFWwindow *window, double xpos, double ypos) {
          static_cast<GlWinImage *>(glfwGetWindowUserPointer(window))
              ->mouseMoveEvent(window, xpos, ypos);
        });

    glfwSetKeyCallback(mpWindow, [](GLFWwindow *window, int key, int sc,
                                    int action, int mods) {
      static_cast<GlWinImage *>(glfwGetWindowUserPointer(window))
          ->keyEvent(window, key, sc, action, mods);
    });

    auto setVals = [&](int id, double y, double x, double XX, double YY, double XY, double offs, double size) {
	    Calc& c = arrCalc[id];
	    c._x=x;
	    c._y=y;
	    c.bXX=XX;
	    c.bYY=YY;
	    c.bXY=XY;
	    c.offset=offs;
	    c.size=size;
	    c.invSize=1.0/size;
	    c.printVals();
    };

    //setVals(2, -4, 4, 2, 0.1, 0, -2, 1);
    //setVals(3, 4, 4, 0.1, 2, 0, -2, 1);
    //setVals(0, -4, -4, 1, 0.5, 0, -2, 1);
    //setVals(1, 4, -4, 0.5, 1, 0, -2, 1);
  }

private:
  void updateState() {
    if (mPause) {
      return;
    }
    mTime += 1e-1;

    for (Calc &calc1 : arrCalc) {
      for (Calc const &calc2 : arrCalc) {
        if (calc1.id != calc2.id) {
          calc1.updateFromOther(calc2._x, calc2._y, 1);
          calc1.updateFromOther(0, 0, 2);
        }
      }
      calc1.updateVel();
    }
  }

  class Calc {
  public:
    Calc() {
      randVals();
      count++;
    }

    double getVal(double x, double y) const {
      x += _x;
      y += _y;
      double val = x * x * bXX + y * y * bYY + y * x * bXY + y * x * x * bXXY +
                   x * y * y * bYYX;
      double sign = val > 0 ? 1.0 : -1.0;
      val = sign * fastSqrt(abs(val)) * invSize + offset;
      val = clip(val, -range, range);
      return val;
    }

    void randVals() {
      // random initial location
      _x = randfc(5.5);
      _y = randfc(5.5);

      // initial velocity
      dx = randfc(0.3);
      dy = randfc(0.3);

      // params for this shape
      bXXY = randfc(0.5);
      bYYX = randfc(0.5);
      bXX = randfc(1) + 1.0;
      bYY = randfc(1) + 1.0;
      angle = randf(2 * M_PI);
      dAngle = 0;
      bXY = cos(angle);
      offset = randfc(2);

      range = 1; // range of output values, which affects number of mod lines

      size = 5.0 + randf(2);
      invSize = 1.0 / size;
      printVals();
    }
    void printVals() {
      std::cout << "x: " << _x << " y: " << _y << " dx: " << dx << " dy: " << dy
                << std::endl;
      std::cout << "bXX: " << bXX << " bYY: " << bYY << " bXY: " << bXY
                << std::endl;
      std::cout << "Size: " << size << " offset: " << offset
                << " range: " << range << std::endl;
    }

    void updateFromOther(double x, double y, double wgt) {
      const auto sqr = [](double x) {
        return std::min(5.0, x * x) * (x > 0 ? 1.0 : -1.0);
      };
      double maxAcc = 0.001;
      double updRate = 0.001;
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
      size += randfc(0.1) + 0.1 * (5 - size);
    }

    double bXX{}, bYY{}, bXY{}, bXXY{}, bYYX{};
    double angle{}, dAngle{};
    double offset{};
    double _x{}, _y{}, dx{}, dy{};
    double size{}, range{1};
    double invSize{};;
    static int count;
    int id = count;
  };

  std::array<Calc, 4> arrCalc;

  inline double getValue(XYPair loc) {
    double x = loc.x * 5;
    double y = loc.y * 5;

    double val = 1;
    for (Calc const &calc : arrCalc) {
      val *= calc.getVal(x, y);
    }

    return std::max(0.0, std::min(1.0, fmod(mInv ? -val : val, 2)));
  }

  void drawImage(const int width, const int height) {
    int idx = 0;

    updateState();
    for (int y = 0; y < IMAGE_SIZE; ++y) {
      for (int x = 0; x < IMAGE_SIZE; ++x) {
        const double val = getValue(mFrame.imageToWorld(x, y));
        int palIdx = std::max<int>(
            0, std::min<int>(mPalette.SIZE - 1, mPalette.SIZE * val));
        mImage.image[idx++] = mPalette[palIdx];
      }
    }

    glPixelZoom(width / static_cast<float>(IMAGE_SIZE),
                -height / static_cast<float>(IMAGE_SIZE));
    glRasterPos2i(0, height);
    glDrawPixels(IMAGE_SIZE, IMAGE_SIZE, GL_RGB, GL_FLOAT, &mImage.image[0]);
  }

  void mouseEvent(GLFWwindow *window, int button, int action,
                  [[maybe_unused]] int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      double px, py;
      glfwGetCursorPos(window, &px, &py);
      mLastMousePos = XYPair(px, py);
      mMouseLeftDown = GLFW_PRESS == action;

      int width, height;
      glfwGetWindowSize(mpWindow, &width, &height);
      if (mMouseLeftDown) {
        const double gridX =
            std::max(0.0, std::min<double>(IMAGE_SIZE - 1,
                                           IMAGE_SIZE * px / (double)width));
        const double gridY =
            std::max(0.0, std::min<double>(IMAGE_SIZE - 1,
                                           IMAGE_SIZE * py / (double)height));
        const XYPair cen = mFrame.imageToWorld(gridX, gridY);
        mFrame.mCentre = cen;
        mFrame.mScale.x *= 0.8;
        mFrame.mScale.y *= 0.8;
      }
    }
  }

  void mouseMoveEvent([[maybe_unused]] GLFWwindow *window, double xpos,
                      double ypos) {
    if (mMouseLeftDown) {
      int width, height;
      glfwGetWindowSize(mpWindow, &width, &height);

      XYPair newMousePos(xpos, ypos);
      [[maybe_unused]] XYPair delta = newMousePos - mLastMousePos;
    }
  }

  void keyEvent([[maybe_unused]] GLFWwindow *window, int key,
                [[maybe_unused]] int sc, int action,
                [[maybe_unused]] int mods) {
    if (GLFW_PRESS == action) {
      if (key == 'Q')
        mQuit = true;
      if (key == 'R')
        mFrame.reset();
      if (key == 'P')
        mPause = !mPause;
      if (key == 'I')
        mInv = !mInv;
      if (key == ' ') {
        std::cout << "\nRand Vals:" << std::endl;
        for (Calc &calc : arrCalc) {
          calc.randVals();
        }
      }
      if (key == 'C')
        mPalette.nextPalette();
    }
  }

  GLFWwindow *mpWindow{};
  int mType{};
  Palette<> mPalette;
  ImageType mImage;
  FrameType mFrame;
  double mTime = 0;

  bool mMouseLeftDown = false;
  XYPair mLastMousePos{};
  bool mPause = false;
  bool mInv = false;
  bool mQuit = false;
};

int GlWinImage::Calc::count = 0;
