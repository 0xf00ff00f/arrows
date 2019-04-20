#include "panic.h"

#include "arrows.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <unistd.h>

#include <cstdio>

int main(int argc, char *argv[])
{
    bool fullscreen = false;

    int opt;
    while ((opt = getopt(argc, argv, "f")) != -1) {
        switch (opt) {
            case 'f':
                fullscreen = true;
                break;

            default:
                break;
        }
    }

    if (!glfwInit())
        panic("glfwInit failed");

    glfwSetErrorCallback([](int error, const char *description) {
        panic("GLFW error: %s", description);
    });

    int window_width, window_height;
    GLFWmonitor *monitor;

    if (!fullscreen) {
        monitor = nullptr;
        window_width = 910;
        window_height = 512;
    } else {
        monitor = glfwGetPrimaryMonitor();
        const auto mode = glfwGetVideoMode(monitor);
        window_width = mode->width;
        window_height = mode->height;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    GLFWwindow *window = glfwCreateWindow(window_width, window_height, "1997", monitor, nullptr);
    if (!window)
        panic("glfwCreateWindow failed");

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewInit();

    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
    });

    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);

    {
        arrows demo(fb_width, fb_height);

        const auto start = std::chrono::steady_clock::now();

#ifdef DUMP_FRAMES
        constexpr auto total_time = 3000;
        constexpr auto total_frames = 160;
        auto frame_num = 0;
        std::vector<char> frame_data;
        frame_data.resize(fb_width*fb_height*4);
#endif

        while (!glfwWindowShouldClose(window)) {
            glViewport(0, 0, fb_width, fb_height);
            glClearColor(0.5, 0.5, 0.5, 0.0);
            glClear(GL_COLOR_BUFFER_BIT);

#ifndef DUMP_FRAMES
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
#else
            const auto elapsed = static_cast<float>(frame_num*total_time)/total_frames;
#endif
            demo.redraw(elapsed);

#ifdef DUMP_FRAMES
            glReadPixels(0, 0, fb_width, fb_height, GL_RGBA, GL_UNSIGNED_BYTE, frame_data.data());

            char path[80];
            sprintf(path, "%05d.ppm", frame_num++);

            if (FILE *out = fopen(path, "wb")) {
                fprintf(out, "P6\n%d %d\n255\n", fb_width, fb_height);
                char *p = frame_data.data();
                for (auto i = 0; i < fb_width*fb_height; ++i)
                {
                    fputc(*p++, out);
                    fputc(*p++, out);
                    fputc(*p++, out);
                    ++p;
                }
                fclose(out);
            }
#endif

            glfwSwapBuffers(window);
            glfwPollEvents();

#ifdef DUMP_FRAMES
            if (frame_num == total_frames)
                break;
#endif
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
