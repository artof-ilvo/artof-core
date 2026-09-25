#include <Operation/ImplementControl.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include <boost/filesystem.hpp>
#include <Exceptions/FileExceptions.hpp>
#include <Utils/Geometry/Transform.h>
#include <Utils/Geometry/Angle.h>
#include <Utils/Geometry/Point.h>
#include <Utils/Logging/LoggerStream.h>

using namespace Ilvo::Core;
using namespace Ilvo::Exception;
using namespace Ilvo::Utils::Redis;
using namespace Ilvo::Utils::Geometry;
using namespace Ilvo::Utils::Settings;
using namespace Ilvo::Utils::Logging;

using namespace std;
using namespace Eigen;
using namespace nlohmann;
using namespace boost::filesystem;

ImplementControl::ImplementControl() : 
    platform(Platform::getInstance()),
    measuringDiscreteStarted(false),
    currentDiscrImplState(DRIVING)
{
}

void ImplementControl::init(Utils::Redis::VariableManager* manager, shared_ptr<Traject> traject, shared_ptr<PositionData> position) 
{
    // load the traject
    this->manager = manager;
    this->traject = traject;
    this->position = position;
    this->navModeMemory = manager->getVariable("pc.navigation.mode")->getValue<int>();
}

void ImplementControl::update(bool autoMode) 
{   
    disableImplement = manager->getVariable("pc.implement.disable")->getValue<bool>();

    // process
    for (Task& task: traject->getField().getTasks()) {  
        // update 
        task.updateState(manager); 
        bool continuousTask = std::find(continuousOperationTypes.begin(), continuousOperationTypes.end(), task.getType()) != continuousOperationTypes.end();

        if (task.isType("hitch")) {
            updateHitch(task);
            // Slow down when there is a hitch moving.
            manager->getVariable("pc.implement.slow_down")->setValue(task.getHitch().getHitchMoving());
        } else if (task.isType("discrete") && autoMode) {
            updateDiscrete(task); 
        } else if (task.isType("cardan") ) {
            updateCardan(task);
        } else if (continuousTask) {
            updateContinuous(task);
        }
    } 
}

void ImplementControl::reset()
{
    LoggerStream::getInstance() << INFO << "Resetting ImplementControl:";
    for (Task& task: traject->getField().getTasks()) { 
        LoggerStream::getInstance() << INFO << " - Resetting task: " << task.getName();
        task.updateState(manager); 

        if (task.isType("discrete")) {
            task.onDiscrReset(
                traject->closestPoint(task.getDiscreteReference()), 
                traject->getInterpolation());
            currentDiscrImplState = DRIVING;
            manager->getVariable("pc.navigation.mode")->setValue(navModeMemory);
        }

        task.getImplement().resetSections(); 
    }

    // Reset all the hitches
    for (Hitch& hitch: platform.hitches) {
        LoggerStream::getInstance() << INFO << " - Resetting hitch: " << hitch.getEntityName();
        string entityName = hitch.getEntityName();

        hitch.setActivateDiscrete(manager, false);
        hitch.setActivateCardan(manager, false);
        hitch.setActivateContinuous(manager, false);
        hitch.setActivate(manager, false);
        hitch.setBusy(manager, false);
    }

    // Reset other parameters
    LoggerStream::getInstance() << INFO << " - Resetting other parameters.";
    manager->getVariable("pc.implement.slow_down")->setValue(false);
}

void ImplementControl::updateHitch(Task& task) {
    string entityName = task.getHitch().getEntityName();
    bool active = false;
    bool foundImplementOperation = false;
    for (string operationType: continuousOperationTypes) {
        if (traject->getField().hasTaskWithTypeOnHitch(operationType, task.getHitch().name)) {
            Task& operationTask = traject->getField().getTaskWithTypeOnHitch(operationType, task.getHitch().name);
            Point fistOperationSection; 
            try {
                fistOperationSection = operationTask.getImplement().getSections().at(0)->getPolygon().center();
            } catch(const boost::geometry::centroid_exception& e) {
                LoggerStream::getInstance() << ERROR << "Section polygon is empty, " << e.what();
                break;
            } catch(const std::exception& e) {
                LoggerStream::getInstance() << ERROR << "Unexpected error to get fist ImplementControl section, " << e.what();
                break;
            }
            
            active = task.insideTaskMap(fistOperationSection, disableImplement);
            task.getHitch().setActivate(manager, active);
            return;
        }
    }

    // else for hitch or discrete task
    // if (task.isType("hitch")) {
    active = task.hitchInTaskMap()  && !disableImplement;
    task.getHitch().setActivate(manager, active);
    // }
}

void ImplementControl::updateContinuous(Task& task) 
{
    bool active = task.updateSections(manager, disableImplement);
    task.getHitch().setActivate(manager, active);
} 

void ImplementControl::updateCardan(Task& task) 
{
    bool active = task.cardanEnabled(manager, disableImplement);
    task.getHitch().setActivateCardan(manager, active);
} 

void ImplementControl::updateDiscrete(Task& task)
{
    // first execute onDiscrPoint to set implPoint properly
    double interpolationDistance = manager->getVariable("pc.purepursuit.inter_point_distance")->getValue<double>();
    double pathDistanceToNextPoint = task.distanceToNextDiscrPoint(traject->closestPoint(task.getDiscreteReference()), interpolationDistance);

    switch (currentDiscrImplState)
    {
    case DRIVING:
    {
        if (inRange(0.0, 1.5, pathDistanceToNextPoint)) {
            LoggerStream::getInstance() << DEBUG <<"pathDistanceToNextPoint: " << pathDistanceToNextPoint << " - DRIVING -> SLOW_DOWN";
            manager->getVariable("pc.implement.slow_down")->setValue(true);
            currentDiscrImplState = SLOW_DOWN;
        }
        break;
    }
    case SLOW_DOWN:
    {
            if (inRange(-1.5, 0.0, pathDistanceToNextPoint)) {
            uint8_t routine = task.getTaskMapRoutine();
            LoggerStream::getInstance() << DEBUG <<"pathDistanceToNextPoint: " << pathDistanceToNextPoint << " - SLOW_DOWN -> ROUTINE_0";
            LoggerStream::getInstance() << DEBUG <<"Discrete routine initiated: " << (int) routine;

            // ROUTINE 0 is the default routine, if the routine is > 0, we need to set the routine variable in the PLC and set the navigation mode to external
            if (routine == 0) {
                measuringDiscreteStarted = false;
                currentDiscrImplState = ROUTINE_0;
                task.activateSection("P", 1);  // Activate the section
                task.getHitch().setActivate(manager, true);
            } else {
                currentDiscrImplState = ROUTINE;
                // task.activateSection("P", routine);  // Activate the section << why does this not work?
                manager->getVariable("plc.control.hitch_rb.activate_sections.0")->setValue(routine);
                navModeMemory = manager->getVariable("pc.navigation.mode")->getValue<int>();
                manager->getVariable("pc.navigation.mode")->setValue(5); // set navigation mode to external
                LoggerStream::getInstance() << DEBUG <<"Discrete routine initiated: " << (int) routine << " - ROUTINE_0 -> ROUTINE";
            }

            // Set the next discrete point
            task.incrDiscrPoint(); // increment the discrete point
        } else if (abs(pathDistanceToNextPoint) > 1.5) {
            currentDiscrImplState = DRIVING;
        }
        break;
    }
    case ROUTINE_0:
    {
        // Default routine
        // generate block pulse of 500ms
        if (!measuringDiscreteStarted) {            
            if ( pulseGenerator.generatePulse(500ms) ) {
                task.getHitch().setActivateDiscrete(manager, true);
            } else {
                measuringDiscreteStarted = true;
            }
        } else {
            task.getHitch().setActivateDiscrete(manager, false);
            bool discreteImplementActive = manager->getVariable("plc.monitor." + task.getHitch().getEntityName() + ".busy")->getValue<bool>();

            busyDiscrImplEdge.detect(discreteImplementActive);
            if (busyDiscrImplEdge.falling) {
                LoggerStream::getInstance() << DEBUG <<"ROUTINE_0 -> DRIVING";
                manager->getVariable("pc.implement.slow_down")->setValue(false);
                task.getHitch().setActivate(manager, false);
                task.activateSection("P", 0);  // Deactivate the section
                currentDiscrImplState = DRIVING;
            }
        }
        
        break;
    }
    case ROUTINE:
    {    
        // Routine of external controller
        bool routinetActive = manager->getVariable("plc.control.hitch_rb.activate_sections.0")->getValue<int>(); 

        routineEdge.detect(routinetActive);
        if (routineEdge.falling) { 
            LoggerStream::getInstance() << DEBUG <<"ROUTINE -> DRIVING";
            manager->getVariable("pc.implement.slow_down")->setValue(false);
            // task.activateSection("P", routine); << why does this not work?
            manager->getVariable("plc.control.hitch_rb.activate_sections.0")->setValue(0);
            manager->getVariable("pc.navigation.mode")->setValue(navModeMemory); // set navigation mode to normal
            currentDiscrImplState = DRIVING;
        }

        break;
    }
    default:
        break;
    }
}
