#ifndef MOVE_HPP
#define MOVE_HPP

#include <string>

class Move
{
public:
    std::string sender;
    std::string receiver;
    std::string data;
    int id;
    std::string signature;

    Move(std::string sender, std::string receiver, std::string data);

    void signTransaction(const std::string &privateKey);

    bool isValid() const;

    std::string toString() const;
};

#endif