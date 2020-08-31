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
#include <algorithm>
#include <cmath>
#pragma warning(push,0)
#include <cxxopts.hpp>
#pragma warning(pop)
#include <fstream>
#include <nlohmann/json.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/opencv.hpp>

using namespace std::chrono_literals;
namespace fs = std::filesystem;
using Json = nlohmann::json;

float parseFloat(const Json &fp) {
    return static_cast<float>(fp.get<double>());
}

NVGcolor parseColor(const Json &col) {
    if (col.size() == 3)
        return nvgRGB(col[0].get<int>(), col[1].get<int>(), col[2].get<int>());
    return nvgRGBA(col[0].get<int>(), col[1].get<int>(), col[2].get<int>(), col[3].get<int>());
}

glm::vec2 parseVec2(const Json &vec) {
    return { parseFloat(vec[0]), parseFloat(vec[1]) };
}

class Drawable {
private:
    NVGcolor m_col;
    float m_width;
    bool m_fill;
    NVGcolor m_kcol;
    bool m_useKcol;
    float m_layer;
protected:

    void tryUseKcol(const Json &args) {
        if (args.count("color")) {
            m_useKcol = true;
            m_kcol = parseColor(args["color"]);
        }
    }

    void commit(NVGcontext *ctx) const {
        if (m_fill) {
            nvgFillColor(ctx, m_useKcol ? m_kcol : m_col);
            nvgFill(ctx);
        }
        else {
            nvgStrokeColor(ctx, m_useKcol ? m_kcol : m_col);
            nvgStrokeWidth(ctx, m_width);
            nvgStroke(ctx);
        }
    }

public:
    explicit Drawable(const Json &args) :m_col(parseColor(args["color"])), m_width(1.0f), m_fill(args.count("fill") ? args["fill"].get<bool>() : false), m_useKcol(false), m_layer(0) {
        if (!m_fill && args.count("width"))
            m_width = parseFloat(args["width"]);
        if (args.count("layer"))
            m_layer = parseFloat(args["layer"]);
    }
    virtual std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const = 0;
    virtual void loadParams(const Json &args) = 0;
    virtual void draw(NVGcontext *ctx, float w, float h) const = 0;
    virtual ~Drawable() = 0 {}
    bool  operator<(const Drawable &rhs) const {
        return m_layer < rhs.m_layer;
    }
};

std::shared_ptr<Drawable> mix(const std::shared_ptr<Drawable> &lhs, const std::shared_ptr < Drawable> &rhs, float u) {
    return lhs->mix(u, rhs);
}

#define MIX(name) res->name=glm::mix(name,crhs->name,u)

class Rect final :public Drawable {
private:
    glm::vec2 m_pos, m_siz;

public:
    explicit Rect(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_pos = parseVec2(args["pos"]);
        m_siz = parseVec2(args["siz"]);
        tryUseKcol(args);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Rect>(rhs);
        assert(crhs);
        auto res = std::make_shared<Rect>(*this);
        MIX(m_pos);
        MIX(m_siz);
        return res;
    }
    void draw(NVGcontext *ctx, float w, float h) const override {
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

void drawArrow(NVGcontext *ctx, glm::vec2 ori, glm::vec2 dir, float len) {
    if (len <= 0.0f)return;
    glm::vec2 rot = glm::normalize(glm::vec2{ -1.0f, 1.0f }) * len;
    auto off1 = rotate(dir, rot);
    drawLine(ctx, ori, ori + off1);
    auto off2 = rotate(dir, { rot.x, -rot.y });
    drawLine(ctx, ori, ori + off2);
    nvgCircle(ctx, ori.x, ori.y, 1.0f);
}

class Line final :public Drawable {
private:
    glm::vec2 m_beg, m_end;
    float m_begArrow, m_endArrow;

public:
    explicit Line(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_beg = parseVec2(args["beg"]);
        m_end = parseVec2(args["end"]);
        m_begArrow = (args.count("beg_arrow") ? parseFloat(args["beg_arrow"]) : -1.0f);
        m_endArrow = (args.count("end_arrow") ? parseFloat(args["end_arrow"]) : -1.0f);
        tryUseKcol(args);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Line>(rhs);
        assert(crhs);
        auto res = std::make_shared<Line>(*this);
        MIX(m_beg);
        MIX(m_end);
        MIX(m_begArrow);
        MIX(m_endArrow);
        return res;
    }
    void draw(NVGcontext *ctx, float w, float h) const override {
        nvgBeginPath(ctx);
        drawLine(ctx, m_beg, m_end);
        auto delta = m_beg - m_end;
        auto dir = glm::normalize(delta);
        drawArrow(ctx, m_beg, dir, m_begArrow);
        drawArrow(ctx, m_end, -dir, m_endArrow);
        commit(ctx);
    }
};

class Curve final :public Drawable {
private:
    glm::vec2 m_beg, m_end, m_ctrl;
    float m_begArrow, m_endArrow;
public:
    explicit Curve(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_beg = parseVec2(args["beg"]);
        m_end = parseVec2(args["end"]);
        m_ctrl = parseVec2(args["ctrl"]);
        m_begArrow = (args.count("beg_arrow") ? parseFloat(args["beg_arrow"]) : -1.0f);
        m_endArrow = (args.count("end_arrow") ? parseFloat(args["end_arrow"]) : -1.0f);
        tryUseKcol(args);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Curve>(rhs);
        assert(crhs);
        auto res = std::make_shared<Curve>(*this);
        MIX(m_beg);
        MIX(m_end);
        MIX(m_ctrl);
        MIX(m_begArrow);
        MIX(m_endArrow);
        return res;
    }
    void draw(NVGcontext *ctx, float w, float h) const override {
        nvgBeginPath(ctx);
        nvgMoveTo(ctx, m_beg.x, m_beg.y);
        auto ct1 = (m_beg + m_ctrl) * 0.5f, ct2 = (m_ctrl + m_end) * 0.5f;
        nvgBezierTo(ctx, ct1.x, ct1.y, ct2.x, ct2.y, m_end.x, m_end.y);
        drawArrow(ctx, m_beg, glm::normalize(m_beg - ct1), m_begArrow);
        drawArrow(ctx, m_end, glm::normalize(m_end - ct2), m_endArrow);
        commit(ctx);
    }
};

class Text final :public Drawable {
private:
    glm::vec2 m_center;
    float m_siz;
    std::vector< std::string> m_text;
public:
    explicit Text(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_center = parseVec2(args["center"]);
        m_siz = parseFloat(args["size"]);
        if (args["text"].type() == Json::value_t::array) {
            for (auto &&line : args["text"])
                m_text.push_back(line.get<std::string>());
        }
        else m_text = { args["text"].get<std::string>() };
        tryUseKcol(args);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Text>(rhs);
        assert(crhs);
        auto res = std::make_shared<Text>(*this);
        MIX(m_center);
        MIX(m_siz);
        res->m_text = (u < 0.5f ? m_text : crhs->m_text);
        return res;
    }
    void draw(NVGcontext *ctx, float w, float h) const override {
        nvgBeginPath(ctx);
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFontFace(ctx, "font");
        nvgFontSize(ctx, m_siz);
        auto basey = m_center.y - m_siz * 0.5f * m_text.size();
        for (size_t i = 0; i < m_text.size(); ++i)
            nvgText(ctx, m_center.x, basey + m_siz * i, m_text[i].c_str(), nullptr);
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
        tryUseKcol(args);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Circle>(rhs);
        assert(crhs);
        auto res = std::make_shared<Circle>(*this);
        MIX(m_center);
        MIX(m_radius);
        return res;
    }
    void draw(NVGcontext *ctx, float w, float h) const override {
        nvgBeginPath(ctx);
        nvgCircle(ctx, m_center.x, m_center.y, m_radius);
        commit(ctx);
    }
};

class Ray final :public Drawable {
private:
    glm::vec2 m_origin;
    float m_angle, m_arrow, m_arrowOffset;
public:
    explicit Ray(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_origin = parseVec2(args["origin"]);
        m_angle = parseFloat(args["angle"]);
        m_arrow = parseFloat(args["arrow"]);
        m_arrowOffset = parseFloat(args["arrow_offset"]);
        tryUseKcol(args);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Ray>(rhs);
        assert(crhs);
        auto res = std::make_shared<Ray>(*this);
        MIX(m_origin);
        MIX(m_angle);
        MIX(m_arrow);
        MIX(m_arrowOffset);
        return res;
    }
    void draw(NVGcontext *ctx, float w, float h) const override {
        nvgBeginPath(ctx);
        auto dir = glm::vec2{ cos(m_angle), sin(m_angle) };
        auto dest = m_origin + dir * 1e5f;
        drawLine(ctx, m_origin, dest);
        auto pos = m_origin + dir * m_arrowOffset;
        drawArrow(ctx, pos, dir, m_arrow);
        commit(ctx);
    }
};

class HalfPlane final :public Drawable {
private:
    glm::vec2 m_p1, m_p2;
    float m_arrow, m_arrowOffset;
public:
    explicit HalfPlane(const Json &args) :Drawable(args) {}
    void loadParams(const Json &args) override {
        m_p1 = parseVec2(args["p1"]);
        m_p2 = parseVec2(args["p2"]);
        m_arrow = parseFloat(args["arrow"]);
        m_arrowOffset = parseFloat(args["arrow_offset"]);
        tryUseKcol(args);
    }
    std::shared_ptr<Drawable> mix(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<HalfPlane>(rhs);
        assert(crhs);
        auto res = std::make_shared<HalfPlane>(*this);
        MIX(m_p1);
        MIX(m_p2);
        MIX(m_arrow);
        MIX(m_arrowOffset);
        return res;
    }
    void draw(NVGcontext *ctx, float w, float h) const override {
        nvgBeginPath(ctx);
        auto dir = glm::normalize(m_p1 - m_p2);
        auto beg = m_p2 + dir * 1e5f;
        auto end = m_p1 - dir * 1e5f;
        drawLine(ctx, beg, end);
        auto N = glm::vec2{ -dir.y, dir.x };

        auto step = dir * m_arrowOffset;
        glm::vec2 base;
        if (fabsf(step.x) > fabsf(step.y)) {
            base = m_p1;
            if (step.x < 0.0f)step = -step;
            while (m_p1.x > 0.0f)
                base -= step;
            while (m_p1.x < 0.0f)
                base += step;
        }
        else {
            if (step.y < 0.0f)step = -step;
            base = m_p1;
            while (m_p1.y > 0.0f)
                base -= step;
            while (m_p1.y < 0.0f)
                base += step;
        }
        while (base.x < w && base.y < h) {
            auto dst = base + N * m_arrow * 1.5f;
            drawLine(ctx, base, dst);
            drawArrow(ctx, dst, N, m_arrow);
            base += step;
        }
        commit(ctx);
    }
};

#undef MIX

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
        ITEM(Ray);
        ITEM(HalfPlane);
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
    float odw = dw, odh = dh;
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
    glDisable(GL_FRAMEBUFFER_SRGB);

    auto ctx = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    nvgCreateFont(ctx, "font", "consola.ttf");

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

        //debug scissor
        {
            nvgBeginPath(ctx);
            nvgRect(ctx, offset.x, offset.y, dw, dh);
            nvgStrokeColor(ctx, nvgRGBf(1.0f, 1.0f, 1.0f));
            nvgStroke(ctx);
        }

        nvgScissor(ctx, offset.x, offset.y, dw, dh);
        nvgTranslate(ctx, offset.x, offset.y);
        nvgScale(ctx, scale, scale);

        std::vector<std::shared_ptr<Drawable>> toDraw;
        for (auto &&ani : anis) {
            auto &&frames = ani.frames;
            auto iter = std::lower_bound(frames.cbegin(), frames.cend(), KeyFrame{ ct, MixMode::lerp, nullptr });
            if (iter == frames.cbegin() || iter == frames.cend())
                continue;
            auto prev = iter - 1;
            auto u = (ct - prev->timeStamp) / (iter->timeStamp - prev->timeStamp);
            toDraw.push_back(mix(prev->drawable, iter->drawable, applyMixFunc(iter->mixMode, u)));
        }

        std::sort(toDraw.begin(), toDraw.end(), [] (const std::shared_ptr<Drawable> &lhs, const std::shared_ptr<Drawable> &rhs) {
            return *lhs < *rhs;
            });

        for (auto &&item : toDraw)
            item->draw(ctx, odw, odh);

        nvgEndFrame(ctx);
        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        glfwPollEvents();
        glfwSetWindowTitle(window, ("Progress:" + std::to_string(100.0f * ct / endTime) + "%").c_str());

        cv::Mat buffer(sz, CV_8UC4);
        glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, buffer.data);
        cv::Mat frameData;
        cv::flip(buffer, frameData, 0);
        cv::Mat bgr;
        cv::cvtColor(frameData, bgr, cv::COLOR_RGB2BGR);

        writer.write(bgr);
    }

    writer.release();
    nvgDeleteGL3(ctx);
    glfwTerminate();
    return 0;
}
