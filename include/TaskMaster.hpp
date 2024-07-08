#ifndef TASKMASTER_H
# define TASKMASTER_H

#include <string>
#include <iostream>
#include <vector>
#include "Command.hpp"
#include "ConfigParser.hpp"
#include "Process.hpp"
#include "Utils.hpp"
#include "colors.hpp"
#include <set>

class Process;

class TaskMaster {
	public:
		explicit TaskMaster(const std::string& configFilePath);
		~TaskMaster();
		static void stopAllProcesses();
		static void reloadConfig();
		static void updateExistingProcesses(const json& newConfig);
		static void addNewProcesses(const json& newConfig);
		static void removeOldProcesses(const json& newConfig);
		static void updateInstances(Process& process, int newInstances);

	private:
		std::string configFilePath;
		ConfigParser configParser;
		bool loggingEnabled;
		std::string logFilePath;
		static std::map<std::string, Process> processes;

		const std::string statusCmd = "status";
    	const std::string startCmd = "start";
    	const std::string stopCmd = "stop";
    	const std::string restartCmd = "restart";
		const std::string reloadCmd = "reload";

		static void initializeLogger(const json& config);
		static void initializeProcesses(const json& config);
		static void startInitialProcesses();
		void commandLoop();
		void handleCommand(const std::string& command);
		static void startProcess(const std::string& processName);
		static void stopProcess(const std::string& processName);
		static void restartProcess(const std::string& processName);
		static void sendSighupSignalToReload();
		static void displayStatus();
		static void displayUsage();
		static Process* findProcess(const std::string& processName);
};

#endif
