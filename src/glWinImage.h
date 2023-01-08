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
  while (error > 0.001) {
    guess = (guess + x / guess) / 2;
    error = x - guess * guess;
  }
  return guess;
}

inline double powI(double x, int i) { return i > 0 ? x * powI(x, i - 1) : 1; }

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
  }

private:
  void updateState() {
    if (mPause) {
      return;
    }
    mTime += 1e-1;

    c1.updateFromOther(c2._x, c2._y);
    c1.updateFromOther(c3._x, c3._y);
    c1.updateFromOther(0, 0);
    c1.updateFromOther(0, 0);

    c2.updateFromOther(c1._x, c1._y);
    c2.updateFromOther(c3._x, c3._y);
    c2.updateFromOther(0, 0);
    c2.updateFromOther(0, 0);

    c3.updateFromOther(c1._x, c1._y);
    c3.updateFromOther(c2._x, c2._y);
    c3.updateFromOther(0, 0);
    c3.updateFromOther(0, 0);

    c1.updateVel();
    c2.updateVel();
    c3.updateVel();
  }

  class Calc {
  public:
    Calc() { randVals(); }
    double getVal(double x, double y) {
      x += _x;
      y += _y;

      double val = x * x * bXX + y * y * bYY + y * x * bXY;
      double sign = val > 0 ? 1.0 : -1.0;
      val = sign * fastSqrt(abs(val)) / size + offset;
      val = clip(val, -range, range);
      return val;
    }

    double randf(double max) { return (rand() % (int)(max * 100)) / 100.0; };
    double randfc(double max) {
      return (rand() % (int)(max * 100)) / 100.0 - max / 2.0;
    };
    double clip(double x, double down, double up) {
      return std::max(down, std::min(up, x));
    };

    void randVals() {

      // random initial location
      _x = randfc(4);
      _y = randfc(4);

      // initial velocity
      dx = randfc(0.2);
      dy = randfc(0.2);

      // params for this shape
      bXX = randfc(1) + 1.0;
      bYY = randfc(1) + 1.0;
      bXY = randfc(2);
      offset = randfc(3);

      range = 2; // range of output values, which affects number of mod lines

      size = 2.0 + randf(5);
      std::cout << "bXX: " << bXX << " bYY: " << bYY << " bXY: " << bXY
                << std::endl;
      std::cout << "Size: " << size << " offset: " << offset
                << " range: " << range << std::endl;
    }

    void updateFromOther(double x, double y) {
      const auto sqr = [](double x) {
        return std::min(5.0, x * x) * (x > 0 ? 1.0 : -1.0);
      };
      double maxAcc = 0.01;
      dx += clip(0.01 / sqr(x - _x), -maxAcc, maxAcc);
      dy += clip(0.01 / sqr(y - _y), -maxAcc, maxAcc);
    }
    void updateVel() {
      _x += dx;
      _y += dy;
      // bXY += randfc(0.02) + 0.05 * (0-bXY);
      // bXX += randfc(0.02) + 0.05 * (1-bXX);
      // bXX += randfc(0.02) + 0.05 * (1-bYY);
      size += randfc(0.1) + 0.1 * (5 - size);
    }

    double bXX, bYY, bXY;
    double offset;
    double _x, _y;
    double dx, dy;
    double size;
    double range;
  };

  Calc c1, c2, c3;

  inline double getValue(XYPair loc) {
    loc.x *= 2;
    loc.y *= 2;

    double val = c1.getVal(loc.x, loc.y) * c2.getVal(loc.x, loc.y) *
                 c3.getVal(loc.x, loc.y);

    return std::max(0.0, std::min(1.0, fmod(-val, 2)));
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
      if (key == ' ') {
        std::cout << "\nRand Vals:" << std::endl;
        c1.randVals();
        c2.randVals();
        c3.randVals();
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

  bool mMouseLeftDown{false};
  XYPair mLastMousePos{};
  bool mPause = false;
  bool mQuit{false};
};
