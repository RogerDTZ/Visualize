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
    NVGcolor m_col;
    float m_width;

protected:
    virtual void setDrawPara(NVGcontext* ctx) const {
        nvgStrokeColor(ctx, m_col);
        nvgStrokeWidth(ctx, m_width);
    }

public:
	explicit Drawable(DrawableID id) :m_id(id), m_col(nvgRGBf(1.f,1.f,1.f)), m_width(1.f) {}
    virtual std::shared_ptr<Drawable> lerp(float u, const std::shared_ptr<Drawable> &rhs) const = 0;
    void setColor(NVGcolor col) {
        m_col = col;
    }
    void setWidth(float width) {
        m_width = width;
    }
    virtual void draw(NVGcontext *ctx) const = 0;
    virtual ~Drawable() = 0;
    DrawableID getID() const {
        return m_id;
    }
};

Drawable::~Drawable() {}

class Rect final :public Drawable {
private:
    float m_x, m_y, m_w, m_h;

public:
    Rect(DrawableID id, float x, float y, float w, float h) :Drawable(id), m_x(x), m_y(y), m_w(w), m_h(h) {}
    std::shared_ptr<Drawable> lerp(float u, const std::shared_ptr<Drawable>& rhs) const override {
        auto crhs = std::dynamic_pointer_cast<Rect>(rhs);
        assert(crhs);
        assert(crhs->getID() == getID());
        return std::make_shared<Rect>(getID(), glm::mix(m_x, crhs->m_x, u), glm::mix(m_y, crhs->m_y, u), glm::mix(m_w, crhs->m_w, u), glm::mix(m_h, crhs->m_h, u));
    }
    void draw(NVGcontext* ctx) const override {
        nvgBeginPath(ctx);
        nvgRect(ctx, m_x, m_y, m_w, m_h);
        setDrawPara(ctx);
        nvgStroke(ctx);
    }

};

class Curve final :public Drawable {
private:
	glm::vec2 m_beg, m_end, m_ctrl;
public:
	Curve(DrawableID id, glm::vec2 beg, glm::vec2 end, glm::vec2 ctrl) :Drawable(id), m_beg(beg), m_end(end), m_ctrl(ctrl) {}
	std::shared_ptr<Drawable> lerp(float u, const std::shared_ptr<Drawable>& rhs) const override {
		auto crhs = std::dynamic_pointer_cast<Curve>(rhs);
		assert(crhs);
		assert(crhs->getID() == getID());
		return std::make_shared<Curve>(getID(), glm::mix(m_beg, crhs->m_beg, u), glm::mix(m_end, crhs->m_end, u), glm::mix(m_ctrl, crhs->m_ctrl, u));
	}
	void draw(NVGcontext* ctx) const override {
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, m_beg.x, m_beg.y);
		auto ct1 = (m_beg + m_ctrl) * 0.5f, ct2 = (m_ctrl + m_end) * 0.5f;
		nvgBezierTo(ctx, ct1.x, ct1.y, ct2.x, ct2.y, m_end.x, m_end.y);
        setDrawPara(ctx);
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
    auto r1 = std::make_shared<Rect>(1, 300.f, 300.f, 500.f, 100.f);
    auto r2 = std::make_shared<Rect>(1, 800.f, 700.f, 200.f, 300.f);

    while (!glfwWindowShouldClose(window)) {

        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        int win_w, win_h;
        glfwGetWindowSize(window, &win_w, &win_h);
        int frame_w, frame_h;
        glfwGetFramebufferSize(window, &frame_w, &frame_h);
        nvgBeginFrame(vg, static_cast<float>(win_w), static_cast<float>(win_h), static_cast<float>(frame_w) / win_w);

        /* Test Drawing*/
        c1->draw(vg); c2->draw(vg);
        r1->draw(vg); r2->draw(vg);
        auto t = static_cast<float>(glfwGetTime());
        auto u = fmodf(t, 1.0f);
        auto c3 = c1->lerp(u, c2);
        auto r3 = r1->lerp(u, r2);
        c3->draw(vg);
        r3->setColor(nvgRGB(0, 255, 143));
        r3->setWidth(4.f);
        r3->draw(vg);

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
