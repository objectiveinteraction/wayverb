#pragma once

#include "modern_gl_utils/generic_shader.h"

#include "glm/gtc/type_ptr.hpp"

class LitSceneShader final : public mglu::ShaderProgram {
public:
    LitSceneShader();

    void set_colour(const glm::vec3& c) const;

    void set_model_matrix(const glm::mat4& mat) const;
    void set_view_matrix(const glm::mat4& mat) const;
    void set_projection_matrix(const glm::mat4& mat) const;

private:
    static const std::string vertex_shader;
    static const std::string fragment_shader;
};
