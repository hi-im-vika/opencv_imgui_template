/**
 * CApp.h - new file
 * 2024-04-15
 * vika <https://github.com/hi-im-vika>
 */

#pragma once

#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
#include <SDL.h>
#include <sstream>
#include <math.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include "CWindow.hpp"
#include "CCommonBase.hpp"
#include "CDPIHandler.hpp"

class CApp : public CCommonBase {
private:
    // imgui
    std::unique_ptr<CWindow> _window;
    SDL_Event _evt;
    GLuint _opencv_tex;
    cv::Mat _opencv_img, _opencv_raw_img;
    std::mutex _lockout_opencv;
    GLuint _mask_tex;
    cv::Mat _mask_img, _mask_raw_img;
    std::mutex _lockout_mask;
    bool _show_demo_window;

    // opencv
    cv::VideoCapture _video_capture;
    bool _flip_image;
    std::vector<std::string> _hsv_slider_names;
    cv::Scalar_<int> _hsv_threshold_low, _hsv_threshold_high;
    std::vector<int*> _pointer_hsv_thresholds;

    void mat_to_tex(cv::Mat &input, GLuint &output);

public:
    CApp(cv::Size s);
    ~CApp();

    void update() override;
    void draw() override;

};
