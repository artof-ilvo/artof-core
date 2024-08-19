/**
 * @file GpsDevice.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief GPS driver
 * @version 0.1
 * @date 2024-03-19
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#include <Utils/Timing/Clk.h>
#include <Utils/Geometry/Transform.h>
#include <Utils/Settings/Platform.h>
#include <Utils/Settings/State.h>
#include <Utils/Peripheral/Peripheral.h>
#include <ThirdParty/UTM.hpp>

#include <Utils/Redis/VariableManager.h>
#include <Gps/Stonex.h>
#include <Gps/Simplertk3b.h>


namespace Ilvo {
namespace Core {

    /**
     * @brief GPS device variable manager
     * 
     * @details The GPS driver variable manager contains the GPS driver peripheral and the GPS variables.
     * It measures the GPS position and updates the transformation to the UTM coordinate system constantly
     * and also updates the state of the platform, i.e. the robot center state, the hitch states, etc.
     */
    class GpsDevice: public Utils::Redis::VariableManager
    {
    private:
        /** @brief Peripheral of the GPS device */
        std::unique_ptr<Utils::Peripheral::Peripheral> peripheral;
        /** @brief Variable keeps track if GPS is found */
        bool gpsfound;

        /** @brief Translation matrix */
        Eigen::Vector3d rawR;
        /** @brief Rotation matrix */
        Eigen::Vector3d rawT;
        /** @brief Translation matrix covariance */
        Eigen::Vector3d rawRCov;
        /** @brief Rotation matrix covariance */
        Eigen::Vector3d rawTCov;
    public:
        GpsDevice(const std::string ns);
        ~GpsDevice() = default;
 
        void init() override;
        void serverTick() override;
    };

} // Core
} // Ilvo