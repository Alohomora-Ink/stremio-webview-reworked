#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <string>
#include <functional>
#include <map>
#include <optional> 
#include "../types.h"

namespace WebViewProtocol
{
    class CommandHandler
    {
    public:
        using CommandFunction = std::function<void(const json& payload, const std::optional<std::string>& messageId)>;

        CommandHandler() = default;
        void RegisterCommand(const std::string& commandName, CommandFunction handler);
        void HandleCommand(const std::wstring& message);

    private:
        std::map<std::string, CommandFunction> m_commands;
    };
}

#endif // COMMAND_HANDLER_H