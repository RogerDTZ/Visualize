#include <nanovg.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>
#define NANOVG_GL3_IMPLEMENTATION	
#include "nanovg_gl.h"
#include <functional>
#include <thread>
#include <memory>
#include <chrono>
#include <glm/glm.hpp>
#include <cassert>
#include <cstdint>
#include <map>
#include <cmath>

using namespace std::chrono_literals;

using DrawableID = uint32_t;

class Drawable {
private:
    DrawableID m_id;
public:
    explicit Drawable(DrawableID id) :m_id(id) {}
    virtual std::shared_ptr<Drawable> lerp(float u, const std::shared_ptr<Drawable> &rhs) const = 0;
    virtual void draw(NVGcontext *ctx) const = 0;
    virtual ~Drawable() = 0;
    DrawableID getID() const {
        return m_id;
    }
};

Drawable::~Drawable() {}

class Curve final :public Drawable {
private:
    glm::vec2 m_beg, m_end, m_ctrl;
public:
    Curve(DrawableID id, glm::vec2 beg, glm::vec2 end, glm::vec2 ctrl) :Drawable(id), m_beg(beg), m_end(end), m_ctrl(ctrl) {}
    std::shared_ptr<Drawable> lerp(float u, const std::shared_ptr<Drawable> &rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Curve>(rhs);
        assert(crhs);
        assert(crhs->getID() == getID());
        return std::make_shared<Curve>(getID(), glm::mix(m_beg, crhs->m_beg, u), glm::mix(m_end, crhs->m_end, u), glm::mix(m_ctrl, crhs->m_ctrl, u));
    }
    void draw(NVGcontext *ctx) const override {
        nvgBeginPath(ctx);
        nvgMoveTo(ctx, m_beg.x, m_beg.y);
        auto ct1 = (m_beg + m_ctrl) * 0.5f, ct2 = (m_ctrl + m_end) * 0.5f;
        nvgBezierTo(ctx, ct1.x, ct1.y, ct2.x, ct2.y, m_end.x, m_end.y);
        nvgStrokeColor(ctx, nvgRGBf(1.0f, 1.0f, 1.0f));
        nvgStroke(ctx);
    }
};

using FrameInfo = std::map<DrawableID, std::shared_ptr<Drawable>>;

int main(void) {
    GLFWwindow *window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(1440, 900, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewInit();
    auto vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

    glfwSetTime(0.0f);

    auto c1 = std::make_shared<Curve>(0, glm::vec2{ 100.0f, 100.0f }, glm::vec2{ 400.0f, 200.0f }, glm::vec2{ 100.0f, 300.0f });
    auto c2 = std::make_shared<Curve>(0, glm::vec2{ 200.0f, 100.0f }, glm::vec2{ 300.0f, 100.0f }, glm::vec2{ 400.0f, 500.0f });

    while (!glfwWindowShouldClose(window)) {

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        int win_w, win_h;
        glfwGetWindowSize(window, &win_w, &win_h);
        int frame_w, frame_h;
        glfwGetFramebufferSize(window, &frame_w, &frame_h);
        nvgBeginFrame(vg, static_cast<float>(win_w), static_cast<float>(win_h), static_cast<float>(frame_w) / win_w);

        c1->draw(vg);
        c2->draw(vg);
        auto t = static_cast<float>(glfwGetTime());
        auto u = fmodf(t, 1.0f);
        auto c3 = c1->lerp(u, c2);
        c3->draw(vg);

        nvgEndFrame(vg);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    nvgDeleteGL3(vg);
    glfwTerminate();
    return 0;
}
