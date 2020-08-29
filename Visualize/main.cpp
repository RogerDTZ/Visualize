#include <nanovg.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>
#define NANOVG_GL3_IMPLEMENTATION	
#include <nanovg_gl.h>
#include <functional>
#include <thread>
#include <memory>
#include <chrono>
#include <glm/glm.hpp>
#include <cassert>
#include <cstdint>
#include <map>
#include <filesystem>
#include <cmath>
#pragma warning(push,0)
#include <cxxopts.hpp>
#pragma warning(pop)
#include <fstream>
#include <nlohmann/json.hpp>
#include <opencv2/videoio.hpp>

using namespace std::chrono_literals;
namespace fs = std::filesystem;
using Json = nlohmann::json;

float parseFloat(const Json &fp) {
    return static_cast<float>(fp.get<double>());
}

NVGcolor parseColor(const Json &col) {
    return nvgRGB(col[0].get<int>(), col[1].get<int>(), col[2].get<int>());
}

glm::vec2 parseVec2(const Json &vec) {
    return { parseFloat(vec[0]), parseFloat(vec[1]) };
}

class Drawable {
private:
    NVGcolor m_col;
    float m_width;
    bool m_fill;
protected:
    Json m_args;
    void commit(NVGcontext *ctx) const {
        if (m_fill) {
            nvgFillColor(ctx, m_col);
            assert(m_width == 1.0f);
            nvgFill(ctx);
        }
        else {
            nvgStrokeColor(ctx, m_col);
            nvgStrokeWidth(ctx, m_width);
            nvgStroke(ctx);
        }
    }

public:
    explicit Drawable(const Json &args) :m_args(args), m_col(parseColor(args["color"])), m_width(parseFloat(args["width"])), m_fill(false) {
        auto iter = args.find("fill");
        if (iter != args.cend())m_fill = iter->get<bool>();
    }
    virtual std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const = 0;
    virtual void loadParams(const Json &args) = 0;
    virtual void draw(NVGcontext *ctx) const = 0;
    virtual ~Drawable() = 0 {}
};

std::shared_ptr<Drawable> mix(const std::shared_ptr<Drawable> &lhs, const std::shared_ptr < Drawable> &rhs, float u) {
    return lhs->mix(u, rhs);
}

class Rect final :public Drawable {
private:
    glm::vec2 m_pos, m_siz;

public:
    explicit Rect(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_pos = parseVec2(args["pos"]);
        m_siz = parseVec2(args["siz"]);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Rect>(rhs);
        assert(crhs);
        auto res = std::make_shared<Rect>(m_args);
        res->m_pos = glm::mix(m_pos, crhs->m_pos, u);
        res->m_siz = glm::mix(m_siz, crhs->m_siz, u);
        return res;
    }
    void draw(NVGcontext *ctx) const override {
        nvgBeginPath(ctx);
        nvgRect(ctx, m_pos.x, m_pos.y, m_siz.x, m_siz.y);
        commit(ctx);
    }
};

void drawLine(NVGcontext *ctx, glm::vec2 beg, glm::vec2 end) {
    nvgMoveTo(ctx, beg.x, beg.y);
    nvgLineTo(ctx, end.x, end.y);
}

glm::vec2 rotate(glm::vec2 dir, glm::vec2 rot) {
    return { dir.x * rot.x - dir.y * rot.y, dir.x * rot.y + dir.y * rot.x };
}

void drawArrow(NVGcontext *ctx, glm::vec2 ori, glm::vec2 dir) {
    glm::vec2 rot = glm::normalize(glm::vec2{ -1.0f, 1.0f }) * 15.0f;
    auto off1 = rotate(dir, rot);
    drawLine(ctx, ori, ori + off1);
    auto off2 = rotate(dir, { rot.x, -rot.y });
    drawLine(ctx, ori, ori + off2);
    nvgCircle(ctx, ori.x, ori.y, 1.0f);
}

class Line final :public Drawable {
private:
    glm::vec2 m_beg, m_end;

public:
    explicit Line(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_beg = parseVec2(args["beg"]);
        m_end = parseVec2(args["end"]);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Line>(rhs);
        assert(crhs);
        auto res = std::make_shared<Line>(m_args);
        res->m_beg = glm::mix(m_beg, crhs->m_beg, u);
        res->m_end = glm::mix(m_end, crhs->m_end, u);
        return res;
    }
    void draw(NVGcontext *ctx) const override {
        nvgBeginPath(ctx);
        drawLine(ctx, m_beg, m_end);
        auto delta = m_beg - m_end;
        auto dir = glm::normalize(delta);
        drawArrow(ctx, m_beg, dir);
        drawArrow(ctx, m_end, -dir);
        commit(ctx);
    }
};

class Curve final :public Drawable {
private:
    glm::vec2 m_beg, m_end, m_ctrl;
public:
    explicit Curve(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_beg = parseVec2(args["beg"]);
        m_end = parseVec2(args["end"]);
        m_ctrl = parseVec2(args["ctrl"]);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Curve>(rhs);
        assert(crhs);
        auto res = std::make_shared<Curve>(m_args);
        res->m_beg = glm::mix(m_beg, crhs->m_beg, u);
        res->m_end = glm::mix(m_end, crhs->m_end, u);
        res->m_ctrl = glm::mix(m_ctrl, crhs->m_ctrl, u);
        return res;
    }
    void draw(NVGcontext *ctx) const override {
        nvgBeginPath(ctx);
        nvgMoveTo(ctx, m_beg.x, m_beg.y);
        auto ct1 = (m_beg + m_ctrl) * 0.5f, ct2 = (m_ctrl + m_end) * 0.5f;
        nvgBezierTo(ctx, ct1.x, ct1.y, ct2.x, ct2.y, m_end.x, m_end.y);
        drawArrow(ctx, m_beg, glm::normalize(m_beg - ct1));
        drawArrow(ctx, m_end, glm::normalize(m_end - ct2));
        commit(ctx);
    }
};

class Text final :public Drawable {
private:
    glm::vec2 m_center;
    float m_siz;
    std::string m_text;
public:
    explicit Text(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_center = parseVec2(args["center"]);
        m_siz = parseFloat(args["siz"]);
        m_text = args["text"].get<std::string>();
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Text>(rhs);
        assert(crhs);
        auto res = std::make_shared<Text>(m_args);
        res->m_center = glm::mix(m_center, crhs->m_center, u);
        res->m_siz = glm::mix(m_siz, crhs->m_siz, u);
        res->m_text = (u < 0.5f ? m_text : crhs->m_text);
        return res;
    }
    void draw(NVGcontext *ctx) const override {
        nvgBeginPath(ctx);
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFontFace(ctx, "font");
        nvgFontSize(ctx, m_siz);
        nvgText(ctx, m_center.x, m_center.y, m_text.c_str(), nullptr);
        commit(ctx);
    }
};

class Circle final :public Drawable {
private:
    glm::vec2 m_center;
    float m_radius;
public:
    explicit Circle(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_center = parseVec2(args["center"]);
        m_radius = parseFloat(args["radius"]);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Circle>(rhs);
        assert(crhs);
        auto res = std::make_shared<Circle>(m_args);
        res->m_center = glm::mix(m_center, crhs->m_center, u);
        res->m_radius = glm::mix(m_radius, crhs->m_radius, u);
        return res;
    }
    void draw(NVGcontext *ctx) const override {
        nvgBeginPath(ctx);
        nvgCircle(ctx, m_center.x, m_center.y, m_radius);
        commit(ctx);
    }
};


class DrawableFactory final {
public:
    using Generator = std::function<std::shared_ptr<Drawable>(const Json &args)>;
    DrawableFactory() {
#define ITEM(name) m_generators[#name] = [] (const Json &args) {   return std::make_shared<name>(args);  }
        ITEM(Rect);
        ITEM(Curve);
        ITEM(Line);
        ITEM(Text);
        ITEM(Circle);
#undef ITEM
    }
    Generator get(const std::string &type) const {
        auto iter = m_generators.find(type);
        if (iter == m_generators.cend())throw;
        return iter->second;
    }
private:
    std::map <std::string, Generator> m_generators;
};

enum class MixMode {
    lerp, smoothstep, steep
};

MixMode str2MixMode(const std::string &name) {
    static const std::map<std::string, MixMode> lct = { { "lerp", MixMode::lerp }, { "smoothstep", MixMode::smoothstep }, { "steep", MixMode::steep } };
    auto iter = lct.find(name);
    if (iter == lct.cend())throw;
    return iter->second;
}

float applyMixFunc(MixMode mode, float u) {
    switch (mode) {
        case MixMode::lerp:
            return u;
            break;
        case MixMode::smoothstep:
        {
            auto f = [] (float u) {return  u * u * (3.0f - 2.0f * u); };
            return f(f(u));
        }
        break;
        case MixMode::steep:
            return 0.0f;
            break;
        default:throw;
            break;
    }
}

struct KeyFrame final {
    float timeStamp;
    MixMode mixMode;
    std::shared_ptr<Drawable> drawable;
    bool operator<(const KeyFrame &rhs) const {
        return timeStamp < rhs.timeStamp;
    }
};

struct DrawableAnimation final {
    std::vector<KeyFrame> frames;
};

int main(int argc, char **argv) {
    cxxopts::Options options("Renderer", "Algorithm Renderer");

    options.add_options()("input", "input file", cxxopts::value<std::string>())
        ("width", "video width", cxxopts::value<size_t>()->default_value("1920"))
        ("height", "video height", cxxopts::value<size_t>()->default_value("1080"))
        ("output", "output file", cxxopts::value<std::string>()->default_value("output.mp4"))
        ("rate", "frame rate", cxxopts::value<float>()->default_value("60"));

    auto result = options.parse(argc, argv);
    fs::path input = result["input"].as<std::string>();
    fs::path output = result["output"].as<std::string>();
    size_t width = result["width"].as<size_t>();
    size_t height = result["height"].as<size_t>();
    float rate = result["rate"].as<float>();
    float step = 1.0f / rate;
    float r1 = static_cast<float>(width) / height;

    std::ifstream in(input);
    Json json;
    in >> json;

    float dw = parseFloat(json["virtual_width"]);
    float dh = parseFloat(json["virtual_height"]);
    float endTime = parseFloat(json["duration"]);
    float r2 = dw / dh;

    float scale = (r2 > r1 ? (static_cast<float>(width) / dw) : static_cast<float>(height) / dh);
    dw *= scale; dh *= scale;
    glm::vec2 offset = { (width - dw) * 0.5f, (height - dh) * 0.5f };

    DrawableFactory factory;
    auto drawables = json["drawables"];
    std::vector<DrawableAnimation> anis;
    for (auto &&drawable : drawables) {
        auto type = drawable["type"].get<std::string>();
        auto gen = factory.get(type);
        auto frames = drawable["frame"];
        DrawableAnimation ani;
        for (auto &&frame : frames) {
            KeyFrame kframe;
            float ts = frame["ts"].get<float>();
            kframe.timeStamp = ts;
            if (frame.count("mix_mode")) {
                auto mixMode = frame["mix_mode"].get<std::string>();
                kframe.mixMode = str2MixMode(mixMode);
            }
            else kframe.mixMode = MixMode::lerp;
            kframe.drawable = gen(drawable);
            kframe.drawable->loadParams(frame);
            ani.frames.push_back(kframe);
        }
        std::sort(ani.frames.begin(), ani.frames.end());
        anis.push_back(ani);
    }

    GLFWwindow *window;
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_FALSE);
    window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), "Renderer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewInit();
    auto ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

    cv::Size sz(static_cast<int>(width), static_cast<int>(height));
    cv::VideoWriter writer(output.string(), cv::VideoWriter::fourcc('H', 'E', 'V', 'C'), rate, sz);

    for (float ct = 0.0f; ct < endTime; ct += step) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearStencil(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        int win_w, win_h;
        glfwGetWindowSize(window, &win_w, &win_h);
        int frame_w, frame_h;
        glfwGetFramebufferSize(window, &frame_w, &frame_h);
        nvgBeginFrame(ctx, static_cast<float>(win_w), static_cast<float>(win_h), 1.0f);
        nvgTranslate(ctx, offset.x, offset.y);
        nvgScale(ctx, scale, scale);

        for (auto &&ani : anis) {
            auto &&frames = ani.frames;
            auto iter = std::lower_bound(frames.cbegin(), frames.cend(), KeyFrame{ ct, MixMode::lerp, nullptr });
            if (iter == frames.cbegin() || iter == frames.cend())
                continue;
            auto prev = iter - 1;
            auto u = (ct - prev->timeStamp) / (iter->timeStamp - prev->timeStamp);
            auto toDraw = mix(prev->drawable, iter->drawable, applyMixFunc(iter->mixMode, u));
            toDraw->draw(ctx);
        }

        nvgEndFrame(ctx);
        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        glfwPollEvents();
        glfwSetWindowTitle(window, (std::to_string(100.0f * ct / endTime) + "%").c_str());

        cv::Mat buffer(sz, CV_8UC4);
        glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, buffer.data);
        cv::Mat frameData;
        cv::flip(buffer, frameData, 0);
        writer.write(frameData);
    }

    writer.release();
    nvgDeleteGL3(ctx);
    glfwTerminate();
    return 0;
}
