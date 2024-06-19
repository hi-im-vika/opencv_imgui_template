/**
 * CApp.cpp - new file
 * 2024-04-15
 * vika <https://github.com/hi-im-vika>
 */

#include "../include/CApp.hpp"


CApp::CApp(cv::Size s) {
    _window_size = s;

    // SDL init
    uint init_flags = SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER;

    if (SDL_Init(init_flags) != 0) {
        spdlog::error("Error during SDL init");
        exit(-1);
    }

    // dear imgui init
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    _window = std::make_unique<CWindow>(WINDOW_NAME, _window_size.width, _window_size.height);
    if (_window == nullptr) {
        spdlog::error("Error creating window");
        exit(-1);
    }

    SDL_GL_MakeCurrent(_window->get_native_window(), _window->get_native_context());
    SDL_GL_SetSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingTransparentPayload = true;

    // scale fonts for DPI
    const float font_scaling_factor = CDPIHandler::get_scale();
    const float font_size = 16.0F * font_scaling_factor;
    const std::string font_path = "../res/font/inter.ttf";

    io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
    io.FontDefault = io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
    CDPIHandler::set_global_font_scaling(&io);

    // rendering init

    ImGui_ImplSDL2_InitForOpenGL(_window->get_native_window(), _window->get_native_context());
    ImGui_ImplOpenGL3_Init(glsl_version);

    _show_demo_window = false;

    // OpenCV init

    _video_capture = cv::VideoCapture("videotestsrc ! appsink", cv::CAP_GSTREAMER); // change this to change video source
    _opencv_img = cv::Mat::ones(cv::Size(20, 20), CV_8UC3);
    _mask_img = cv::Mat::ones(cv::Size(20, 20), CV_8UC3);
    _flip_image = false;
    _hsv_slider_names = {
            "Hue (lower)",
            "Hue (upper)",
            "Saturation (lower)",
            "Saturation (upper)",
            "Value (lower)",
            "Value (upper)",
    };
    _pointer_hsv_thresholds = {
            &_hsv_threshold_low[0],
            &_hsv_threshold_high[0],
            &_hsv_threshold_low[1],
            &_hsv_threshold_high[1],
            &_hsv_threshold_low[2],
            &_hsv_threshold_high[2],
    };

    _hsv_threshold_low = cv::Scalar_<int>(0,0,0);
    _hsv_threshold_high = cv::Scalar_<int>(255,255,255);

    // preallocate texture handle
    glGenTextures(1, &_opencv_tex);
    glGenTextures(1, &_mask_tex);
}

CApp::~CApp() = default;

void CApp::update() {
    _video_capture.read(_opencv_raw_img);
    _lockout_opencv.lock();
    _opencv_img = _opencv_raw_img;
    _lockout_opencv.unlock();

    if (!_opencv_img.empty()) {
        cv::Scalar hsv_threshold_low = _hsv_threshold_low;
        cv::Scalar hsv_threshold_high = _hsv_threshold_high;
        _lockout_mask.lock();
        cv::cvtColor(_opencv_img, _mask_raw_img, cv::COLOR_BGR2HSV);
        cv::dilate(_mask_raw_img, _mask_raw_img, cv::Mat());
        cv::inRange(_mask_raw_img, hsv_threshold_low, hsv_threshold_high, _mask_raw_img);
        _mask_raw_img.convertTo(_mask_raw_img, CV_8UC1);
        _mask_img = _mask_raw_img;
        _lockout_mask.unlock();
    }
}

void CApp::draw() {

    // handle all events
    while (SDL_PollEvent(&_evt)) {
        ImGui_ImplSDL2_ProcessEvent(&_evt);
        switch (_evt.type) {
            case SDL_QUIT:
                spdlog::info("Quit");
                _do_exit = true;
                break;
            default:
                break;
        }
    }

    bool *p_open = nullptr;
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport();

    ImGui::Begin("Settings", p_open);

    // opencv parameters
    ImGui::SeparatorText("Mask Calibration");
    ImGui::BeginGroup();
    ImGui::BeginTable("##cal_item_table", 2, ImGuiTableFlags_SizingFixedFit);
    ImGui::TableSetupColumn("##cal_item_title", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("##cal_item_value", ImGuiTableColumnFlags_WidthStretch);
    for (int i = 0; i < _hsv_slider_names.size(); i++) {
        if (i < 2) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", _hsv_slider_names.at(i).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-FLT_MIN);
            ImGui::SliderInt(_hsv_slider_names.at(i).c_str(), _pointer_hsv_thresholds.at(i), 0, 180);
            ImGui::PopItemWidth();
        } else {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", _hsv_slider_names.at(i).c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-FLT_MIN);
            ImGui::SliderInt(_hsv_slider_names.at(i).c_str(), _pointer_hsv_thresholds.at(i), 0, 255);
            ImGui::PopItemWidth();
        }
    }
    ImGui::EndTable();
    ImGui::EndGroup();
    ImGui::End();

    // opencv image
    ImGui::Begin("OpenCV", p_open);

    // from https://www.reddit.com/r/opengl/comments/114lxvr/imgui_viewport_texture_not_fitting_scaling_to/
    ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    float ratio = ((float) _opencv_img.cols) / ((float) _opencv_img.rows);
    float viewport_ratio = viewport_size.x / viewport_size.y;

    _lockout_opencv.lock();
    _opencv_img = _opencv_raw_img;
    _lockout_opencv.unlock();

    mat_to_tex(_opencv_img, _opencv_tex);

    // Scale the image horizontally if the content region is wider than the image
    if (viewport_ratio > ratio) {
        float imageWidth = viewport_size.y * ratio;
        float xPadding = (viewport_size.x - imageWidth) / 2;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xPadding);
        ImGui::Image((ImTextureID) (intptr_t) _opencv_tex, ImVec2(imageWidth, viewport_size.y));
    }
        // Scale the image vertically if the content region is taller than the image
    else {
        float imageHeight = viewport_size.x / ratio;
        float yPadding = (viewport_size.y - imageHeight) / 2;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yPadding);
        ImGui::Image((ImTextureID) (intptr_t) _opencv_tex, ImVec2(viewport_size.x, imageHeight));
    }
    ImGui::End();

    // mask image
    ImGui::Begin("Mask", p_open);

    // from https://www.reddit.com/r/opengl/comments/114lxvr/imgui_viewport_texture_not_fitting_scaling_to/
    viewport_size = ImGui::GetContentRegionAvail();
    ratio = ((float) _mask_img.cols) / ((float) _mask_img.rows);
    viewport_ratio = viewport_size.x / viewport_size.y;

    _lockout_mask.lock();
    _mask_img = _mask_raw_img;
    _lockout_mask.unlock();

    mat_to_tex(_mask_img, _mask_tex);

    // Scale the image horizontally if the content region is wider than the image
    if (viewport_ratio > ratio) {
        float imageWidth = viewport_size.y * ratio;
        float xPadding = (viewport_size.x - imageWidth) / 2;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xPadding);
        ImGui::Image((ImTextureID) (intptr_t) _mask_tex, ImVec2(imageWidth, viewport_size.y));
    }
        // Scale the image vertically if the content region is taller than the image
    else {
        float imageHeight = viewport_size.x / ratio;
        float yPadding = (viewport_size.y - imageHeight) / 2;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yPadding);
        ImGui::Image((ImTextureID) (intptr_t) _mask_tex, ImVec2(viewport_size.x, imageHeight));
    }
    ImGui::End();

    // imgui window (for debug)
    ImGui::Begin("ImGui", p_open);
    ImGui::Text("dear imgui says hello! (%s) (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
    ImGui::Checkbox("Show demo window",&_show_demo_window);

    ImGui::End();

    if (_show_demo_window) {
        ImGui::ShowDemoWindow(&_show_demo_window);
    }

    ImGui::Render();

    glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
    glClearColor(0.5F, 0.5F, 0.5F, 1.00F);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
        SDL_Window *backup_current_window{SDL_GL_GetCurrentWindow()};
        SDL_GLContext backup_current_context{SDL_GL_GetCurrentContext()};
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(_window->get_native_window());

    // limit to 1000 FPS
    while ((int) std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - _perf_draw_start).count() < 1);
}

void CApp::mat_to_tex(cv::Mat &input, GLuint &output) {
    if (input.empty()) return;
    cv::Mat flipped;
    // might crash here if input array is somehow emptied
    cv::cvtColor(input, flipped, cv::COLOR_BGR2RGB);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    glBindTexture(GL_TEXTURE_2D, output);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, input.cols, input.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, flipped.data);
}

int main(int argc, char *argv[]) {
    CApp c = CApp(cv::Size(1280, 720));
    c.run();
    return 0;
}