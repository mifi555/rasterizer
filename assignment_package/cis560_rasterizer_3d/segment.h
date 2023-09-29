//#ifndef SEGMENT_H
//#define SEGMENT_H

//#endif // SEGMENT_H

#pragma once
#include <glm/glm.hpp>
#include <algorithm>


// used to determine which pixels on a row triangle overlap
// each Segment instance will represent one edge of a triangle
class Segment {
private:
    glm::vec2 endpoint1;
    glm::vec2 endpoint2;

    float slope;

public:
    //Constructor
    Segment(const glm::vec4& p1, const glm::vec4&p2) :
        endpoint1(p1), endpoint2(p2) {

        //if x endpoints of endpoints are different, there's a slope
        if (endpoint2.x - endpoint1.x != 0){
            slope = (endpoint2.y - endpoint1.y) / (endpoint2.x - endpoint1.x);
        }
//        else {
//            // handle case when slope is vertical
//            slope = std::numeric_limits<float>::infinity();
//        }

    }
    // computes the x-intersection of the line segment with a horizontal line
    // based on the horizontal line's y-coordinate.
    //returns a boolean indicating whrther lines intersect at all
    bool getIntersection(int y, float* x) {

        //Case: segment is horizontal, and therefore cannot intersect with horizontal line
        if(endpoint1.y == endpoint2.y) {
            return false;
        }
        // line is outside of segment's bounds:
        if(y < std::min(endpoint1.y, endpoint2.y)) {
            return false;
        }

        if(y > std::max(endpoint1.y, endpoint2.y)) {
            return false;
        }

        // edge is vertical (x coordinate of both endpoints is the same)
        if(endpoint1.x == endpoint2.x){
            *x = endpoint1.x;
            return true;
        }

        // point slope formula:
        // y - y_1 = m (x - x_1)
        // x = (y - y_1)/m + x_1

        *x = (y - endpoint1.y)/slope + endpoint1.x;

        return true;
    }
};
