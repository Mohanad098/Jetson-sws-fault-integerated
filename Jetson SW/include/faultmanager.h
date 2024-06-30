// FaultManager.h

#pragma once

#include <queue>
#include <thread>
#include <iostream>
#include "threadsafequeue.h"
#include <atomic>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>


/* Declerations for FPS and FDT */
#define MAX_FPS_THRESHOLD       100
#define MIN_FPS_THRESHOLD       0
#define MAX_FDT_THRESHOLD       100
#define MIN_FDT_THRESHOLD       0

/* Declerations for max velocity and steering */
#define MAX_VELOCITY_THRESHOLD  220
#define MAX_STEERING_THRESHOLD  540

class FaultManager
{
public:
    FaultManager(ThreadSafeQueue<std::string>& commandsQueue,ThreadSafeQueue<std::string>& faultsQueue);
    ~FaultManager();

    void faultstart();
    void faultstop();
    void faulthandling(const std::string& fault);
    void logFault(const std::string& fault);

private:
    ThreadSafeQueue<std::string>& commandsQueue;
    ThreadSafeQueue<std::string>& faultsQueue;
    std::thread faultsthread;
    bool running;
    void faultfind();
    // For logging
    std::ofstream faultLogFile;
    boost::filesystem::path logDir;
    std::string generateLogFilename();
};


