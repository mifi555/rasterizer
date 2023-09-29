//#ifndef CAMERA_H
//#define CAMERA_H

//#endif // CAMERA_H

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class Camera {
public:
    glm::vec4 forward; //represents camera's forward direction Z axis. Default value of <0, 0, -1, 0>.
    glm::vec4 right; // represents camera's right direction X axis. Default value of <1, 0, 0, 0>.
    glm::vec4 up; //represents camera's up direction Y axis. Default value of <0, 1, 0, 0>.

    float fov; //camera's field of view. Default value of 45 degrees
    glm::vec4 position; //camera's position in the world space. Default value of <0, 0, 10, 1>

    float nearClip; //near clip plane. Default value of 0.01.
    float farClip; //far clip plane. Default value of 100.0.
    float aspectRatio; //caemra's aspect ratio. Default value of 1.0

public:
    Camera() :
        forward(0, 0, -1, 0),
        right(1, 0, 0, 0),
        up(0, 1, 0, 0),
        fov(45.0f),
        position(0, 0, 10, 1),
        nearClip(0.01f),
        farClip(100.0f),
        aspectRatio(1.0f) {
    }


    // A function which returns a view matrix based on the camera's local axes and position.
    // view matrix: O * T

    // O:
    // [Rx, Ry, Rz, 0]
    // [Ux, Uy, Uz, 0]
    // [Fx, Fy, Fz, 0]
    // [0, 0, 0, 1]

    // T:
    // [1, 0, 0, -eyex]
    // [0, 1, 0, -eyey]
    // [0, 0, 1, -eyez]
    // [0, 0, 0, 1]

    glm::mat4 getViewMatrix() {
        glm::mat4 O(1.0f);

        O[0][0] = right.x;
        O[1][0] = right.y;
        O[2][0] = right.z;

        O[0][1] = up.x;
        O[1][1] = up.y;
        O[2][1] = up.z;

        O[0][2] = forward.x;
        O[1][2] = forward.y;
        O[2][2] = forward.z;

        glm::mat4 T(1.0f);

        T[3][0] = -position.x;
        T[3][1] = -position.y;
        T[3][2] = -position.z;

        T[3][3] = 1.0f;

        glm::mat4 viewMatrix = O * T;

        return viewMatrix;
    }

    // A function which returns a perspective projection matrix based on the camera's clipping planes, aspect ratio, and field of view.
    glm::mat4 getProjectionMatrix() {
        glm::mat4 projectionMatrix(0.0f);

        projectionMatrix[0][0] = 1/(aspectRatio * tan(fov/2));
        projectionMatrix[1][1] = 1/tan(fov/2);
        projectionMatrix[2][2] = farClip / (farClip - nearClip);
        projectionMatrix[2][3] = 1.0f;
        projectionMatrix[3][2] = (-farClip * nearClip)/(farClip - nearClip);

        return projectionMatrix;
    }

    //Three functions that translate the camera along each of its local axes, both forward and backward.
    //The amount of translation should be determined by an input to the function.

    void translateForward(float magnitude) {
        position += forward * magnitude;
    }

    void translateRight(float magnitude) {
        position += right * magnitude;
    }

    void translateUp(float magnitude) {
        position += up * magnitude;
    }

    //Three functions that rotate the camera about each of its local axes.
    //Note that these functions should only alter the orientation of the camera; its position should not change.
    //The amount of rotation should be determined by an input to the function.

    void rotateForward(float degree) {
        //starting with an identity matrix (i.e., no transformation) and then applying a rotation to it.
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(degree), glm::vec3(forward));
        right = rotationMatrix * right;
        up = rotationMatrix * up;
    }

    void rotateRight(float degree) {
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(degree), glm::vec3(right));
        forward = rotationMatrix * forward;
        up = rotationMatrix * up;

    }

    void rotateUp(float degree) {
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(degree), glm::vec3(up));
        forward = rotationMatrix * forward;
        right = rotationMatrix * right;
    }

};
