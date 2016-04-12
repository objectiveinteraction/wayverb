#pragma once

#include "ConfigPanel.hpp"
#include "FilePackage.hpp"

#include "BasicDrawableObject.hpp"
#include "BoxObject.hpp"
#include "MeshObject.hpp"
#include "ModelObject.hpp"
#include "ModelSectionObject.hpp"
#include "OctahedronObject.hpp"

#define GLM_FORCE_RADIANS
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "combined_config.h"
#include "octree.h"
#include "raytracer.h"
#include "scene_data.h"
#include "voxel_collection.h"
#include "waveguide.h"

#include "../JuceLibraryCode/JuceHeader.h"

#include <cmath>
#include <future>
#include <mutex>

class RaytraceObject final : public ::Drawable {
public:
    RaytraceObject(const GenericShader& shader,
                   const RaytracerResults& results);
    void draw() const override;

private:
    const GenericShader& shader;

    VAO vao;
    StaticVBO geometry;
    StaticVBO colors;
    StaticIBO ibo;
    GLuint size;
};

class VoxelisedObject final : public BasicDrawableObject {
public:
    VoxelisedObject(const GenericShader& shader,
                    const SceneData& scene_data,
                    const VoxelCollection& voxel);
    void draw() const override;

private:
    std::vector<glm::vec3> get_vertices(const SceneData& scene_data) const;
    std::vector<GLuint> get_indices(const SceneData& scene_data,
                                    const VoxelCollection& voxel) const;

    VoxelCollection voxel;
};

class DrawableScene final : public ::Drawable, public ::Updatable {
public:
    DrawableScene(const GenericShader& shader,
                  const SceneData& scene_data,
                  const config::Combined& cc);
    ~DrawableScene() noexcept;

    void update(float dt) override;
    void draw() const override;

    void init_waveguide(const SceneData& scene_data,
                        const config::Waveguide& cc);
    void trigger_pressure_calculation();
    RaytracerResults get_raytracer_results(const SceneData& scene_data,
                                           const config::Combined& cc);

private:
    const GenericShader& shader;

    cl::Context context;
    cl::Device device;
    cl::CommandQueue queue;

    std::unique_ptr<VoxelisedObject> model_object;
    std::unique_ptr<OctahedronObject> source_object;
    std::unique_ptr<OctahedronObject> receiver_object;

    using Waveguide = RectangularWaveguide;
    //    using Waveguide = TetrahedralWaveguide;

    std::unique_ptr<MeshObject<Waveguide>> mesh_object;
    std::unique_ptr<Waveguide> waveguide;
    std::future<std::vector<cl_float>> future_pressure;
    std::thread waveguide_load_thread;

    std::unique_ptr<RaytraceObject> raytrace_object;
    std::future<RaytracerResults> raytracer_results;

    mutable std::mutex mut;
};

class SceneRenderer final : public OpenGLRenderer {
public:
    SceneRenderer();
    virtual ~SceneRenderer();
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void set_aspect(float aspect);
    void update_scale(float delta);
    static glm::mat4 get_projection_matrix(float aspect);

    glm::mat4 get_projection_matrix() const;
    glm::mat4 get_view_matrix() const;

    void set_rotation(float azimuth, float elevation);

    void load_from_file_package(const FilePackage& fp);

    glm::mat4 get_scale_matrix() const;

private:
    void update();
    void draw() const;
    std::unique_ptr<GenericShader> shader;

    std::unique_ptr<DrawableScene> scene;

    glm::mat4 projection_matrix;

    glm::mat4 rotation;
    float scale;
    glm::mat4 translation;

    std::mutex mut;
};
