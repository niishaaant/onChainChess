#ifndef MAINBLOCK_HPP
#define MAINBLOCK_HPP

#include <string>
#include <vector>
#include "Game.hpp"

class MainBlock
{
public:
    int index;
    std::string previousHash;
    std::vector<Game> games;
    long timestamp;
    int nonce;
    std::string hash;
    int difficulty;

    MainBlock(int idx, std::string prevHash, std::vector<Game> games);

    std::string calculateHash();
    void mineBlock(int difficulty);
};

#endif
