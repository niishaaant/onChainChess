#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <random>

// #include "Wallet.cpp"
// #include "Node.hpp"
// #include "Blockchain.hpp"

#include "Player.hpp"
#include "Game.hpp"
#include "MainNode.hpp"
#include "MainChain.hpp"

using namespace std;

void initializeFiles()
{
    cout << "creating files" << endl;
    std::ofstream transactionFile("./createdTransaction.json");
    transactionFile << "[]";
    transactionFile.close();

    std::ofstream walletFile("./createdWallet.json");
    walletFile << "[]";
    walletFile.close();

    std::ofstream logFile("./logs.json");
    logFile << "[]";
    logFile.close();

    // Delete the ./data folder if it exists
    system("rm -rf ./data");

    // Create a new ./data folder
    system("mkdir ./data");
}

string generateRandomMove()
{
    const vector<string> files = {"a", "b", "c", "d", "e", "f", "g", "h"};
    const vector<string> ranks = {"1", "2", "3", "4", "5", "6", "7", "8"};
    const vector<string> pieces = {"", "N", "B", "R", "Q", "K"}; // Optional piece notation

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> fileDist(0, files.size() - 1);
    uniform_int_distribution<> rankDist(0, ranks.size() - 1);
    uniform_int_distribution<> pieceDist(0, pieces.size() - 1);

    string move = pieces[pieceDist(gen)] + files[fileDist(gen)] + ranks[rankDist(gen)];
    return move;
}

bool createNewGame(Player &p1, Player &p2)
{
    if (p1.opponent == nullptr && p2.opponent == nullptr)
    {
        Game *newGame = new Game(vector<string>{p1.nodeId, p2.nodeId});
        bool success1 = p1.gameStrated(ref(p2), *newGame);
        bool success2 = p2.gameStrated(ref(p1), *newGame);

        if (!(success1 || success2))
        {
            throw runtime_error("Failed to create a new game. One of the players is already in a game.");
            return false;
        }

        cout << "New game created with ID: " << newGame->toString() << endl;
        return true;
    }

    return false;
}

int main()
{

    initializeFiles();
    try
    {

        // // Initialize blockchain and nodes
        MainChain *blk = new MainChain();
        MainNode *node1 = new MainNode(*blk, 5);
        MainNode *node2 = new MainNode({node1}, 5);
        MainNode *node3 = new MainNode({node1, node2}, 5);

        // Ensure all nodes are interconnected
        node1->connectPeer(node2);
        node1->connectPeer(node3);
        node2->connectPeer(node3);

        // // Initialize blockchain and nodes
        // MainChain *blk = new MainChain();
        // MainNode *node1 = new MainNode(*blk, 5);
        // MainNode *node2 = new MainNode({node1}, 5);

        // // Ensure all nodes are interconnected
        // node1->connectPeer(node2);
        // node2->connectPeer(node1);

        // Start mining threads for all nodes
        thread mainMiningThread1(&MainNode::mineBlock, node1);
        thread mainMiningThread2(&MainNode::mineBlock, node2);
        thread mainMiningThread3(&MainNode::mineBlock, node3);

        // Initialize players
        Player p1(4), p2(4), p3(4), p4(4), p5(4), p6(4), p7(4), p8(4);
        // p1.connectNode(*node1);
        // p2.connectNode(*node1);
        // p3.connectNode(*node2);
        // p4.connectNode(*node2);
        // p5.connectNode(*node3);
        // p6.connectNode(*node3);
        // p7.connectNode(*node3);
        // p8.connectNode(*node3);

        cout << "Players created successfully!" << endl;

        // Create games
        createNewGame(p1, p2);
        createNewGame(p3, p4);
        createNewGame(p5, p6);
        createNewGame(p7, p8);

        cout << "Players connected successfully, games started" << endl;

        // Add transactions (moves)
        bool turn = false;
        for (int i = 0; i < 10; i++)
        {
            string move = generateRandomMove();
            if (turn)
            {
                p1.createMove(move);
                p3.createMove(move);
                p5.createMove(move);
                p7.createMove(move);
            }
            else
            {
                p2.createMove(move);
                p4.createMove(move);
                p6.createMove(move);
                p8.createMove(move);
            }
            turn = !turn;
            this_thread::sleep_for(chrono::milliseconds(100)); // Prevent overwhelming nodes
        }

        cout << "Transactions added successfully!" << endl;

        // Start player mining threads
        thread playerMiningThread1(&Player::mineBlock, &p1);
        thread playerMiningThread2(&Player::mineBlock, &p2);
        thread playerMiningThread3(&Player::mineBlock, &p3);
        thread playerMiningThread4(&Player::mineBlock, &p4);
        thread playerMiningThread5(&Player::mineBlock, &p5);
        thread playerMiningThread6(&Player::mineBlock, &p6);
        thread playerMiningThread7(&Player::mineBlock, &p7);
        thread playerMiningThread8(&Player::mineBlock, &p8);

        // Allow time for mining and synchronization
        this_thread::sleep_for(chrono::seconds(30));

        // Stop nodes
        node1->stop();
        node2->stop();
        // node3->stop();

        // Join player mining threads
        playerMiningThread1.join();
        playerMiningThread2.join();
        playerMiningThread3.join();
        playerMiningThread4.join();
        playerMiningThread5.join();
        playerMiningThread6.join();
        playerMiningThread7.join();
        playerMiningThread8.join();

        // Join node mining threads
        mainMiningThread1.join();
        mainMiningThread2.join();
        mainMiningThread3.join();
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}