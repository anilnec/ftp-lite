#include "CommandParser.hpp"
#include <sstream>

CommandParser::CommandParser(const std::string& input) {
    std::istringstream iss(input);
    std::string token;
    while (iss >> token)
        tokens_.push_back(token);
}

std::string CommandParser::getCommand() const {
    return tokens_.empty() ? "" : tokens_[0];
}

std::string CommandParser::getArg(size_t index) const {
    if (index + 1 < tokens_.size()) return tokens_[index + 1];
    return "";
}
