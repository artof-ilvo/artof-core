/**
 * @file FsmPurePursuit.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief State machine of navigation mode PURE_PP
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
     * @brief Pure pursuit without spinning (navigation mode PURE_PP)
     * 
     * @details The robot follows the traject with pure pursuit only, also through the corners.
     */
    class FsmPurePursuit: public Fsm
    {
    public:
        using Fsm::Fsm;

        void update() override;
    };

} // Core
} // Ilvo
