#include "command_handler.h"
#include "../../helpers/helpers.h"
#include "../../logger/logger.h"

namespace WebViewProtocol
{
    void CommandHandler::RegisterCommand(const std::string& commandName, CommandFunction handler)
    {
        m_commands[commandName] = handler;
    }

    void CommandHandler::HandleCommand(const std::wstring& message)
    {
        try
        {
            auto commandJson = json::parse(WStringToUtf8(message));
            std::string commandName = commandJson.at("command").get<std::string>();
            const auto& payload = commandJson.at("payload");

            std::optional<std::string> messageId;
            if (commandJson.contains("messageId")) {
                messageId = commandJson.at("messageId").get<std::string>();
            }

            if (m_commands.count(commandName))
            {
                m_commands[commandName](payload, messageId);
            }
            else
            {
                LOG_WARN("CommandHandler", "Unknown command received: " + commandName);
            }
        }
        catch (const json::exception& e)
        {
            LOG_ERROR("CommandHandler", "JSON parsing error: " + std::string(e.what()));
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("CommandHandler", "Error handling command: " + std::string(e.what()));
        }
    }
}