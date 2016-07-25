#pragma once

#include "boundaries.h"
#include "geometric.h"
#include "scene_data.h"

#include "common/aligned/vector.h"

class Octree final {
public:
    Octree() = default;
    Octree(const CopyableSceneData& mesh_boundary,
           size_t max_depth,
           float padding = 0);
    Octree(const CopyableSceneData& mesh_boundary,
           size_t max_depth,
           const aligned::vector<size_t>& to_test,
           const CuboidBoundary& aabb);

    CuboidBoundary get_aabb() const;
    bool has_nodes() const;
    const std::array<Octree, 8>& get_nodes() const;
    const aligned::vector<size_t>& get_triangles() const;

    size_t get_side() const;

    aligned::vector<const Octree*> intersect(const geo::Ray& ray) const;
    const Octree& get_surrounding_leaf(const glm::vec3& v) const;

private:
    CuboidBoundary aabb;
    aligned::vector<size_t> triangles;
    std::unique_ptr<std::array<Octree, 8>> nodes;
};
