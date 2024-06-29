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
		// TODO: add destructor to stop all
		void initializeProcesses();

	private:
		std::string configFilePath;
		ConfigParser configParser;
		std::map<std::string, ProcessControl> processes;

		void commandLoop();
		void startInitialProcesses();
		void handleCommand(const std::string& command);
		void displayStatus();
		void displayUsage();
		void startProcess(const std::string& processName);
};
#endif
