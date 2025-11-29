#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <optional>

namespace compiler
{
    class Parser
    {
    public:
        Parser() = default;

        void AddOption(std::string_view flag,
            std::string_view description,
            bool required = false,
            int minArgs = 0)
        {
            Option option;
            option.name = flag;
            option.description = description;
            option.isRequired = required;
            option.minArgs = minArgs;
            options[std::string(flag)] = std::move(option);
        }

        bool Parse(int argc, const char* const argv[]) noexcept
        {
            if (argc < 1)
            {
                std::cout << "argc < 1";
                return false;
            }

            programName = argv[0];

            for (int i = 1; i < argc; ++i)
            {
                std::string arg = argv[i];

                if (arg.length() > 1 && arg[0] == '-')
                {
                    //remove leading dashes
                    std::string flag = arg;
                    if (flag.length() > 2 && flag.substr(0, 2) == "--")
                        flag = flag.substr(2);
                    else if (flag[0] == '-')
                        flag = flag.substr(1);

                    auto it = options.find(flag);
                    if (it == options.end())
                    {
                        continue;
                    }

                    Option& option = it->second;
                    option.found = true;

                    //Collect arguments for this option
                    int argCount = 0;
                    while (i + 1 < argc && argv[i + 1][0] != '-')
                    {
                        ++i;

                        option.args.push_back(argv[i]);
                        ++argCount;
                    }

                    // Check mimium arguments requirement
                    if (static_cast<int>(option.args.size()) < option.minArgs)
                    {
                        std::cout << "Not enough arguments! Required: " << option.minArgs << ", provided: " << option.args.size() << "\n";
                        return false;
                    }
                }
            }

            // See if required options all provided
            for (const auto& [flagName, option] : options)
            {
                if (option.isRequired && !option.found)
                {
                    std::cout << "Option [" << flagName << "] is required, but not provided!\n";
                    return false;
                }
            }

            return true;
        }

        void Clear()
        {
            options.clear();
            programName.clear();
        }

        void ResetProgram()
        {
            for (auto& [key, val] : options)
            {
                val.args.clear();
                val.found = false;
            }
            programName.clear();
        }

        void PrintHelp() const noexcept
        {
            std::cout << "Usage: " << programName << " [options]\n\n";
            std::cout << "Options:\n";

            for (const auto& [flag, option] : options)
            {
                std::cout << "  -" << flag << ", --" << flag;
                if (option.minArgs > 0)
                {
                    std::cout << " <arg";
                    if (option.minArgs > 1)
                        std::cout << "1>..<arg" << option.minArgs << ">";
                    else
                        std::cout << ">";
                }
                std::cout << "\n";
                std::cout << "      " << option.description;
                if (option.isRequired)
                    std::cout << " (required)";
                std::cout << "\n\n";
            }
        }

        bool HasOption(std::string_view key) const
        {
            auto it = options.find(std::string(key));
            return it != options.end() && it->second.found;
        }

        // Get first argument as specific type (convenience function)
        template<typename T>
        std::optional<T> GetOption(std::string_view flag) const
        {
            auto args = GetOptionArgs(flag);
            if (args.empty())
                return std::nullopt;

            if constexpr (std::is_same_v<T, std::string>)
            {
                return args[0];
            }
            else
            {
                return ConvertValue<T>(args[0], std::string(flag), 0);
            }
        }

        // Get option arguments as strings
        std::vector<std::string> GetOptionArgs(std::string_view flag) const
        {
            auto it = options.find(std::string(flag));
            if (it == options.end() || !it->second.found)
                return {};
            return it->second.args;
        }

        // Get all arguments as specific type
        template<typename T>
        std::vector<T> GetOptions(std::string_view flag) const
        {
            auto args = GetOptionArgs(flag);
            std::vector<T> result;

            for (size_t i = 0; i < args.size(); ++i)
            {
                if constexpr (std::is_same_v<T, std::string>)
                {
                    result.push_back(args[i]);
                }
                else
                {
                    auto converted = ConvertValue<T>(args[i], std::string(flag), i);
                    if (converted.has_value())
                        result.push_back(converted.value());
                }
            }

            return result;
        }


    protected:
        struct Option
        {
            std::string                         name;
            std::string                         description;
            bool                                isRequired = false;
            int                                 minArgs = 0;
            std::vector<std::string>            args;
            bool                                found = false;  // Track if option was found during parsing
        };

        std::unordered_map<std::string, Option>         options;         // All options and their arguments
        std::string                                     programName;     // Name of the program


        template<typename T>
        std::optional<T> ConvertValue([[maybe_unused]] const std::string& value, [[maybe_unused]] std::string flag, [[maybe_unused]] size_t argIndex) const
        {
            static_assert(sizeof(T) == 0, "Type conversion not implemented for this type");
            return std::nullopt;
        }

        // Type to get back must be specialized
        template<>
        inline std::optional<std::string> ConvertValue<std::string>(const std::string& value, [[maybe_unused]] std::string flag, [[maybe_unused]] size_t argIndex) const
        {
            return value;
        }

        template<>
        inline std::optional<int64_t> ConvertValue<int64_t>(const std::string& value, [[maybe_unused]] std::string flag, [[maybe_unused]] size_t argIndex) const
        {
            try
            {
                size_t pos;
                int64_t result = std::stoll(value, &pos);
                if (pos != value.length())
                    return std::nullopt; // Not fully converted
                return result;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        template<>
        inline std::optional<int> ConvertValue<int>(const std::string& value, [[maybe_unused]] std::string flag, [[maybe_unused]] size_t argIndex) const
        {
            try
            {
                size_t pos;
                int result = std::stoi(value, &pos);
                if (pos != value.length())
                    return std::nullopt; // Not fully converted
                return result;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        template<>
        inline std::optional<double> ConvertValue<double>(const std::string& value, [[maybe_unused]] std::string flag, [[maybe_unused]] size_t argIndex) const
        {
            try
            {
                size_t pos;
                double result = std::stod(value, &pos);
                if (pos != value.length())
                    return std::nullopt; // Not fully converted
                return result;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
    };

} // namespace compiler