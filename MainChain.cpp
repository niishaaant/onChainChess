#include "MainChain.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <fstream>

using namespace std;

MainChain::MainChain()
{
    // Add the genesis block
    chain.push_back(createGenesisBlock());
}

MainBlock MainChain::createGenesisBlock()
{
    return MainBlock(0, "0", vector<Game>());
}

void MainChain::updateRating(const MainBlock &block)
{
    for (const auto &game : block.games)
    {
        if (game.gameComplete)
        {
            if (game.players[0] == game.winnerId)
            {
                rating[game.players[0]] += 1.0;
                rating[game.players[1]] -= 1.0;
            }
            else
            {
                rating[game.players[0]] -= 1.0;
                rating[game.players[1]] += 1.0;
            }
        }
    }
}

void MainChain::addBlock(MainBlock newBlock)
{
    updateRating(newBlock); // Update balances before adding the block
    chain.push_back(newBlock);
}

MainBlock MainChain::getLastBlock()
{
    if (chain.empty())
    {
        throw runtime_error("Blockchain is empty");
    }
    return chain.back();
}

vector<MainBlock> MainChain::getChain()
{
    return chain;
}

double MainChain::getRating(const string &address)
{
    if (rating.find(address) == rating.end())
    {
        return 0.0; // Default balance is 0
    }
    return rating[address];
}
