#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <glm/glm.hpp>  // Explicit GLM include

class Player {
public:
    Player() : position(0, 10, 0), direction(0, 0, 1) {}
    glm::vec3 position;
    glm::vec3 direction;
    float height = 1.75f; // 5 ft 9 in
};

#endif