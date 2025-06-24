#ifndef BLOCKGAME_HPP
#define BLOCKGAME_HPP

#include <string>
#include <vector>
#include "Move.hpp"

class BlockGame
{
public:
    int index;
    std::string previousHash;
    std::vector<Move> moves;
    long timestamp;
    int nonce;
    std::string hash;
    int difficulty;

    BlockGame(int idx, std::string prevHash, std::vector<Move> moves);

    std::string calculateHash();
    void mineBlock(int difficulty);
};

#endif
