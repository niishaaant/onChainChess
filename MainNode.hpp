#ifndef MAINNODE_HPP
#define MAINNODE_HPP

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "MainChain.hpp"
#include "MainBlock.hpp"
#include "Game.hpp"

class MainNode
{
private:
    MainChain &blockchain;
    int difficulty;
    std::queue<Game> transactionQueue;
    std::mutex mtx;
    std::mutex mtxPeers;
    std::condition_variable cv;
    std::vector<MainNode *> peers;

    void logMessage(const std::string &message);
    bool isValidTransaction(const Game &txn);
    void broadcastTransaction(const Game &txn);
    void broadcastTransaction(const Game &txn, MainNode *peer);
    void broadcastBlock(const MainBlock &block, int peerId);
    void receiveTransaction(const Game &txn, MainNode *peer);
    void addTransaction(const Game &txn, MainNode *peer);
    void receiveBlock(const MainBlock &block, MainNode *peer);
    bool verifyNewBlock(const MainBlock &block);
    bool verifyValidGame(const Game &game);
    void updateBlockchainFile(const MainBlock &block);
    void updateMempoolFile(const vector<Game> &transactions);
    void syncPeers();

public:
    MainNode(MainChain &bc, int diff = 7);
    MainNode(vector<MainNode *> peers, int diff = 7);
    bool running = true;
    int nodeId;

    void addTransaction(const Game &txn);
    void mineBlock();
    void connectPeer(MainNode *peer);
    void stop();
    ~MainNode();
};

#endif
