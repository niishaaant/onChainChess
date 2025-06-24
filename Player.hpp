#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "Game.hpp"
#include "Move.hpp"
#include "MainNode.hpp"

class Player
{
private:
    Game &blockchain;
    int difficulty;
    queue<Move> transactionQueue;
    mutex mtx;
    mutex mtxPeers;
    condition_variable cv;
    vector<Player *> peers;
    vector<MainNode *> mainNodes;
    queue<Game> completeGames;
    void logMessage(const string &message);
    bool isValidMove(const Move &txn);
    void broadcastTransaction(const Move &txn);
    void broadcastTransaction(const Move &txn, Player *peer);
    void broadcastBlock(const BlockGame &block, string peerId);
    void receiveTransaction(const Move &txn, Player *peer);
    void addTransaction(const Move &txn, Player *peer);
    void receiveBlock(const BlockGame &block, Player *peer);
    bool verifyNewBlock(const BlockGame &block);
    void syncPeers();
    bool verifyValidGame(const Game &game);
    void generateKeyPair();

public:
    Player(int diff = 4);
    bool running = true;
    Player *opponent;
    string nodeId;
    string publicKey;
    string privateKey;

    void addMove(const Move &txn);
    void addCompleteGame(const Game &game);
    void sendCompleteGame();
    void mineBlock();
    bool gameStrated(Player &opponent, Game &newChain);
    void connectPeer(Player &peer);
    void connectNode(MainNode &peer);
    void createMove(string data);
    void stop();
    ~Player();
};

#endif
