#include "rasterizer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

#include <algorithm>

#include "segment.h"
#include "camera.h"

Rasterizer::Rasterizer(const std::vector<Polygon>& polygons)
    : m_polygons(polygons)
{}

QImage Rasterizer::RenderScene()
{
    QImage result(512, 512, QImage::Format_RGB32);
    // Fill the image with black pixels.
    // Note that qRgb creates a QColor,
    // and takes in values [0, 255] rather than [0, 1].

    result.fill(qRgb(0.f, 0.f, 0.f));

    //initializing Z buffer to store Z coordinates
    //dimensions W x H pixels
    std::array<float, 512 * 512> zBuffer;
    std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<float>::max());

    //CAMERA: view and projection matrices
    glm::mat4 viewMatrix = getCamera().getViewMatrix();
    glm::mat4 projectionMatrix = getCamera().getProjectionMatrix();


    //for each Polygon P
    for (Polygon p : this->m_polygons){
        //for each Triangle t
        for (Triangle t :  p.m_tris) {
            //get vertices of t
            unsigned int vertex_1_index = t.m_indices[0];
            unsigned int vertex_2_index = t.m_indices[1];
            unsigned int vertex_3_index = t.m_indices[2];


            Vertex vertex1 = p.m_verts.at(vertex_1_index);
            Vertex vertex2 = p.m_verts.at(vertex_2_index);
            Vertex vertex3 = p.m_verts.at(vertex_3_index);

            //**3D Rasterization: CAMERA [UNCOMMENT THE 3 LINES BELOW TO HAVE 2D RASTERIZATION RUN AGAIN] **
            //transform vertices from world space to camera space
            vertex1.m_pos = worldSpaceToScreenSpace(vertex1.m_pos, viewMatrix, projectionMatrix, 512, 512);
            vertex2.m_pos = worldSpaceToScreenSpace(vertex2.m_pos, viewMatrix, projectionMatrix, 512, 512);
            vertex3.m_pos = worldSpaceToScreenSpace(vertex3.m_pos, viewMatrix, projectionMatrix, 512, 512);

            //compute bounding box of T
            BoundingBox bb;

            //bottom left corner of the bounding box
            bb.minX = std::min({vertex1.m_pos.x, vertex2.m_pos.x, vertex3.m_pos.x});
            bb.minY = std::min({vertex1.m_pos.y, vertex2.m_pos.y, vertex3.m_pos.y});

            //top right corner of the bounding box
            bb.maxX = std::max({vertex1.m_pos.x, vertex2.m_pos.x, vertex3.m_pos.x});
            bb.maxY = std::max({vertex1.m_pos.y, vertex2.m_pos.y, vertex3.m_pos.y});

            //clamp bounding box to screen
            bb.ClampToScreen(512, 512);

            //array of Segments representing 3 edges of triangle
            std::array<Segment, 3> segments = {
                Segment(vertex1.m_pos, vertex2.m_pos),
                Segment(vertex2.m_pos, vertex3.m_pos),
                Segment(vertex3.m_pos, vertex1.m_pos)
            };

            //iterate over y coordinates within bounding box (these are our pixel rows)
            for (int y = bb.minY; y <= bb.maxY; y++){

                float xLeft = 512; //initialized to screenwidth
                float xRight = 0; //initialized to minimum screen

                //iterate over a collection of line segments
                for (Segment &segment : segments){

                    float xIntersection;

                    // For a given triangle, we want to find the min and max X intercept with our pixel row
                    //one of these intersections will be outside of the box

                    //Need to make sure the pixel row only tests for intersection with edges it would overlap within the bounding box
                    //If the pixel row’s Y coord is between the Y coords of an edge’s endpoints, it will overlap the edge within the bounding box

                    //if segment intersects with y value
                    if(segment.getIntersection(y, &xIntersection)){
                        // set xLeft to the minimum of itself and the x-intersection with the Segment
                        xLeft = std::min(xLeft, xIntersection);
                        // set xRight to the maximum of itself and the x-intersection
                        xRight = std::max(xRight, xIntersection);
                    }
                }
                //double check that values are not being drawn out of screen size of 512x512
                //limit is (0, 512)
                xLeft = std::max(0.0f, xLeft);
                xRight = std::min(511.0f, xRight);

                //drawing pixels for particular row
                for (int x = static_cast<int>(xLeft); x <= static_cast<int>(xRight); x++){

                    glm::vec4 point = glm::vec4(x, y, 0, 0);

                    //** 2D barycentric interpolation; UNCOMMENT for 2D RASTERIZATION **
                    //glm::vec3 barycentricinterpolation = BarycentricInterpolation(vertex1.m_pos, vertex2.m_pos, vertex3.m_pos, point);

                    //** 3D barycentric interpolation for 3D RASTERIZATION **
                    //interpolate each fragment's Z with correct perspective distortion, then interpolate each fragment's UVs with correct perspective distortion
                    glm::vec3 barycentricinterpolation = BarycentricInterpolation3D(vertex1.m_pos, vertex2.m_pos, vertex3.m_pos, point);

                    //interpolate color (used for 2D RASTERIZATION)
                    glm::vec3 colorinterpolation = interpolateColor(vertex1.m_color, vertex2.m_color, vertex3.m_color, barycentricinterpolation);

                    //check Z values; access the element corresponding to (x, y) as array[x + W * y] for 2D
                    int zBufferIndex = x + 512 * y;

                    //calculate depth
                    float interpolatedDepth = barycentricinterpolation.x * vertex1.m_pos.z
                                              + barycentricinterpolation.y * vertex2.m_pos.z
                                              + barycentricinterpolation.z * vertex3.m_pos.z;

                    //** UV interpolation **
                    glm::vec2 uv1 = vertex1.m_uv;
                    glm::vec2 uv2 = vertex2.m_uv;
                    glm::vec2 uv3 = vertex3.m_uv;
                    glm::vec2 interpolatedUV = interpolateUV(uv1, uv2, uv3, barycentricinterpolation);
                    glm::vec3 textureColor = GetImageColor(interpolatedUV, p.mp_texture);

                    //** LAMBERT **
                    // Normal interpolation
                    glm::vec4 normal = interpolateNormals(vertex1.m_normal, vertex2.m_normal, vertex3.m_normal, barycentricinterpolation);

                    float lambertColor = lambert(m_camera, normal);

                    glm::vec3 lambertTextureColor = lambertColor * textureColor;

                    //use the color of the fragment with the smallest Z-coordinate
                    if (interpolatedDepth < zBuffer[zBufferIndex]){
                        zBuffer[zBufferIndex] = interpolatedDepth;

                        // ** UNCOMMENT FOR 2D RASTERIZATION **
                        //result.setPixel(x, y, qRgb(colorinterpolation.r, colorinterpolation.g, colorinterpolation.b));

                        //3D: [NO lambert shading]
                        //result.setPixel(x, y, qRgb(textureColor.r, textureColor.g, textureColor.b));

                        //3D: lambert shading
                        //clamp values
                        result.setPixel(x, y, qRgb(glm::clamp(lambertTextureColor.r, 0.0f, 255.0f), glm::clamp(lambertTextureColor.g, 0.0f, 255.0f), glm::clamp(lambertTextureColor.b, 0.0f, 255.0f)));
                    }
                }
            }
        }
    }
    return result;
}

//Barycentric interpolation

glm::vec3 Rasterizer::BarycentricInterpolation (glm::vec4& v1, glm::vec4& v2, glm::vec4& v3, glm::vec4& point) {
    // calculating areas
    //area of a triangle = length(cross(P1-P2, P3-P2))/2

    //dot product cannot be computed on vec4, must be vec3
    //convert input to vec3
    //z coordinates treated as 0
    glm::vec3 p1 = glm::vec3(v1.x, v1.y, 0.0f);
    glm::vec3 p2 = glm::vec3(v2.x, v2.y, 0.0f);
    glm::vec3 p3 = glm::vec3(v3.x, v3.y, 0.0f);
    glm::vec3 p = glm::vec3(point.x, point.y, 0.0f);

    //compute areas using a cross product
    //float totalAreaCross = glm::length(glm::cross(p2 - p1, p3 - p1)) * 0.5f;

    float s1 = glm::length(glm::cross(p2 - p, p3 - p)) * 0.5f;
    float s2 = glm::length(glm::cross(p3 - p, p1 - p)) * 0.5f;
    float s3 = glm::length(glm::cross(p1 - p, p2 - p)) * 0.5f;

    float s = s1 + s2 + s3;

    float s1s = s1/s;
    float s2s = s2/s;
    float s3s = s3/s;

    //return barycentric coordinates
    return glm::vec3(s1s, s2s, s3s);
}

glm::vec3 Rasterizer::interpolateColor(glm::vec3& v1Color, glm::vec3& v2Color, glm::vec3& v3Color, glm::vec3& barycentricInfluence){
    return barycentricInfluence.x * v1Color + barycentricInfluence.y * v2Color + barycentricInfluence.z * v3Color;
}

glm::vec4 Rasterizer::worldSpaceToScreenSpace(const glm::vec4& worldVertex, const glm::mat4& view, const glm::mat4& projection, int screenWidth, int screenHeight){
    //view * p
    glm::vec4 cameraSpace = view * worldVertex;

    //proj * p
    glm::vec4 unhomogenizedScreenSpace = projection * cameraSpace;

    // P/=Uw
    glm::vec4 screenSpace = unhomogenizedScreenSpace /unhomogenizedScreenSpace.w;

    //from screen space to pixel space
    float Px = (screenWidth * (screenSpace.x + 1.0f)) * 0.5f;
    float Py = (screenHeight * (1.0f - screenSpace.y)) * 0.5f;

    //return Point in Pixel Space, P = (Px, Py, Pz, Pw)
    return glm::vec4(Px, Py, screenSpace.z, 1.0f);
}

//3D Barycentric Interpolation
glm::vec3 Rasterizer::BarycentricInterpolation3D(glm::vec4& v1, glm::vec4& v2, glm::vec4& v3, glm::vec4& point){
    // calculating areas
    //area of a triangle = length(cross(P1-P2, P3-P2))/2

    //dot product cannot be computed on vec4, must be vec3
    //convert inptut to vec3
    //z coordinates treated as 0
    glm::vec3 p1 = glm::vec3(v1.x, v1.y, 0.0f);
    glm::vec3 p2 = glm::vec3(v2.x, v2.y, 0.0f);
    glm::vec3 p3 = glm::vec3(v3.x, v3.y, 0.0f);
    glm::vec3 p = glm::vec3(point.x, point.y, 0.0f);

    //compute areas using a cross product
    //float totalAreaCross = glm::length(glm::cross(p2 - p1, p3 - p1)) * 0.5f;

    float s1 = glm::length(glm::cross(p2 - p, p3 - p)) * 0.5f;
    float s2 = glm::length(glm::cross(p3 - p, p1 - p)) * 0.5f;
    float s3 = glm::length(glm::cross(p1 - p, p2 - p)) * 0.5f;

    float s = s1 + s2 + s3;

    float s1s = s1/(s * v1.w);
    float s2s = s2/(s * v2.w);
    float s3s = s3/(s * v3.w);

    //return barycentric coordinates
    return glm::vec3(s1s, s2s, s3s);
}

glm::vec2 Rasterizer::interpolateUV(glm::vec2& v1UV, glm::vec2& v2UV, glm::vec2& v3UV, glm::vec3& barycentricInfluence) {
    return barycentricInfluence.x * v1UV + barycentricInfluence.y * v2UV + barycentricInfluence.z * v3UV;
}

glm::vec4 Rasterizer::interpolateNormals(glm::vec4& v1normal, glm::vec4& v2normal, glm::vec4& v3normal, glm::vec3& barycentricInfluence) {
    return barycentricInfluence.x * v1normal + barycentricInfluence.y * v2normal + barycentricInfluence.z * v3normal;
}

float Rasterizer::lambert(const Camera& camera, const glm::vec4& normal) {
    float ambientTerm = 0.3f;

    //source of light will be the negative of the camera's vector
    glm::vec4 lightVector = -(camera.forward);

    //normalize camera's forward vector
    glm::vec4 normalizedForward = glm::normalize(lightVector);

    float lightOnSinglePoint = glm::clamp(glm::dot(normalizedForward, normal), 0.0f, 1.0f);

    return ambientTerm + lightOnSinglePoint;
}

void Rasterizer::ClearScene() {
    m_polygons.clear();
}

