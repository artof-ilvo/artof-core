/**
 * @file FsmRollback.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief State machine of navigation mode PP_ROLL_BACK
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
     * @brief The robot turns and rolls back (navigation mode PP_ROLL_BACK)
     * 
     * @details The robot starts turning when approaching a corner. If the second corner is closer then its turning radius. It rolls back to get in the right postition to take this corner.
     */
    class FsmRollback: public Fsm
    {
    public:
        using Fsm::Fsm;

        void update() override;
    };

} // Core
} // Ilvo
