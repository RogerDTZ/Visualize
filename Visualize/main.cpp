#include <nanovg.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>
#define NANOVG_GL3_IMPLEMENTATION	// Use GL2 implementation.
#include "nanovg_gl.h"
#include <cstdlib>
#include <mutex>
#include <queue>
#include <functional>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

struct NVGcontext* vg = nullptr;
int win_w, win_h;

int main(void)
{
	GLFWwindow* window;

	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(1440, 900, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewInit();
	vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

	while (!glfwWindowShouldClose(window))
	{

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);
		glfwGetFramebufferSize(window, &win_w, &win_h);
		nvgBeginFrame(vg, win_w, win_h, 1.0);
		nvgEndFrame(vg);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}