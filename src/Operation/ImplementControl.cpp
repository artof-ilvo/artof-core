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
        } else if (task.isType("discrete") && autoMode) {
            updateDiscrete(task); 
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
            traject->onDiscrReset(task.getHitch().getState().asAffine());
        }

        task.getImplement().resetSections(); 
    }

    // Reset all the hitches
    for (Hitch& hitch: manager->getPlatform().hitches) {
        LoggerStream::getInstance() << INFO << " - Resetting hitch: " << hitch.getEntityName();
        string entityName = hitch.getEntityName();

        // discrete
        manager->getVariable("plc.control." + entityName + ".activate_discrete")->setValue(false);

        // hitch
        manager->getVariable("plc.control." + entityName + ".activate")->setValue(false);

        // cardan
        manager->getVariable("plc.control." + entityName + ".activate_cardan")->setValue(false);

        // continous ImplementControl
        manager->getVariable("plc.control." + entityName + ".activate_continuous")->setValue(false);

        // busy ImplementControl
        manager->getVariable("plc.monitor." + entityName + ".busy")->setValue(false);
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
            manager->getVariable("plc.control." + entityName + ".activate")->setValue(active); 
            return;
        }
    }

    // else for hitch or discrete task
    active = task.hitchInTaskMap()  && !disableImplement;
    manager->getVariable("plc.control." + entityName + ".activate")->setValue(active); 
}

void ImplementControl::updateContinuous(Task& task) 
{
    string activateName = "plc.control." + task.getHitch().getEntityName() + (task.isType("cardan") ? ".activate_cardan" : ".activate_continuous");

    bool active = false;
    active = task.updateSections(manager, disableImplement);

    manager->getVariable(activateName)->setValue(active);
} 

void ImplementControl::updateDiscrete(Task& task)
{
    // first execute onDiscrPoint to set implPoint properly
    double interpolationDistance = manager->getVariable("pc.purepursuit.inter_point_distance")->getValue<double>();
    double pathDistanceToNextPoint = traject->distanceToNextDiscrPoint(task, interpolationDistance);
    task.activateSection("P", currentDiscrImplState == MEASURING);

    switch (currentDiscrImplState)
    {
    case DRIVING:
        if (inRange(0.0, 1.5, pathDistanceToNextPoint)) {
            LoggerStream::getInstance() << DEBUG <<"pathDistanceToNextPoint: " << pathDistanceToNextPoint << " - DRIVING -> SLOW_DOWN";
            manager->getVariable("pc.implement.slow_down")->setValue(true);
            currentDiscrImplState = SLOW_DOWN;
        }
        break;
    case SLOW_DOWN:
        if (inRange(-1.5, 0.0, pathDistanceToNextPoint)) {
            LoggerStream::getInstance() << DEBUG <<"pathDistanceToNextPoint: " << pathDistanceToNextPoint << " - SLOW_DOWN -> MEASURING";
            traject->incrDiscrPoint(task); // increment the discrete point
            measuringDiscreteStarted = false;
            manager->getVariable("plc.control." + task.getHitch().getEntityName() + ".activate")->setValue(true);
            currentDiscrImplState = MEASURING;
        } else if (abs(pathDistanceToNextPoint) > 1.5) {
            currentDiscrImplState = DRIVING;
        }
        break;
    case MEASURING:
        // generate block pulse of 500ms
        if (!measuringDiscreteStarted) {            
            if ( pulseGenerator.generatePulse(500ms) ) {
                manager->getVariable("plc.control." + task.getHitch().getEntityName() + ".activate_discrete")->setValue(true);
            } else {
                measuringDiscreteStarted = true;
            }
        } else {
            manager->getVariable("plc.control." + task.getHitch().getEntityName() + ".activate_discrete")->setValue(false);
            bool discreteImplementActive = manager->getVariable("plc.monitor." + task.getHitch().getEntityName() + ".busy")->getValue<bool>();

            busyDiscrImplEdge.detect(discreteImplementActive);
            if (busyDiscrImplEdge.falling) {
                LoggerStream::getInstance() << DEBUG <<"MEASURING -> DRIVING";
                manager->getVariable("pc.implement.slow_down")->setValue(false);
                manager->getVariable("plc.control." + task.getHitch().getEntityName() + ".activate")->setValue(false);
                currentDiscrImplState = DRIVING;
            }
        }
        
        break;
    default:
        break;
    }
}
