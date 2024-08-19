/**
 * @file IlvoJob.h
 * @author Axel Willekens (axel.willekens@ilvo.vlaanderen.be)
 * @brief 
 * @version 0.1
 * @date 2024-03-20
 * 
 * @copyright Copyright (c) 2024 Flanders Research Institute for Agriculture, Fisheries and Food (ILVO)
 * 
 */

#pragma once

#include <string>
#include <chrono>

#include <ThirdParty/json.hpp>
#include <System/IlvoJobData.h>

namespace Ilvo {
namespace Core {
    /**
     * @brief Abstraction for an add-on (docker) or process (process)
     * 
     */
    class IlvoJob
    {
    protected:
        IlvoJobData data;
        std::chrono::system_clock::time_point startTime;
    public:
        IlvoJob(nlohmann::json ilvoJob);
        ~IlvoJob() = default;

        /**
         * @brief The job is updated based on context extracted from the redis database.
         * 
         * @param updateJob 
         * @return true 
         * @return false 
         */
        bool update(IlvoJobData updateJob);

        IlvoJobData& getData();

        virtual nlohmann::json toJson() = 0;
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool runs() = 0;
        virtual void updateSoftware() = 0;
    };
    
}
}