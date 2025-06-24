#ifndef MAINCHAIN_HPP
#define MAINCHAIN_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include "MainBlock.hpp"
#include "Game.hpp"

using namespace std;

class MainChain
{
private:
    vector<MainBlock> chain;              // Ordered chain
    unordered_map<string, double> rating; // Hash map to maintain balances

    MainBlock createGenesisBlock();
    void updateRating(const MainBlock &block);

public:
    MainChain();

    void addBlock(MainBlock newGame);
    MainBlock getLastBlock();
    vector<MainBlock> getChain();
    double getRating(const string &address);
};

#endif
