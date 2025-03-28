// If we still couldn't find an escape, as a last resort,
// try moving upward slightly which often helps with vertical walls
if (glm::length(potentialEscapeDir) < 0.001f) {
    glm::vec3 upwardEscapePos = stuckPosition + glm::vec3(0.0f, 0.1f, 0.0f);
    if (!collidesWithBlocks(upwardEscapePos, velocity, world, false)) {
        nextPos = upwardEscapePos;
        if (verboseDebug) {
            std::cout << "Applied upward escape as last resort" << std::endl;
        }
    } else {
        // Can't move in any direction, stop completely at current position
        break;
    }
}

// End of nested blocks
                                    }
                                }
                            }
                        } else {
                            // Neither stepping up nor sliding works
                            break;
                        }
                    } else {
                        // Not near ground and can't slide on either axis
                        break;
                    } 