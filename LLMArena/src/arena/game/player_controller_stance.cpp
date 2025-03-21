// src/arena/game/player_controller_stance.cpp
#include "../include/arena/game/player_controller.h"
#include <QDebug>
#include <QMutex>

// External reference to movement mutex
extern QMutex playerMovementMutex;

void PlayerController::beginStanceTransition(PlayerStance newStance) {
    if (stance == newStance || inStanceTransition)
        return;
    
    // Already set target stance
    targetStance = newStance;
    inStanceTransition = true;
    
    // Set transition time based on current stance and target
    int transitionTime = 0;
    
    if (stance == PlayerStance::Prone) {
        if (targetStance == PlayerStance::Standing) {
            transitionTime = 1000; // 1 second from prone to standing
        } else if (targetStance == PlayerStance::Crouching) {
            transitionTime = 500; // 0.5 seconds from prone to crouch
        }
    } else if (stance == PlayerStance::Crouching) {
        if (targetStance == PlayerStance::Standing) {
            transitionTime = 300; // 0.3 seconds from crouch to standing
        } else if (targetStance == PlayerStance::Prone) {
            transitionTime = 500; // 0.5 seconds from crouch to prone
        }
    } else if (stance == PlayerStance::Standing) {
        if (targetStance == PlayerStance::Crouching) {
            transitionTime = 200; // 0.2 seconds from standing to crouch
        } else if (targetStance == PlayerStance::Prone) {
            transitionTime = 800; // 0.8 seconds from standing to prone
        }
    }
    
    // Start the timer if there's a transition time
    if (transitionTime > 0) {
        stanceTransitionTimer.start(transitionTime);
    } else {
        // Complete immediately if no transition time
        completeStanceTransition();
    }
}

void PlayerController::completeStanceTransition() {
    if (!inStanceTransition)
        return;
    
    stance = targetStance;
    inStanceTransition = false;
    
    emit stanceChanged(stance);
}