/**
 * @file FsmGTurn.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief State machine of navigation mode PP_SPINNING_180
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
     * @brief G-Turn for cornering (navigation mode PP_SPINNING_180)
     * 
     * @details The robot spins for 180 degrees when reaching the first corner drives sideways to the second corner and starts driving forward again when reaching the second corner. 
     * Note: applicable for 4ws4wd robots.
     */
    class FsmGTurn: public Fsm
    {
    public:
        using Fsm::Fsm;

        void update() override;
    };

} // Core
} // Ilvo
