#pragma once

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

#include <utils/logger/Logger.h>

namespace nvd {

/// @brief Handles argument parsing, validation, and error reporting seamlessly.
class ProgramOptions
{
public:
    /// @brief construct ProgramOptions object
    ProgramOptions(std::string description);

    /// @brief print usage/help message
    void print_help() const;

    /// @brief parse command-line arguments
    bool parse(int argc, char* argv[]);
    
    /// @brief add optional arguments with a default value
    template <typename T>
    void add_arg(const std::string& name, const std::string& short_opt, const std::string& description, const T& default_value);

    /// @brief add optional or required arguments without a default value
    template <typename T>
    void add_arg(const std::string& name, const std::string& short_opt, const std::string& description);

    /// @brief lookup argument value given its name
    ///        If an argument has no default value, it will throw runtime error
    /// @return value of the given argument name, nullopt otherwise    
    template <typename T>
    std::optional<T> get_value(const std::string& name) const;

private:

    boost::program_options::options_description _desc;
    boost::program_options::variables_map _vmap;
};

// ----------------------------------------------------------------
// template methods implementation

template <typename T>
void ProgramOptions::add_arg(const std::string& name, const std::string& short_opt,const std::string& description, const T& default_value)
{
    _desc.add_options()(name.c_str(), boost::program_options::value<T>()->default_value(default_value), description.c_str());
}

// Add argument without a default value (required or optional)
template <typename T>
void ProgramOptions::add_arg(const std::string& name, const std::string& short_opt, const std::string& description) 
{
    _desc.add_options()(name.c_str(), boost::program_options::value<T>(), description.c_str());
}

// Get the value of an argument
template <typename T>
std::optional<T> ProgramOptions::get_value(const std::string& name) const 
{
    if (_vmap.count(name)) 
    {
        return _vmap[name].as<T>();
    } 
    else 
    {
        //throw std::runtime_error();
        LOGERROR("Argument '{}' not found.", name);
        return std::nullopt;
    }
}

} // namespace
