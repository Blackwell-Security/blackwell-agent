#pragma once

#include <agent_info.hpp>
#include <centralized_configuration.hpp>
#include <command_handler.hpp>
#include <communicator.hpp>
#include <configuration_parser.hpp>
#include <isignal_handler.hpp>
#include <moduleManager.hpp>
#include <multitype_queue.hpp>
#include <signal_handler.hpp>
#include <task_manager.hpp>

#include <sysInfo.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

/// @brief Agent class
///
/// This class handles the configuration, communication with the manager,
/// command handling, task management, and module management.
class Agent
{
public:
    /// @brief Constructor
    /// @param configFilePath Path to the configuration file
    /// @param signalHandler Pointer to a custom ISignalHandler implementation
    /// @throws std::runtime_error If the Agent is not registered
    /// @throws Any exception propagated from dependencies used within the constructor
    Agent(const std::string& configFilePath,
          std::unique_ptr<ISignalHandler> signalHandler = std::make_unique<SignalHandler>());

    /// @brief Destructor
    ~Agent();

    /// @brief Runs the agent
    ///
    /// This method sets up the agent and starts the task manager.
    void Run();

    /// @brief Reload the modules
    ///
    /// This method stops all modules launched by moduleManager, and starts them again.
    void ReloadModules();

private:
    /// @brief Task manager
    TaskManager m_taskManager;

    /// @brief System info
    SysInfo m_sysInfo;

    /// @brief Configuration parser
    std::shared_ptr<configuration::ConfigurationParser> m_configurationParser;

    std::string m_dataPath;

    /// @brief Queue for storing messages
    std::shared_ptr<MultiTypeQueue> m_messageQueue;

    /// @brief Pointer to a custom ISignalHandler implementation
    std::unique_ptr<ISignalHandler> m_signalHandler;

    /// @brief Agent info
    AgentInfo m_agentInfo;

    /// @brief Communicator
    communicator::Communicator m_communicator;

    /// @brief Module manager
    ModuleManager m_moduleManager;

    /// @brief Command handler
    command_handler::CommandHandler m_commandHandler;

    /// @brief Centralized configuration
    centralized_configuration::CentralizedConfiguration m_centralizedConfiguration;

    /// @brief Mutex to coordinate agent reload
    std::mutex m_reloadMutex;

    /// @brief Indicates if the agent is running
    std::atomic<bool> m_running = true;

    /// @brief Agent thread count
    size_t m_agentThreadCount;
};
