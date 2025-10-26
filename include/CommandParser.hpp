#pragma once
#include <string>
#include <vector>

class CommandParser {
public:
    explicit CommandParser(const std::string& input);
    std::string getCommand() const;
    std::string getArg(size_t index) const;
private:
    std::vector<std::string> tokens_;
};