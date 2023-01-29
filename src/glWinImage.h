#pragma once
#include "scene.h"
#include "utils.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <chrono>
#include <numeric>
#include <math.h>

class GlWinImage {
    using ImageType = Image<IMAGE_SIZE>;
    using FrameType = Frame<ImageType>;
    auto POS(auto x, auto y) { return ImageType::POS(x, y); }

  public:
    double secondsPerFrame=0;
    std::chrono::high_resolution_clock::time_point lastLog_{};

    void draw() {
        int width, height;
        glfwGetWindowSize(mpWindow, &width, &height);

        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glOrtho(0, width, 0, height, -1.0, 1.0);
        glViewport(0, 0, width, height);

        auto start = std::chrono::high_resolution_clock::now();
        drawImage();
        auto stop = std::chrono::high_resolution_clock::now();
        std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        secondsPerFrame = std::lerp(secondsPerFrame, duration.count() / 1e6, 0.05);

        if (std::chrono::duration_cast<std::chrono::seconds>(stop - lastLog_).count() > 5) {
            lastLog_ = stop;
            std::cout << "Seconds per frame: " << secondsPerFrame << std::endl;
        }

        glPixelZoom(width / static_cast<float>(IMAGE_SIZE), -height / static_cast<float>(IMAGE_SIZE));
        glRasterPos2i(0, height);
        glDrawPixels(IMAGE_SIZE, IMAGE_SIZE, GL_RGB, GL_FLOAT, &mImage.image[0]);

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

        glfwSetMouseButtonCallback(mpWindow, [](GLFWwindow *window, int button, int action, int mods) {
            static_cast<GlWinImage *>(glfwGetWindowUserPointer(window))->mouseEvent(window, button, action, mods);
        });

        glfwSetCursorPosCallback(mpWindow, [](GLFWwindow *window, double xpos, double ypos) {
            static_cast<GlWinImage *>(glfwGetWindowUserPointer(window))->mouseMoveEvent(window, xpos, ypos);
        });

        glfwSetKeyCallback(mpWindow, [](GLFWwindow *window, int key, int sc, int action, int mods) {
            static_cast<GlWinImage *>(glfwGetWindowUserPointer(window))->keyEvent(window, key, sc, action, mods);
        });
    }

  private:
    void drawImage() {
        scene_.updateState();
        float wyBegin = -0.5f * mFrame.mScale.y + mFrame.mCentre.y;
        float wyEnd = (static_cast<float>(IMAGE_SIZE) / static_cast<float>(mFrame.VIEW_HEIGHT) -0.5f) * mFrame.mScale.y + mFrame.mCentre.y;
        float yStep = (wyEnd - wyBegin) / static_cast<float>(IMAGE_SIZE);

        float wxBegin = -0.5f * mFrame.mScale.x + mFrame.mCentre.x;
        float wxEnd = (static_cast<float>(IMAGE_SIZE) / static_cast<float>(mFrame.VIEW_WIDTH) -0.5f) * mFrame.mScale.x + mFrame.mCentre.x;
        float xStep = (wxEnd - wxBegin) / static_cast<float>(IMAGE_SIZE);

        scene_.drawImage(mImage.image, mPalette, wxBegin, xStep, IMAGE_SIZE, wyBegin, yStep, IMAGE_SIZE);
    }

    void mouseEvent(GLFWwindow *window, int button, int action, [[maybe_unused]] int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            double px, py;
            glfwGetCursorPos(window, &px, &py);
            mLastMousePos = XYPair(px, py);
            mMouseLeftDown = GLFW_PRESS == action;

            int width, height;
            glfwGetWindowSize(mpWindow, &width, &height);
            if (mMouseLeftDown) {
                const double gridX = std::max(0.0, std::min<double>(IMAGE_SIZE - 1, IMAGE_SIZE * px / (double)width));
                const double gridY = std::max(0.0, std::min<double>(IMAGE_SIZE - 1, IMAGE_SIZE * py / (double)height));
                const XYPair cen = mFrame.imageToWorld(gridX, gridY);
                mFrame.mCentre = cen;
                mFrame.mScale.x *= 0.8;
                mFrame.mScale.y *= 0.8;
            }
        }
    }

    void mouseMoveEvent([[maybe_unused]] GLFWwindow *window, double xpos, double ypos) {
        if (mMouseLeftDown) {
            int width, height;
            glfwGetWindowSize(mpWindow, &width, &height);

            XYPair newMousePos(xpos, ypos);
            [[maybe_unused]] XYPair delta = newMousePos - mLastMousePos;
        }
    }

    void keyEvent([[maybe_unused]] GLFWwindow *window, int key, [[maybe_unused]] int sc, int action,
                  [[maybe_unused]] int mods) {
        if (GLFW_PRESS == action) {
            if (key == 'Q')
                mQuit = true;
            if (key == 'R')
                mFrame.reset();
            if (key == 'P')
                scene_.mPause = !scene_.mPause;
            if (key == ' ') {
                std::cout << "\nRand Vals:" << std::endl;
                scene_.rand();
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

    bool mMouseLeftDown = false;
    XYPair mLastMousePos{};
    bool mQuit = false;
    Scene2 scene_;
};
