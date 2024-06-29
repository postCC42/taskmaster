#ifndef TASKMASTER_H
# define TASKMASTER_H

#include <string>
#include <iostream>
#include <vector>
#include "ProcessControl.hpp"
#include "ConfigParser.hpp"

class TaskMaster {
	public:
		explicit TaskMaster(const std::string& configFilePath);
		void initializeProcesses();

	private:
		std::string configFilePath;
		std::vector<ProcessControl> processes;
		ConfigParser configParser;

		void commandLoop();
		void startInitialProcesses();
		void handleCommand(const std::string& command);
		void displayStatus();
		void displayUsage();
		void startProcess(const std::string& processName);
};
#endif
