#include <logger.hpp>
#include <restart_handler_unix.hpp>

#include <chrono>
#include <thread>

namespace restart_handler
{

    std::vector<char*> RestartHandler::startupCmdLineArgs;

    bool UsingSystemctl()
    {
        return (0 == std::system("which systemctl > /dev/null 2>&1") && nullptr != std::getenv("INVOCATION_ID"));
    }

    boost::asio::awaitable<module_command::CommandExecutionResult> RestartWithSystemd()
    {
        LogInfo("Systemctl restarting wazuh agent service.");
        if (std::system("systemctl restart wazuh-agent") != 0)
        {
            co_return module_command::CommandExecutionResult {module_command::Status::IN_PROGRESS,
                                                              "Systemctl restart execution"};
        }
        else
        {
            LogError("Failed using systemctl.");
            co_return module_command::CommandExecutionResult {module_command::Status::FAILURE,
                                                              "Systemctl restart failed"};
        }
    }

    void StopAgent(const pid_t pid, const int timeoutInSecs)
    {
        const time_t startTime = time(nullptr);

        // Shutdown Gracefully
        kill(pid, SIGTERM);

        while (true)
        {
            if (kill(pid, 0) != 0)
            {
                LogInfo("Agent stopped.");
                break;
            }

            if (difftime(time(nullptr), startTime) > timeoutInSecs)
            {
                LogError("Timeout reached! Forcing agent process termination.");
                kill(pid, SIGKILL);
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    boost::asio::awaitable<module_command::CommandExecutionResult> RestartWithFork()
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            LogError("Fork failed.");
        }
        else if (pid == 0)
        {
            // Child process
            StopAgent(getppid(), RestartHandler::timeoutInSecs);

            LogInfo("Starting wazuh agent in a new process.");

            if (execve(RestartHandler::startupCmdLineArgs[0], RestartHandler::startupCmdLineArgs.data(), nullptr) == -1)
            {
                LogError("Failed to spawn new Wazuh agent process.");
            }
        }

        co_return module_command::CommandExecutionResult {module_command::Status::IN_PROGRESS,
                                                          "Pending restart execution"};
    }

    boost::asio::awaitable<module_command::CommandExecutionResult> RestartHandler::RestartCommand()
    {

        if (UsingSystemctl())
        {
            return RestartWithSystemd();
        }
        else
        {
            return RestartWithFork();
        }
    }

} // namespace restart_handler
