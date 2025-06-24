#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include "BlockGame.hpp"
#include "Move.hpp"

using namespace std;

class Game
{
private:
    vector<BlockGame> chain;

    BlockGame createGenesisBlock();

public:
    Game(vector<string> players);
    Game();
    int gameId;
    vector<string> players;
    string winnerId;

    bool gameComplete = false;

    void addBlock(BlockGame newBlock);
    BlockGame getLastBlock();
    vector<BlockGame> getChain() const;
    string toString() const;
    void endGame();

    ~Game() = default;
};

#endif
