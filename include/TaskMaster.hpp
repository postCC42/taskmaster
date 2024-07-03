#ifndef TASKMASTER_H
# define TASKMASTER_H

#include <string>
#include <iostream>
#include <vector>
#include "Command.hpp"
#include "ConfigParser.hpp"
#include "Process.hpp"
#include "Utils.hpp"

class TaskMaster {
	public:
		explicit TaskMaster(const std::string& configFilePath);
		// TODO: add destructor to stop all
		void initializeProcesses();

	private:
		std::string configFilePath;
		ConfigParser configParser;
		std::map<std::string, Process> processes;

		const std::string statusCmd = "status";
    	const std::string startCmd = "start";
    	const std::string stopCmd = "stop";

		void commandLoop();
		void startInitialProcesses();
		void handleCommand(const std::string& command);
		void displayStatus();
		void displayUsage();
		void startProcess(const std::string& processName);
		void stopProcess(const std::string& processName);
		Process* findProcess(const std::string& processName);
		void stopAllProcesses();
};

#endif
