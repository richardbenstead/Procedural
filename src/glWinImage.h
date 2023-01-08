#pragma once
#include <math.h>
#include <cstring>
#include <iostream>
#include "utils.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <complex>
#include <algorithm>

inline double fastSqrt(double x) {
  if (x <= 0) {
    return 0;
  }

  double guess = x / 2;
  double error = x - guess * guess;
  while (error > 0.01) {
    guess = (guess + x / guess) / 2;
    error = x - guess * guess;
  }
  return guess;
}

inline double powI(double x, int i) {
    return i > 0 ? x * powI(x, i-1) : 1;
}

class GlWinImage
{
    static constexpr uint16_t IMAGE_SIZE = 1200;
    using ImageType = Image<IMAGE_SIZE>;
    using FrameType = Frame<ImageType>;
    auto POS(auto x, auto y) { return ImageType::POS(x,y); }

public:
    void draw()
    {
        int width, height;
        glfwGetWindowSize(mpWindow, &width, &height);

        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glOrtho(0, width, 0, height, -1.0, 1.0);
        glViewport(0,0,width,height);

        drawImage(width, height);
        
        glfwSwapBuffers(mpWindow);
        glfwPollEvents();
    }

    bool isFinished()
    {
        return mQuit || glfwWindowShouldClose(mpWindow);
    }

    void initialize(const std::string& title)
    {
        mpWindow = glfwCreateWindow(1200, 800, title.c_str(), nullptr, nullptr);
        if (!mpWindow) {
            throw std::runtime_error("glfwCreateWindow failed");
        }

        glfwMakeContextCurrent(mpWindow);
        glfwSetWindowUserPointer(mpWindow, this);

        glfwSetMouseButtonCallback(mpWindow, [](GLFWwindow *window, int button, int action, int mods) {
                static_cast<GlWinImage*>(glfwGetWindowUserPointer(window))->mouseEvent(window, button, action, mods); });

        glfwSetCursorPosCallback(mpWindow, [](GLFWwindow *window, double xpos, double ypos) {
                static_cast<GlWinImage*>(glfwGetWindowUserPointer(window))->mouseMoveEvent(window, xpos, ypos); });

        glfwSetKeyCallback(mpWindow, [](GLFWwindow* window, int key, int sc, int action, int mods) {
                static_cast<GlWinImage*>(glfwGetWindowUserPointer(window))->keyEvent(window, key, sc, action, mods); });
    }

private:
    double s1 = 0;
    double s2 = 0;
    double s3 = 0;
    double s4 = 0;
    double s5 = 0;

    void updateState() {
	if (mPause) {
		return;
	}
	mTime += 1e-1;
	s1 = sin(mTime);
	s2 = sin(2+mTime*1.1);
	s3 = sin(5+ mTime*0.7);
	s4 = -sin(2+mTime*0.3);
	s5 = -sin(3+mTime*0.5);
    }

    class Calc {
	    public:
	Calc() { 
		randVals();
	}
	    double getVal(double x, double y) {
		   double val = powI(x + ofsX, powX) * XX + powI(y + ofsY, powY) * YY + (y+ofsY) * (x+ofsX) * XY;
		   val = fastSqrt(val);
		   val = val / size - 1;
		   double const range = 5;
		   val = std::min(range, std::max(-range, val));
		   return val;
	    }
	void randVals() {
		auto randf = [](double max) {
			return (rand() % (int)(max*100)) / 100.0;
		};
		ofsX = randf(4) - 2.0;
		ofsY = randf(4) - 2.0;
		powX = 2;//+rand() % 2;
		powY = 2;//+rand() % 2;
		XX = randf(1)+0.5;
		YY = randf(1)+0.5;
		XY = randf(2)-1;
		YY=1;
		XX=1;
	
		size = 3+rand() % 5;
		std::cout << "ofs: " << ofsX << ", " << ofsY << std::endl;
		std::cout << "pow: " << powX << ", " << powY << std::endl;
		std::cout << "XX: " << XX << " YY: " << YY << " XY: " << XY << std::endl;
		std::cout << "Size: " << size << std::endl;
	}
	int powX, powY;
	double ofsX, ofsY;
	double XX, YY, XY;
	int size;
    };

    Calc c1,c2,c3;

    inline double getValue(XYPair loc)
    {
	loc.x *= 2;
	loc.y *= 2;

	double val = c1.getVal(loc.x+s1, loc.y+s2) *
			c2.getVal(loc.x+s3*3, loc.y+s4*1.2) *
			c3.getVal(loc.x+s5, loc.y+2*s2);

	// std::cout << val << std::endl;
	return std::max(0.0, std::min(1.0, fmod(-val,2)));
    }

    void drawImage(const int width, const int height)
    {
        int idx = 0;

	updateState();
            for(int y=0; y<IMAGE_SIZE; ++y) {
                for(int x=0; x<IMAGE_SIZE; ++x) {
                    const double val = getValue(mFrame.imageToWorld(x,y));
		    int palIdx = std::max<int>(0, std::min<int>(mPalette.SIZE-1, mPalette.SIZE * val));
                    mImage.image[idx++] = mPalette[palIdx];
                }
            }

        glPixelZoom(width/static_cast<float>(IMAGE_SIZE), -height/static_cast<float>(IMAGE_SIZE));
        glRasterPos2i(0, height);
        glDrawPixels(IMAGE_SIZE, IMAGE_SIZE, GL_RGB, GL_FLOAT, &mImage.image[0]);
    }

    void mouseEvent(GLFWwindow *window, int button, int action, [[maybe_unused]] int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            double px, py;
            glfwGetCursorPos(window, &px, &py);
            mLastMousePos = XYPair(px, py);
            mMouseLeftDown = GLFW_PRESS==action;

            int width, height;
            glfwGetWindowSize(mpWindow, &width, &height);
            if (mMouseLeftDown) {
                const double gridX = std::max(0.0, std::min<double>(IMAGE_SIZE-1, IMAGE_SIZE * px / (double)width));
                const double gridY = std::max(0.0, std::min<double>(IMAGE_SIZE-1, IMAGE_SIZE * py / (double)height));
                const XYPair cen = mFrame.imageToWorld(gridX, gridY);
                mFrame.mCentre = cen;
                mFrame.mScale.x *= 0.8;
                mFrame.mScale.y *= 0.8;
            }
        }
    }

    void mouseMoveEvent([[maybe_unused]] GLFWwindow *window, double xpos, double ypos)
    {
        if (mMouseLeftDown) {
            int width, height;
            glfwGetWindowSize(mpWindow, &width, &height);

            XYPair newMousePos(xpos, ypos);
            [[maybe_unused]] XYPair delta = newMousePos - mLastMousePos;
        }
    }

    void keyEvent([[maybe_unused]] GLFWwindow *window, int key, [[maybe_unused]] int sc, int action, [[maybe_unused]] int mods)
    {
        if (GLFW_PRESS == action) {
            if (key == 'Q') mQuit = true;
            if (key == 'R') mFrame.reset();
            if (key == 'P') mPause =! mPause;
            if (key == ' ') {
		    std::cout << "\nRand Vals:" << std::endl;
		    c1.randVals();
		    c2.randVals();
		    c3.randVals();
	    }
            if (key == 'C') mPalette.nextPalette();
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
