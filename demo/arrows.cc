#include "arrows.h"

#include "util.h"
#include "geometry.h"
#include "gl_shader_program.h"

#include <glm/gtx/rotate_vector.hpp>

namespace
{
constexpr auto num_arrows = 256;
constexpr auto num_curve_points = 30;

constexpr auto aspect_ratio = 16.0f/9.0f;
constexpr auto virt_height = 512.f;
constexpr auto virt_width = virt_height*aspect_ratio;

struct arrow_state
{
    glm::vec2 p0;
    glm::vec2 p1;
    glm::vec2 p2;
    glm::vec2 padding;
    glm::vec4 color;
};
}

arrows::arrows(int width, int height)
    : width_(width)
    , height_(height)
    , ortho_proj_{ortho_projection_matrix(virt_width, virt_height)}
    , program_{new gl::shader_program}
    , bg_program_{new gl::shader_program}
    , geometry_{new geometry}
    , bg_geometry_{new geometry}
{
    init_arrows();
    init_gl_resources();
}

arrows::~arrows() = default;

void arrows::init_gl_resources()
{
    program_->add_shader(GL_VERTEX_SHADER, "assets/arrow.vert");
    program_->add_shader(GL_GEOMETRY_SHADER, "assets/arrow.geom");
    program_->add_shader(GL_FRAGMENT_SHADER, "assets/arrow.frag");
    program_->link();

    bg_program_->add_shader(GL_VERTEX_SHADER, "assets/bg.vert");
    bg_program_->add_shader(GL_FRAGMENT_SHADER, "assets/bg.frag");
    bg_program_->link();

    struct vertex
    {
        GLfloat t;
        GLfloat is_shadow;
    };
    std::vector<vertex> verts(2*2*(num_curve_points - 1));
    for (int i = 0; i < num_curve_points - 1; ++i) {
        const float t0 = static_cast<float>(i)/(num_curve_points - 1);
        const float t1 = static_cast<float>(i + 1)/(num_curve_points - 1);
        verts[2*i] = { t0, 1.0 };
        verts[2*i + 2*(num_curve_points - 1)] = { t0, 0.0 };
        verts[2*i + 1] = { t1, 1.0 };
        verts[2*i + 2*(num_curve_points - 1) + 1] = { t1, 0.0 };
    }

    geometry_->set_data(verts, {{1, GL_FLOAT, offsetof(vertex, t)}, {1, GL_FLOAT, offsetof(vertex, is_shadow)}});

    const std::vector<glm::vec2> bg_verts = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    bg_geometry_->set_data(bg_verts, {{2, GL_FLOAT, 0}});

    glGenBuffers(1, &state_ssbo_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, state_ssbo_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, arrows_.size() * sizeof(arrow_state), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void arrows::init_arrows()
{
    bool red_or_blue = true;
    const auto gen_control_point = [&red_or_blue](float min_x, float max_x, float min_y, float max_y, float min_d, float max_d) {
        if (!red_or_blue) {
            min_x = virt_width - min_x;
            max_x = virt_width - max_x;
        }
        glm::vec2 p(randf(min_x, max_x), randf(min_y, max_y));
        glm::vec2 d = randf(min_d, max_d)*glm::rotate(glm::vec2(1.0, 0.0), static_cast<float>(randf()*2.0*M_PI));
        float phi = randf(1.0, 3.0);
        return std::make_tuple(p, d, phi);
    };

    arrows_.resize(num_arrows);
    for (auto& arrow : arrows_) {
        std::tie(arrow.p0, arrow.d0, arrow.phi0) = gen_control_point(-40, -20, 256 - 10, 256 + 10, 5, 10);
        std::tie(arrow.p1, arrow.d1, arrow.phi1) = gen_control_point(40, 80, 256 - 200, 256 + 200, 80, 120);
        std::tie(arrow.p2, arrow.d2, arrow.phi2) = gen_control_point(300, 600, 256 - 200, 256 + 200, 160, 240);
        arrow.red_or_blue = red_or_blue;
        red_or_blue = !red_or_blue;
    }
}

void arrows::redraw(long time)
{
    const auto spotlight_center = glm::vec2(500, 500);

    bg_program_->bind();
    bg_program_->set_uniform_f("spotlight_center", spotlight_center.x, spotlight_center.y);
    bg_geometry_->bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    const float t = static_cast<float>(time)/1000.0;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, state_ssbo_);

    auto *data = static_cast<arrow_state *>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY));
    for (const auto &arrow : arrows_) {
        data->p0 = arrow.p0 + arrow.d0*cosf(t*arrow.phi0);
        data->p1 = arrow.p1 + arrow.d1*cosf(t*arrow.phi1);
        data->p2 = arrow.p2 + arrow.d2*cosf(t*arrow.phi2);
        data->color = arrow.red_or_blue ? glm::vec4(1, 1, 1, 1) : glm::vec4(.50, .50, .50, 0);
        ++data;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    program_->bind();
    program_->set_uniform_matrix4("proj_modelview", ortho_proj_);
    program_->set_uniform_f("start_thickness", 40.0);
    program_->set_uniform_f("end_thickness", 80.0);
    program_->set_uniform_f("shadow_offset", 8.0, -8.0);
    program_->set_uniform_f("resolution", width_, height_);
    program_->set_uniform_f("spotlight_center", .05, .95);
    program_->set_uniform_i("state_texture", 0); // texunit 0
    program_->set_uniform_f("t_offset", .5 + .5*cos(static_cast<float>(time)/500.));

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, state_ssbo_);

    geometry_->bind();
    glDrawArraysInstanced(GL_LINES, 0, 2*2*(num_curve_points - 1), num_arrows);
}
