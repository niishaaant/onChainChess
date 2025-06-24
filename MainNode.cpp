#include "MainNode.hpp"
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

MainNode::MainNode(MainChain &bc, int diff) : blockchain(bc), difficulty(diff)
{
    nodeId = 1000000000 + rand() % 9000000000;
    logMessage("Node " + to_string(nodeId) + " started");

    // Initialize {nodeId}_mempool.json with an empty array
    string mempoolFilename = "./data/" + to_string(nodeId) + "_mainMempool.json";
    ifstream mempoolCheckFile(mempoolFilename);
    ofstream mempoolInitFile(mempoolFilename);
    if (mempoolInitFile.is_open())
    {
        mempoolInitFile << "[]"; // Initialize with an empty array
        mempoolInitFile.close();
    }
    // Initialize {nodeId}_blockchain.json with an empty array
    string blockchainFilename = "./data/" + to_string(nodeId) + "_mainBlockchain.json";
    ifstream blockchainCheckFile(blockchainFilename);
    ofstream blockchainInitFile(blockchainFilename);
    if (blockchainInitFile.is_open())
    {
        blockchainInitFile << "[]"; // Initialize with an empty array
        blockchainInitFile.close();
    }

    // Add the genesis block to {nodeId}_blockchain.json
    string filename = "./data/" + to_string(nodeId) + "_mainBlockchain.json";
    ifstream checkFile(filename);
    if (checkFile.peek() == ifstream::traits_type::eof())
    {
        ofstream initFile(filename);
        if (initFile.is_open())
        {
            initFile << "[]"; // Initialize with an empty array
            initFile.close();
        }
    }

    json blockchainJson = json::array();
    ifstream inputFile(filename);
    if (inputFile.is_open())
    {
        try
        {
            inputFile >> blockchainJson;
        }
        catch (const json::parse_error &e)
        {
            cerr << "Error parsing JSON 1: " << e.what() << endl;
        }
        inputFile.close();
    }

    MainBlock genesisBlock = blockchain.getChain().front();
    json blockJson = {
        {"index", genesisBlock.index},
        {"previousHash", genesisBlock.previousHash},
        {"hash", genesisBlock.hash},
        {"timestamp", genesisBlock.timestamp},
        {"games", json::array()},
        {"nonce", genesisBlock.nonce}};

    for (const auto &txn : genesisBlock.games)
    {
        blockJson["games"].push_back(txn.toString());
    }

    blockchainJson.push_back(blockJson);

    ofstream outputFile(filename, ios::trunc);
    if (!outputFile.is_open())
    {
        throw runtime_error("Failed to open " + filename + " for writing.");
    }

    outputFile << blockchainJson.dump(4); // Pretty print with 4 spaces
    outputFile.close();
}

MainNode::MainNode(vector<MainNode *> peers, int diff) : blockchain(*(new MainChain())), difficulty(diff) // Initialize with an empty blockchain
{                                                                                                         // Initialize with a dummy address
    nodeId = 1000000000 + rand() % 9000000000;
    if (peers.empty())
    {
        throw runtime_error("No peers connected.");
    }

    for (auto &peer : peers)
    {
        if (peer->running) // Check if the peer is running
        {
            connectPeer(peer);
        }
        else
        {
            logMessage("Peer " + to_string(peer->nodeId) + " is not running. Skipping connection.");
        }
    }

    logMessage("Node " + to_string(nodeId) + " started");

    // Initialize {nodeId}_mempool.json with an empty array
    string mempoolFilename = "./data/" + to_string(nodeId) + "_mainMempool.json";
    ifstream mempoolCheckFile(mempoolFilename);
    ofstream mempoolInitFile(mempoolFilename);
    if (mempoolInitFile.is_open())
    {
        mempoolInitFile << "[]"; // Initialize with an empty array
        mempoolInitFile.close();
    }
    // Initialize {nodeId}_blockchain.json with an empty array
    string blockchainFilename = "./data/" + to_string(nodeId) + "_mainBlockchain.json";
    ifstream blockchainCheckFile(blockchainFilename);
    ofstream blockchainInitFile(blockchainFilename);
    if (blockchainInitFile.is_open())
    {
        blockchainInitFile << "[]"; // Initialize with an empty array
        blockchainInitFile.close();
    }

    syncPeers();

    // Save the blockchain to {nodeId}_blockchain.json
    json blockchainJson = json::array();

    for (const auto &block : blockchain.getChain())
    {
        json blockJson = {
            {"index", block.index},
            {"previousHash", block.previousHash},
            {"hash", block.hash},
            {"timestamp", block.timestamp},
            {"games", json::array()},
            {"nonce", block.nonce}};

        for (const auto &txn : block.games)
        {
            blockJson["games"].push_back(txn.toString());
        }

        blockchainJson.push_back(blockJson);
    }

    ofstream blockchainOutFile(blockchainFilename, ios::trunc);
    if (blockchainOutFile.is_open())
    {
        blockchainOutFile << blockchainJson.dump(4); // Pretty print with 4 spaces
        blockchainOutFile.close();
    }

    // Save the transactionQueue to {nodeId}_mempool.json
    json mempoolJson = json::array();
    {
        lock_guard<mutex> lock(mtx);
        queue<Game> tempQueue = transactionQueue;

        while (!tempQueue.empty())
        {
            const auto &txn = tempQueue.front();
            mempoolJson.push_back(txn.toString());
            tempQueue.pop();
        }
    }

    ofstream mempoolOutFile(mempoolFilename, ios::trunc);
    if (mempoolOutFile.is_open())
    {
        mempoolOutFile << mempoolJson.dump(4); // Pretty print with 4 spaces
        mempoolOutFile.close();
    }
}

void MainNode::logMessage(const string &message)
{
    ifstream inputFile("logs.json");
    json logs = json::array();

    // cout << "Logging message: " << message << endl;

    // Read existing logs from the file

    if (inputFile.is_open())
    {
        try
        {
            inputFile >> logs;
        }
        catch (const json::parse_error &e)
        {
            cerr << "Error parsing JSON 2: " << e.what() << endl;
        }
        inputFile.close();
    }

    // Add the new log entry
    json logEntry = {
        {"timestamp", chrono::duration_cast<chrono::seconds>(
                          chrono::system_clock::now().time_since_epoch())
                          .count()},
        {"message", message}};
    logs.push_back(logEntry);

    // Write back to the file
    ofstream outputFile("logs.json", ios::trunc);
    if (!outputFile.is_open())
    {
        throw runtime_error("Failed to open createdTransaction.json for writing.");
    }

    outputFile << logs.dump(4); // Pretty print with 4 spaces

    outputFile.close();
}

bool MainNode::isValidTransaction(const Game &txn)
{
    if (!txn.gameComplete)
    {
        cout << "here1" << endl;
        return false;
    }
    if (txn.getChain().size() == 0)
    {
        cout << "here2" << endl;
        return false;
    }
    if (txn.players.size() != 2)
    {
        cout << txn.toString() << endl;
        cout << "here3" << endl;
        return false;
    }
    if (txn.winnerId == "")
    {
        cout << "here4" << endl;
        return false;
    }
    return true;
}

void MainNode::mineBlock()
{
    while (running)
    {
        try
        {
            vector<Game> transactions;
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this]
                        { return !running || !transactionQueue.empty(); });

                if (!running)
                    return;

                // Process up to 10 transactions (adjust as needed)
                while (!transactionQueue.empty() && transactions.size() < 10)
                {
                    transactions.push_back(transactionQueue.front());
                    transactionQueue.pop();
                }
            }

            if (transactions.empty())
                continue;

            logMessage("Mining Main block with " + to_string(transactions.size()) +
                       " transactions by Node " + to_string(nodeId));

            MainBlock newBlock(blockchain.getChain().size(), blockchain.getLastBlock().hash, transactions);
            newBlock.mineBlock(difficulty);

            if (verifyNewBlock(newBlock))
            {
                blockchain.addBlock(newBlock);
                broadcastBlock(newBlock, 0);
                cout << "aaaaaaaaaaaaaaaaaaaaaaaaaaa";
                logMessage("Valid block mined by Node " + to_string(nodeId) + ": " + newBlock.hash);

                // Update blockchain and mempool files
                updateBlockchainFile(newBlock);
                updateMempoolFile(transactions);
            }
            else
            {
                logMessage("Invalid block mined by Node " + to_string(nodeId));
            }

            this_thread::sleep_for(chrono::seconds(1)); // Prevent tight loop
        }
        catch (const exception &e)
        {
            logMessage("Error in mining by Node " + to_string(nodeId) + ": " + e.what());
        }
    }
}

void MainNode::updateBlockchainFile(const MainBlock &block)
{
    string filename = "./data/" + to_string(nodeId) + "_mainBlockchain.json";
    json blockchainJson = json::array();

    ifstream inFile(filename);
    if (inFile.is_open())
    {
        try
        {
            inFile >> blockchainJson;
        }
        catch (const json::parse_error &e)
        {
            cerr << "Error parsing JSON: " << e.what() << endl;
        }
        inFile.close();
    }

    json blockJson = {
        {"index", block.index},
        {"previousHash", block.previousHash},
        {"hash", block.hash},
        {"timestamp", block.timestamp},
        {"games", json::array()},
        {"nonce", block.nonce}};
    for (const auto &txn : block.games)
    {
        blockJson["games"].push_back(txn.toString());
    }
    blockchainJson.push_back(blockJson);

    ofstream outFile(filename, ios::trunc);
    if (outFile.is_open())
    {
        outFile << blockchainJson.dump(4);
        outFile.close();
    }
}

void MainNode::updateMempoolFile(const vector<Game> &transactions)
{
    string filename = "./data/" + to_string(nodeId) + "_mainMempool.json";
    json mempool = json::array();

    ifstream inFile(filename);
    if (inFile.is_open())
    {
        try
        {
            inFile >> mempool;
        }
        catch (const json::parse_error &e)
        {
            cerr << "Error parsing JSON: " << e.what() << endl;
        }
        inFile.close();
    }

    for (const auto &txn : transactions)
    {
        mempool.erase(remove_if(mempool.begin(), mempool.end(),
                                [&txn](const json &mempoolTxn)
                                {
                                    return mempoolTxn["gameId"] == txn.gameId &&
                                           mempoolTxn["players"] == txn.players &&
                                           mempoolTxn["winnerId"] == txn.winnerId &&
                                           mempoolTxn["gameComplete"] == txn.gameComplete;
                                }),
                      mempool.end());
    }

    ofstream outFile(filename, ios::trunc);
    if (outFile.is_open())
    {
        outFile << mempool.dump(4);
        outFile.close();
    }
}

void MainNode::connectPeer(MainNode *peer)
{
    if (peer == this)
    {
        logMessage("Cannot connect to self: Node " + to_string(nodeId));
        return;
    }
    if (find(peers.begin(), peers.end(), peer) == peers.end())
    {
        peers.push_back(peer);
        if (find(peer->peers.begin(), peer->peers.end(), this) == peer->peers.end())
        {
            peer->peers.push_back(this); // Ensure bidirectional connection
        }
        logMessage("Node " + to_string(nodeId) + " connected to Node " + to_string(peer->nodeId));
    }
    else
    {
        logMessage("Node " + to_string(nodeId) + " is already connected to Node " + to_string(peer->nodeId));
    }
}

void MainNode::broadcastTransaction(const Game &txn)
{
    for (auto peer : peers)
    {
        logMessage("Transaction broadcasted from Node " + to_string(nodeId) + " to Node " + to_string(peer->nodeId));
        peer->receiveTransaction(txn, this);
    }
}
void MainNode::broadcastTransaction(const Game &txn, MainNode *speer)
{
    for (auto peer : peers)
    {
        if (speer->nodeId == peer->nodeId)
            continue;
        logMessage("Transaction broadcasted from Node " + to_string(nodeId) + " to Node " + to_string(peer->nodeId));
        peer->receiveTransaction(txn, this);
    }
}

bool MainNode::verifyValidGame(const Game &game)
{
    // Verify that the game has a valid chain of blocks
    const auto &chain = game.getChain();
    for (size_t i = 1; i < chain.size(); ++i)
    {
        const auto &currentBlock = chain[i];
        const auto &previousBlock = chain[i - 1];

        // Check if the current block's previousHash matches the hash of the previous block
        if (currentBlock.previousHash != previousBlock.hash)
        {
            return false;
        }

        // Check if the block's hash meets the difficulty criteria
        string target(currentBlock.difficulty, '0');
        if (currentBlock.hash.substr(0, currentBlock.difficulty) != target)
        {
            return false;
        }

        // Verify the integrity of the block's transactions
        for (const auto &txn : currentBlock.moves)
        {
            if (!txn.isValid())
            {
                return false;
            }
        }
    }

    // Additional checks can be added here, such as verifying the game's winner or rules
    return true;
}

void MainNode::syncPeers()
{
    if (peers.empty())
        return;

    MainChain *longestChain = &blockchain;
    MainNode *sourcePeer = nullptr;
    size_t maxLength = blockchain.getChain().size();

    for (auto peer : peers)
    {
        if (peer->blockchain.getChain().size() > maxLength && peer->running)
        {
            maxLength = peer->blockchain.getChain().size();
            longestChain = &peer->blockchain;
            sourcePeer = peer;
        }
    }

    if (longestChain != &blockchain)
    {
        blockchain = *longestChain; // Adopt the longest chain
        logMessage("Node " + to_string(nodeId) + " synced blockchain with Node " +
                   (sourcePeer ? to_string(sourcePeer->nodeId) : "unknown"));
    }

    // Sync transactions
    for (auto peer : peers)
    {
        if (!peer->running)
            continue;
        queue<Game> tempQueue = peer->transactionQueue;
        while (!tempQueue.empty())
        {
            auto &txn = tempQueue.front();
            tempQueue.pop();
            if (isValidTransaction(txn) && verifyValidGame(txn))
            {
                addTransaction(txn);
                logMessage("Transaction synced from Node " + to_string(peer->nodeId) +
                           " to Node " + to_string(nodeId));
            }
        }
    }
}

void MainNode::broadcastBlock(const MainBlock &block, int peerId)
{
    for (auto peer : peers)
    {
        if (peerId != 0)
            if (peer->nodeId == peerId)
                continue;
        peer->receiveBlock(block, this);
        logMessage("Block" + block.hash + "broadcasted from Node " + to_string(nodeId) + " to Node " + to_string(peer->nodeId));
    }
}

void MainNode::receiveTransaction(const Game &txn, MainNode *peer)
{
    addTransaction(txn, peer); // Reuse your validation logic
}

void MainNode::receiveBlock(const MainBlock &block, MainNode *peer)
{
    if (peer == nullptr)
    {
        logMessage("Received block from NULL peer.");
        return;
    }

    cout << "Received block from Node " << peer->nodeId << " me " << this->nodeId << endl;
    // Validate and add block if valid
    for (const auto &existingBlock : blockchain.getChain())
    {
        if (existingBlock.hash == block.hash)
        {
            logMessage("Block already in blockchain, hash: " + block.hash + ", from Node " + to_string(peer->nodeId));
            return;
        }
    }
    cout << "Received block from Node " << peer->nodeId << verifyNewBlock(block) << endl;
    if (verifyNewBlock(block))
    {
        cout << "Valid block received from Node " << peer->nodeId << endl;
        blockchain.addBlock(block);
        broadcastBlock(block, peer->nodeId);
        // Remove transactions in the block from the transaction queue
        {
            lock_guard<mutex> lock(mtx);
            queue<Game> tempQueue;
            while (!transactionQueue.empty())
            {
                const auto &txn = transactionQueue.front();
                bool found = false;
                for (const auto &blockTxn : block.games)
                {
                    if (txn.gameId == blockTxn.gameId &&
                        txn.players == blockTxn.players &&
                        txn.winnerId == blockTxn.winnerId &&
                        txn.gameComplete == blockTxn.gameComplete)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    tempQueue.push(txn);
                }
                transactionQueue.pop();
            }
            transactionQueue = move(tempQueue);
        }

        // Remove transactions in the block from the mempool
        string mempoolFilename = "./data/" + to_string(nodeId) + "_mainMempool.json";
        json mempool = json::array();

        ifstream mempoolInFile(mempoolFilename);
        if (mempoolInFile.is_open())
        {
            try
            {
                mempoolInFile >> mempool;
            }
            catch (const json::parse_error &e)
            {
                cerr << "Error parsing JSON 4: " << e.what() << endl;
            }
            mempoolInFile.close();
        }

        for (const auto &txn : block.games)
        {
            auto it = remove_if(mempool.begin(), mempool.end(),
                                [&txn](const json &mempoolTxn)
                                {
                                    return mempoolTxn["gameId"] == txn.gameId &&
                                           mempoolTxn["players"] == txn.players &&
                                           mempoolTxn["winnerId"] == txn.winnerId &&
                                           mempoolTxn["gameComplete"] == txn.gameComplete;
                                });
            mempool.erase(it, mempool.end());
        }

        ofstream mempoolOutFile(mempoolFilename, ios::trunc);
        if (mempoolOutFile.is_open())
        {
            mempoolOutFile << mempool.dump(4); // Pretty print with 4 spaces
            mempoolOutFile.close();
        }
        string filename = "./data/" + to_string(nodeId) + "_mainBlockchain.json";
        ifstream checkFile(filename);
        if (checkFile.peek() == ifstream::traits_type::eof())
        {
            ofstream initFile(filename);
            if (initFile.is_open())
            {
                initFile << "[]";
                initFile.close();
            }
        }
        json blockchainJson;

        ifstream inFile(filename);
        if (inFile.is_open())
        {
            inFile >> blockchainJson;
            inFile.close();
        }

        json blockJson = {
            {"index", block.index},
            {"previousHash", block.previousHash},
            {"hash", block.hash},
            {"timestamp", block.timestamp},
            {"games", json::array()},
            {"nonce", block.nonce}};

        for (const auto &txn : block.games)
        {
            blockJson["games"].push_back(txn.toString());
        }

        blockchainJson.push_back(blockJson);

        ofstream outFile(filename);
        if (outFile.is_open())
        {
            outFile << blockchainJson.dump(4); // Pretty print with 4 spaces
            outFile.close();
        }
    }
}

bool MainNode::verifyNewBlock(const MainBlock &block)
{
    cout << blockchain.getLastBlock().hash << " " << block.hash << " " << block.difficulty << endl;
    if (blockchain.getLastBlock().hash != block.previousHash)
        return false;

    cout << "here1" << endl;

    for (const auto &txn : block.games)
    {
        if (!isValidTransaction(txn))
            return false;
        cout << "here2" << endl;
    }

    string target(block.difficulty, '0');
    if (block.hash.substr(0, block.difficulty) != target)
        return false;

    cout << "here3" << endl;

    return true;
}

void MainNode::addTransaction(const Game &txn)
{
    if (!isValidTransaction(txn) || !verifyValidGame(txn))
    {
        logMessage("Invalid transaction received by Node " + to_string(nodeId));
        return;
    }
    // Check blockchain for duplicates
    // for (const auto &block : blockchain.getChain())
    // {
    //     if (block.games.empty())
    //     {
    //         continue;
    //     }
    //     for (const auto &blockTxn : block.games)
    //     {
    //         if (blockTxn.gameId == txn.gameId)
    //         {
    //             logMessage("Transaction already in blockchain, gameId: " + txn.gameId);
    //             return;
    //         }
    //     }
    // }
    {
        lock_guard<mutex> lock(mtx);
        queue<Game> tempQueue = transactionQueue;
        while (!tempQueue.empty())
        {
            const auto &queuedTxn = tempQueue.front();
            if (queuedTxn.gameId == txn.gameId && queuedTxn.players == txn.players &&
                queuedTxn.winnerId == txn.winnerId && queuedTxn.gameComplete == txn.gameComplete)
            {
                logMessage("Transaction already exists in queue for Node " + to_string(nodeId));
                return;
            }
            tempQueue.pop();
        }
        transactionQueue.push(txn);
    }
    cv.notify_all();
    broadcastTransaction(txn);

    // Update mempool file
    string filename = "./data/" + to_string(nodeId) + "_mainMempool.json";
    json mempool = json::array();

    ifstream inFile(filename);
    if (inFile.is_open())
    {
        try
        {
            inFile >> mempool;
        }
        catch (const json::parse_error &e)
        {
            cerr << "Error parsing JSON: " << e.what() << endl;
        }
        inFile.close();
    }

    json txnJson = {
        {"gameId", txn.gameId},
        {"players", txn.players},
        {"winnerId", txn.winnerId},
        {"gameComplete", txn.gameComplete}};
    mempool.push_back(txnJson);

    ofstream outFile(filename, ios::trunc);
    if (outFile.is_open())
    {
        outFile << mempool.dump(4);
        outFile.close();
    }

    logMessage("Transaction added to Node " + to_string(nodeId) + ": " + to_string(txn.gameId));
}
void MainNode::addTransaction(const Game &txn, MainNode *peer)
{

    if (!isValidTransaction(txn) || !verifyValidGame(txn))
    {
        logMessage("Invalid transaction received by Node " + to_string(nodeId));
        return;
    }
    // Check blockchain for duplicates
    // for (const auto &block : blockchain.getChain())
    // {
    //     if (block.games.empty())
    //     {
    //         continue;
    //     }
    //     for (const auto &blockTxn : block.games)
    //     {
    //         if (blockTxn.gameId == txn.gameId)
    //         {
    //             logMessage("Transaction already in blockchain, gameId: " + txn.gameId);
    //             return;
    //         }
    //     }
    // }
    {
        lock_guard<mutex> lock(mtx);
        queue<Game> tempQueue = transactionQueue;
        while (!tempQueue.empty())
        {
            const auto &queuedTxn = tempQueue.front();
            if (queuedTxn.gameId == txn.gameId && queuedTxn.players == txn.players &&
                queuedTxn.winnerId == txn.winnerId && queuedTxn.gameComplete == txn.gameComplete)
            {
                logMessage("Transaction already exists in queue for Node " + to_string(nodeId));
                return;
            }
            tempQueue.pop();
        }
        transactionQueue.push(txn);
    }
    cv.notify_all();
    broadcastTransaction(txn, peer);

    // Update mempool file
    string filename = "./data/" + to_string(nodeId) + "_mainMempool.json";
    json mempool = json::array();

    ifstream inFile(filename);
    if (inFile.is_open())
    {
        try
        {
            inFile >> mempool;
        }
        catch (const json::parse_error &e)
        {
            cerr << "Error parsing JSON: " << e.what() << endl;
        }
        inFile.close();
    }

    json txnJson = {
        {"gameId", txn.gameId},
        {"players", txn.players},
        {"winnerId", txn.winnerId},
        {"gameComplete", txn.gameComplete}};
    mempool.push_back(txnJson);

    ofstream outFile(filename, ios::trunc);
    if (outFile.is_open())
    {
        outFile << mempool.dump(4);
        outFile.close();
    }

    logMessage("Transaction added to Node " + to_string(nodeId) + ": " + to_string(txn.gameId));
}

void MainNode::stop()
{
    {
        lock_guard<mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
}

MainNode::~MainNode()
{
    stop();
}
