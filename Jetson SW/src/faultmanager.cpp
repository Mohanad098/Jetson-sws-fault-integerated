// FaultManager.cpp

#include "faultmanager.h"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace gr = boost::gregorian;

//constructor
FaultManager::FaultManager(ThreadSafeQueue<std::string>& commandsQueue,ThreadSafeQueue<std::string>& faultsQueue) 
: commandsQueue(commandsQueue), faultsQueue(faultsQueue), running(false)
{
    // Create the fault logs directory if it doesn't exist
    logDir = "faultlogs";
    if (!fs::exists(logDir)) 
    {
        fs::create_directory(logDir);
    }

    // Open the log file
    std::string logFilename = generateLogFilename();
    faultLogFile.open(logDir / logFilename, std::ios::app);
    if (!faultLogFile.is_open()) 
    {
        std::cerr << "Failed to open fault log file" << std::endl;
    }
}

//destructor
FaultManager::~FaultManager() {
    faultstop();
    if (faultLogFile.is_open()) 
    {
        faultLogFile.close();
    }
}

// Function to generate a log filename with timestamp
std::string FaultManager::generateLogFilename() {
    pt::ptime now = pt::second_clock::local_time();
    std::ostringstream filename;
    filename << "fault_log_"
             << gr::to_iso_extended_string(now.date()) << "_"
             << std::setw(2) << std::setfill('0') << now.time_of_day().hours() << "-"
             << std::setw(2) << std::setfill('0') << now.time_of_day().minutes()
             << ".txt";
    return filename.str();
}

//starting the fault manager thread
void FaultManager::faultstart()
{
    if (running) {
        std::cerr << "Fault manager is already running." << std::endl;
        return;
    }
    running = true;
    faultsthread = std::thread(&FaultManager::faultfind, this);
}


//function to release the thread ( add any cleanup needed )
void FaultManager::faultstop() {
    running = false;
    if (faultsthread.joinable()) {
        faultsthread.join();
    }
}


//function to poll on queue to check for any faults sent
void FaultManager::faultfind()
{
    std::string fault;
    while (running)
    {
        if (faultsQueue.tryPop(fault))
        {
            std::cout << "Received fault in the fault manager "<< fault << std::endl;
            this->faulthandling(fault);
            logFault(fault);
        }
    }
}


//function to identify the fault sent
void FaultManager::faulthandling(const std::string &fault)
{

    std::string command;

    /********************** Camera component fault *******************/

    //Camera can't connect
    if (fault == "Camera_disconnected")
    {
        command = "Read_video";         //Read from video file
        std::cout<<"Live feed not found, connecting to video"<<std::endl;
        commandsQueue.push(command);
    }


    /************************* Commtcp faults ****************************/
    //FPS more than max threshold or less than min threshold
    else if (fault.find("SET_FPS") != std::string::npos)
    {
        std::cout << "Incorrect FPS sent" << std::endl;
        size_t pos = fault.find(":");
        if (pos != std::string::npos)
        {
            std::string fpsValueStr = fault.substr(pos + 1);
            int fpsvalue = std::stoi(fpsValueStr);
            if(fpsvalue<MIN_FPS_THRESHOLD)
            {
                fpsvalue = MIN_FPS_THRESHOLD;
                command = "SET_FPS:" + std::to_string(fpsvalue);
                //Send new FPS value to DMS manager
                commandsQueue.push(command);
            }
            else if(fpsvalue>MAX_FPS_THRESHOLD)
            {
                fpsvalue = MAX_FPS_THRESHOLD;
                command = "SET_FPS:" + std::to_string(fpsvalue);
                //Send new FPS value to DMS manager
                commandsQueue.push(command);
            }
            else //No fault in the fps
            {
                command = "SET_FPS:" + fault.substr(8);
                //Send same FPS to DMS manager
                commandsQueue.push(command);
            }
        }
    }

    //FDT more than max threshold or less than min threshold
    else if (fault.find("SET_FDT:") != std::string::npos)
    {
        std::cout << "Incorrect FDT sent" << std::endl;
        size_t pos = fault.find(":");
        if (pos != std::string::npos)
        {
            std::string fdtValueStr = fault.substr(pos + 1);
            int fdtvalue = std::stoi(fdtValueStr);
            if(fdtvalue<MIN_FDT_THRESHOLD)
            {
                fdtvalue = MIN_FDT_THRESHOLD;
                command = "SET_FDT:" + std::to_string(fdtvalue);
                //Send new FDT value to DMS manager
                commandsQueue.push(command);
            }
            else if(fdtvalue>MAX_FDT_THRESHOLD)
            {
                fdtvalue = MAX_FDT_THRESHOLD;
                command = "SET_FDT:" + std::to_string(fdtvalue);
                //Send new FDT value to DMS manager
                commandsQueue.push(command);
            }
            else //No fault in the fdt
            {
                command = "SET_FDT:" + fault.substr(8);
                //Send same FDT to DMS Manager
                commandsQueue.push(command);
            }
        }
    }

    /* System turn off request at high vehicle velocity */
    else if(fault == "TURN_OFF")
    {
        command = "TURN_OFF";
        commandsQueue.push(command);
    }

    /*
     * No connection to the other device established
     * So DMS can close other components till connection is restored
     */
    else if(fault == "TCP_Connection_Error")
    {
        command = "No_TCP_Connection";
        commandsQueue.push(command);
    }

    /************************** Face detection Faults ********************************/

    //if weights file not found
    else if (fault == "FaceDet_fault")
    {
        std::cout << "Weights file not found" << std::endl;
        command = "SET_FD_MODEL:No Face Detection";
        commandsQueue.push(command);
    }

    /**************************** Vehicle state manager fault *************************/

    else if (fault.find("Velocity_fault") != std::string::npos)
    {
        std::cout << "Velocity above max threshold, setting velocity to max" << std::endl;
    }
    else if (fault.find("Steering_fault") != std::string::npos)
    {
        std::cout << "Steering above max threshold, setting steering to max" << std::endl;
    }

    // Handle unknown fault
    else
    {
        std::cerr << "Unknown fault: " << fault << std::endl;
    }
}

// Function to log faults to the file
void FaultManager::logFault(const std::string& fault) {
    if (faultLogFile.is_open()) {
        pt::ptime now = pt::second_clock::local_time();
        faultLogFile << "[" << pt::to_iso_extended_string(now) << "] " << fault << std::endl;
    } else {
        std::cerr << "Fault log file is not open" << std::endl;
    }
}
