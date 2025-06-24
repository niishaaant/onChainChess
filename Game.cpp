#include <iostream>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include "Game.hpp"

using namespace std;

Game::Game(vector<string> players)
{
    if (players.size() < 2)
    {
        throw runtime_error("At least two players are required to start a game.");
    }

    this->gameId = rand() % 1000000; // Random game ID
    this->gameComplete = false;
    chain.push_back(createGenesisBlock());
    this->players = players;
    this->winnerId = "";
}
Game::Game()
{
    chain.push_back(createGenesisBlock());
    this->winnerId = "";
}

BlockGame Game::createGenesisBlock()
{
    vector<Move> genesisMoves = {};
    return BlockGame(0, "0", genesisMoves);
}

void Game::addBlock(BlockGame newBlock)
{
    chain.push_back(newBlock);
}

BlockGame Game::getLastBlock()
{
    if (chain.empty())
    {
        throw runtime_error("Chain is empty");
    }
    return chain.back();
}

void Game::endGame()
{
    if (this->winnerId != "")
    {
        cerr << "Game already ended";
    }
    if (chain.size() > 0 && chain.size() == 3)
    {
        this->winnerId = chain.back().moves[0].receiver;
        gameComplete = true;
        cout << "Game ended. Winner is player with ID: " << winnerId << endl;
    }
    else
    {
        cerr << "No blocks in the chain to determine a winner." << endl;
    }
}

string Game::toString() const
{
    std::ostringstream ss;
    ss << "Game ID: " << gameId << "\n";
    ss << "Players: ";
    for (const auto &player : players)
    {
        ss << player << " ";
    }
    ss << "\n";
    ss << "Winner ID: " << (winnerId.empty() ? "None" : winnerId) << "\n";
    ss << "Game Complete: " << (gameComplete ? "Yes" : "No") << "\n";
    ss << "Chain Size: " << chain.size() << "\n";
    ss << "Moves:\n";
    for (const auto &block : chain)
    {
        for (const auto &move : block.moves)
        {
            ss << "  Sender: " << move.sender << ", Receiver: " << move.receiver << ", Move: " << move.data << "\n";
        }
    }
    return ss.str();
}

vector<BlockGame> Game::getChain() const
{
    return chain;
}
