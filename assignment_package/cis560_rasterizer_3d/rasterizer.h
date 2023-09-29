#pragma once
#include <polygon.h>
#include <QImage>
#include <camera.h>

struct BarycentricResult {
    glm::vec3 coordinates;
    glm::vec3 homogeneous;
};

class Rasterizer
{
private:
    //This is the set of Polygons loaded from a JSON scene file
    std::vector<Polygon> m_polygons;
    Camera m_camera;

public:
    Rasterizer(const std::vector<Polygon>& polygons);

    QImage RenderScene();
    void ClearScene();

    //Barycentric interpolation
    glm::vec3 BarycentricInterpolation (glm::vec4& v1, glm::vec4& v2, glm::vec4& v3, glm::vec4& point);

    //Color interpolation
    glm::vec3 interpolateColor(glm::vec3& v1Color, glm::vec3& v2Color, glm::vec3& v3Color, glm::vec3& barycentricInfluence);


    //3D

    glm::vec4 worldSpaceToScreenSpace(const glm::vec4& worldVertex, const glm::mat4& view, const glm::mat4& projection, int screenWidth, int screenHeight);

    //camera getter
    Camera& getCamera() {
        return m_camera;
    }

    glm::vec3 BarycentricInterpolation3D (glm::vec4& v1, glm::vec4& v2, glm::vec4& v3, glm::vec4& point);

    glm::vec2 interpolateUV(glm::vec2& v1UV, glm::vec2& v2UV, glm::vec2& v3UV, glm::vec3& barycentricInfluence);
};
