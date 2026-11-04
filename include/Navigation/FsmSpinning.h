/**
 * @file FsmSpinning.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief State machine of navigation mode PP_SPINNING_90
 * @version 0.1
 * @date 2026-11-04
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#pragma once

#include <Navigation/Fsm.h>

namespace Ilvo {
namespace Core {

    /** 
     * @brief Spinning for cornering (navigation mode PP_SPINNING_90)
     * 
     * @details The robot spins for the angle of the corner during cornering. 
     * Note: applicable for 4ws4wd robots or skid steering robots.
     */
    class FsmSpinning: public Fsm
    {
    public:
        using Fsm::Fsm;

        void update() override;
    };

} // Core
} // Ilvo
