#include "Player.hpp"
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

void Player::generateKeyPair()
{
    // Use OpenSSL to generate an actual public-private key pair
    EVP_PKEY *pkey = EVP_PKEY_new();
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    if (!ctx || !pkey)
    {
        throw runtime_error("Failed to initialize OpenSSL context.");
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        throw runtime_error("Failed to initialize key generation.");
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        throw runtime_error("Failed to set RSA key size.");
    }

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        throw runtime_error("Key generation failed.");
    }

    EVP_PKEY_CTX_free(ctx);

    // Extract private key
    BIO *privateKeyBio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PrivateKey(privateKeyBio, pkey, NULL, NULL, 0, NULL, NULL))
    {
        EVP_PKEY_free(pkey);
        BIO_free(privateKeyBio);
        throw runtime_error("Failed to write private key.");
    }

    char *privateKeyData;
    long privateKeyLen = BIO_get_mem_data(privateKeyBio, &privateKeyData);
    privateKey = string(privateKeyData, privateKeyLen);
    BIO_free(privateKeyBio);

    // Extract public key
    BIO *publicKeyBio = BIO_new(BIO_s_mem());
    if (!PEM_write_bio_PUBKEY(publicKeyBio, pkey))
    {
        EVP_PKEY_free(pkey);
        BIO_free(publicKeyBio);
        throw runtime_error("Failed to write public key.");
    }

    char *publicKeyData;
    long publicKeyLen = BIO_get_mem_data(publicKeyBio, &publicKeyData);
    publicKey = string(publicKeyData, publicKeyLen);
    BIO_free(publicKeyBio);

    EVP_PKEY_free(pkey);
}

Player::Player(int diff) : difficulty(diff), blockchain(*(new Game()))
{
    opponent = nullptr;
    generateKeyPair();
    string sanitizedPublicKey = publicKey;
    sanitizedPublicKey.erase(remove(sanitizedPublicKey.begin(), sanitizedPublicKey.end(), '\n'), sanitizedPublicKey.end());
    nodeId = sanitizedPublicKey.substr(sanitizedPublicKey.size() - 40);
    logMessage("Node " + nodeId + " started");

    // Initialize {nodeId}_mempool.json with an empty array
    string mempoolFilename = "./data/" + nodeId + "_mempool.json";
    ifstream mempoolCheckFile(mempoolFilename);
    ofstream mempoolInitFile(mempoolFilename);
    if (mempoolInitFile.is_open())
    {
        mempoolInitFile << "[]"; // Initialize with an empty array
        mempoolInitFile.close();
    }
    // Initialize {nodeId}_blockchain.json with an empty array
    string filename = "./data/" + nodeId + "_blockchain.json";
    ofstream blockchainInitFile(filename);
    if (blockchainInitFile.is_open())
    {
        blockchainInitFile << "[]"; // Initialize with an empty array
        blockchainInitFile.close();
    }
}

void Player::createMove(string data)
{
    if (data.size() <= 0)
    {
        throw invalid_argument("Data must not be empty");
    }

    if (this->opponent == nullptr)
    {
        cerr << "Opponent is not set." << endl;
        return;
    }

    Move transaction(publicKey, this->opponent->publicKey, data);

    transaction.signTransaction(privateKey);
    if (!transaction.isValid())
    {
        throw runtime_error("Transaction signature is invalid.");
    }

    // Sanitize sender and recipient addresses
    string sanitizedSender = transaction.sender;
    sanitizedSender.erase(remove(sanitizedSender.begin(), sanitizedSender.end(), '\n'), sanitizedSender.end());

    string sanitizedRecipient = transaction.receiver;
    sanitizedRecipient.erase(remove(sanitizedRecipient.begin(), sanitizedRecipient.end(), '\n'), sanitizedRecipient.end());

    // Read the existing transactions from createdTransaction.json
    ifstream inputFile("createdMove.json");
    json transactions = json::array();

    std::cout << "Reading existing transactions from createdMove.json..." << std::endl;

    if (inputFile.is_open())
    {
        try
        {
            inputFile >> transactions;
        }
        catch (const json::parse_error &e)
        {
            cerr << "Error parsing JSON: " << e.what() << endl;
        }
        inputFile.close();
    }

    // Append the new transaction
    json newTransaction = {
        {"id", transaction.id},
        {"sender", sanitizedSender},
        {"recipient", sanitizedRecipient},
        {"data", transaction.data}};

    transactions.push_back(newTransaction);

    // Write the updated transactions back to createdTransaction.json
    ofstream outputFile("createdMove.json", ios::trunc);
    if (!outputFile.is_open())
    {
        throw runtime_error("Failed to open createdMove.json for writing.");
    }

    outputFile << transactions.dump(4); // Pretty print with 4 spaces

    std::cout << "move added" << endl;
    outputFile.close();

    this->addMove(transaction);

    logMessage("Transaction added: " + transaction.toString());
}

bool Player::gameStrated(Player &opponent, Game &newChain)
{
    if (this->opponent == nullptr && &opponent != nullptr)
    {
        logMessage("Game started" + newChain.toString());
        this->opponent = &opponent;
        this->blockchain = newChain;
        this->connectPeer(ref(opponent));

        string filename = "./data/" + nodeId + "_blockchain.json";

        // Add the genesis block to {nodeId}_blockchain.json
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

        BlockGame genesisBlock = blockchain.getChain().front();
        json blockJson = {
            {"index", genesisBlock.index},
            {"previousHash", genesisBlock.previousHash},
            {"hash", genesisBlock.hash},
            {"timestamp", genesisBlock.timestamp},
            {"moves", json::array()},
            {"nonce", genesisBlock.nonce},
            {"gameId", blockchain.gameId}};

        blockchainJson.push_back(blockJson);

        ofstream outputFile(filename, ios::trunc);
        if (!outputFile.is_open())
        {
            throw runtime_error("Failed to open " + filename + " for writing.");
        }

        outputFile << blockchainJson.dump(4); // Pretty print with 4 spaces
        outputFile.close();
        return true;
    }
    else if (this->opponent != nullptr && &opponent == nullptr)
    {
        this->opponent = nullptr;
        return false;
    }
}

void Player::logMessage(const string &message)
{
    ifstream inputFile("logs.json");
    json logs = json::array();

    std::cout << "Logging message: " << message << endl;

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

bool Player::isValidMove(const Move &txn)
{
    if (txn.data.size() <= 0)
        return false;
    if (txn.sender.empty() || txn.receiver.empty())
        return false;
    // ADD MORE CHECKS ON MOVES
    return true;
}

void Player::sendCompleteGame()
{
    if (mainNodes.size() == 0)
    {
        cerr << "No MainNode connected." << endl;
        return;
    }
    while (completeGames.size() > 0)
    {

        for (auto mainNode : mainNodes)
        {
            if (mainNode == nullptr || !mainNode->running)
            {
                cerr << "MainNode is not available" << endl;
                continue;
            }
            Game tempGame = completeGames.front();
            try
            {
                // Sending the complete game (tempGame) to the the connected Main Node
                mainNode->addTransaction(tempGame);

                completeGames.pop();

                // Remove the game from {nodeId}_completeGames.json
                string filename = "./data/" + nodeId + "_completeGames.json";
                json completeGamesJson = json::array();

                ifstream inFile(filename);
                if (inFile.is_open())
                {
                    try
                    {
                        inFile >> completeGamesJson;
                    }
                    catch (const json::parse_error &e)
                    {
                        cerr << "Error parsing JSON: " << e.what() << endl;
                    }
                    inFile.close();
                }

                // Find and remove the game from the JSON array
                auto it = remove_if(completeGamesJson.begin(), completeGamesJson.end(),
                                    [&tempGame](const json &gameJson)
                                    {
                                        int index = 0;
                                        for (auto &blockJson : gameJson)
                                        {
                                            if (!(blockJson["index"] == tempGame.getChain()[index].index &&
                                                  blockJson["previousHash"] == tempGame.getChain()[index].previousHash &&
                                                  blockJson["hash"] == tempGame.getChain()[index].hash &&
                                                  blockJson["timestamp"] == tempGame.getChain()[index].timestamp &&
                                                  blockJson["nonce"] == tempGame.getChain()[index].nonce))
                                            {
                                                return false;
                                            }
                                            index++;
                                        }
                                        return true;
                                    });

                completeGamesJson.erase(it, completeGamesJson.end());

                ofstream outFile(filename, ios::trunc);
                if (outFile.is_open())
                {
                    outFile << completeGamesJson.dump(4); // Pretty print with 4 spaces
                    outFile.close();
                }
            }
            catch (const exception &e)
            {
                std::cout << "Error in sending complete game: " << e.what() << endl;
                // Handle the error as needed
            }
        }
    }
}
void Player::mineBlock()
{
    while (running)
    {
        if (blockchain.getChain().size() == 3)
        {
            logMessage("Game ended " + blockchain.toString());
            blockchain.endGame();
            this->addCompleteGame(blockchain);
            blockchain = *(new Game());
            opponent = nullptr;
            string filename = "./data/" + nodeId + "_blockchain.json";
            ofstream blockchainInitFile(filename);
            if (blockchainInitFile.is_open())
            {
                blockchainInitFile << "[]"; // Initialize with an empty array
                blockchainInitFile.close();
            }
        }
        sendCompleteGame();
        try
        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [this]
                    { return !running || transactionQueue.size() >= 5; });

            std::cout << "============================================" << endl;
            logMessage("Mining block..." + this->nodeId + " transactions in queue");

            if (!running)
                return;

            vector<Move> transactions;
            for (int i = 0; i < 5; i++)
            {
                transactions.push_back(transactionQueue.front());
                transactionQueue.pop();
            }
            mtx.unlock();

            BlockGame newBlock(blockchain.getChain().size(), blockchain.getLastBlock().hash, transactions);
            newBlock.mineBlock(difficulty);

            if (verifyNewBlock(newBlock))
            {
                std::cout << "Valid block mined Broadcating" << endl;
                broadcastBlock(newBlock, "0");
                std::cout << "here mining" << endl;
            }
            else
            {
                std::cout << "Invalid block mined by Node " << nodeId << endl;
                continue;
            }

            blockchain.addBlock(newBlock);
            string filename = "./data/" + nodeId + "_blockchain.json";
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
                    cerr << "Error parsing JSON 3: " << e.what() << endl;
                }
                inputFile.close();
            }

            json blockJson = {
                {"index", newBlock.index},
                {"previousHash", newBlock.previousHash},
                {"hash", newBlock.hash},
                {"timestamp", newBlock.timestamp},
                {"moves", json::array()},
                {"nonce", newBlock.nonce},
                {"gameId", blockchain.gameId}};

            for (const auto &txn : newBlock.moves)
            {
                blockJson["moves"].push_back({{"id", txn.id}, {"sender", txn.sender}, {"receiver", txn.receiver}, {"amount", txn.data}});
            }

            blockchainJson.push_back(blockJson);

            ofstream outputFile(filename);
            if (!outputFile.is_open())
            {
                throw runtime_error("Failed to open createdMoves.json for writing.");
            }

            outputFile << blockchainJson.dump(4); // Pretty print with 4 spaces

            std::cout << "block added" << endl;
            outputFile.close();

            string mempoolFilename = "./data/" + nodeId + "_mempool.json";
            json mempool = json::array(); // Initialize as an array

            ifstream mempoolInFile(mempoolFilename);
            if (mempoolInFile.is_open())
            {
                std::cout << "opened" << endl;
                mempoolInFile >> mempool;
                mempoolInFile.close();
            }

            std::cout << mempool.size() << endl;
            std::cout << "Deleting from mempool" << endl;
            for (const auto &txn : transactions)
            {
                auto it = remove_if(mempool.begin(), mempool.end(),
                                    [&txn](const json &mempoolTxn)
                                    {
                                        return mempoolTxn["id"] == txn.id &&
                                               mempoolTxn["sender"] == txn.sender &&
                                               mempoolTxn["receiver"] == txn.receiver &&
                                               mempoolTxn["data"] == txn.data;
                                    });
                mempool.erase(it, mempool.end());
            }

            ofstream mempoolOutFile(mempoolFilename);
            if (mempoolOutFile.is_open())
            {
                mempoolOutFile << mempool.dump(4); // Pretty print with 4 spaces
                mempoolOutFile.close();
            }

            logMessage("Block mined by Node " + nodeId + ": " + newBlock.hash);

            std::cout << "Block mined by Node " << nodeId << ": " << newBlock.hash << endl;

            cv.wait_for(lock, chrono::seconds(2));

            std::cout
                << "-----------------------------------------" << endl;
        }
        catch (const exception &e)
        {
            std::cout << "Error in mining: " << e.what() << endl;
            // Handle the error as needed
        }
    }
}

bool Player::verifyValidGame(const Game &game)
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
            if (!isValidMove(txn) || !txn.isValid())
            {
                return false;
            }
        }
    }

    // Additional checks can be added here, such as verifying the game's winner or rules
    return true;
}

void Player::addCompleteGame(const Game &game)
{
    if (verifyValidGame(game))
    {
        completeGames.push(game);

        string filename = "./data/" + nodeId + "_completeGames.json";
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

        json completeGamesJson = json::array();

        ifstream inFile(filename);
        if (inFile.is_open())
        {
            try
            {
                inFile >> completeGamesJson;
            }
            catch (const json::parse_error &e)
            {
                cerr << "Error parsing JSON: " << e.what() << endl;
            }
            inFile.close();
        }

        json gameJson = json::array();
        for (const auto &block : game.getChain())
        {
            json blockJson = {
                {"index", block.index},
                {"previousHash", block.previousHash},
                {"hash", block.hash},
                {"timestamp", block.timestamp},
                {"moves", json::array()},
                {"nonce", block.nonce}};

            for (const auto &txn : block.moves)
            {
                blockJson["moves"].push_back({{"id", txn.id}, {"sender", txn.sender}, {"receiver", txn.receiver}, {"data", txn.data}});
            }

            gameJson.push_back(blockJson);
        }

        completeGamesJson.push_back(gameJson);

        ofstream outFile(filename, ios::trunc);
        if (outFile.is_open())
        {
            outFile << completeGamesJson.dump(4); // Pretty print with 4 spaces
            outFile.close();
        }
    }
    else
        cerr << "Invalid game data. Cannot add to complete games." << endl;
}

void Player::connectPeer(Player &peer)
{
    // Check if the peer is already connected
    Player *targetPtr = &peer;

    if (find(peers.begin(), peers.end(), targetPtr) == peers.end())
    {
        peers.push_back(&peer);
        peer.connectPeer(*this);
        peer.logMessage("Node " + peer.nodeId + " and Node " + nodeId + " are now connected");
    }
    else
    {
        logMessage("Node " + nodeId + " is already connected to Node " + peer.nodeId);
    }
}
void Player::connectNode(MainNode &peer)
{
    // Check if the peer is already connected
    MainNode *targetPtr = &peer;

    if (find(mainNodes.begin(), mainNodes.end(), targetPtr) == mainNodes.end())
    {
        mainNodes.push_back(&peer);
        this->logMessage("Node " + to_string(peer.nodeId) + " and Player " + nodeId + " are now connected");
    }
    else
    {
        logMessage("Player " + nodeId + " is already connected to Node " + to_string(peer.nodeId));
    }
}

void Player::broadcastTransaction(const Move &txn)
{
    for (auto peer : peers)
    {
        logMessage("Transaction broadcasted from Node " + nodeId + " to Node " + peer->nodeId);
        peer->receiveTransaction(txn, this);
    }
}
void Player::broadcastTransaction(const Move &txn, Player *speer)
{
    for (auto peer : peers)
    {
        if (speer->nodeId == peer->nodeId)
            continue;
        logMessage("Transaction broadcasted from Node " + nodeId + " to Node " + peer->nodeId);
        peer->receiveTransaction(txn, this);
    }
}

void Player::syncPeers()
{
    if (peers.size() > 0)
    {
        for (auto peer : peers)
        {
            // Sync blockchain
            if (peer->blockchain.getChain().size() > blockchain.getChain().size())
            {
                std::cout << "Syncing with peers..." << endl;
                blockchain = peer->blockchain;
                logMessage("Node " + nodeId + " synced blockchain with Node " + peer->nodeId);

                if (peer->transactionQueue.empty())
                {
                    std::cout << "No transactions to sync from Node " + peer->nodeId << endl;
                    continue;
                }
                // Sync transactionQueue
                queue<Move> tempQueue = peer->transactionQueue;
                while (!tempQueue.empty())
                {
                    auto &txn = tempQueue.front();
                    tempQueue.pop();

                    bool alreadyExists = false;

                    lock_guard<mutex> lock(mtx);
                    queue<Move> localQueue = transactionQueue;
                    while (!localQueue.empty())
                    {
                        const auto &queuedTxn = localQueue.front();
                        localQueue.pop();

                        if (queuedTxn.id == txn.id &&
                            queuedTxn.sender == txn.sender &&
                            queuedTxn.receiver == txn.receiver &&
                            queuedTxn.data == txn.data)
                        {
                            alreadyExists = true;
                            break;
                        }
                    }

                    if (!alreadyExists && isValidMove(txn) && txn.isValid())
                    {
                        addMove(txn);
                        logMessage("Transaction synced from Node " + peer->nodeId + " to Node " + nodeId);
                    }
                }
            }
        }
    }
}

void Player::broadcastBlock(const BlockGame &block, string peerId)
{
    for (auto peer : peers)
    {
        if (peerId != "")
            if (peer->nodeId == peerId)
                continue;
        peer->receiveBlock(block, this);
        logMessage("Block" + block.hash + "broadcasted from Node " + nodeId + " to Node " + peer->nodeId);
    }
}

void Player::receiveTransaction(const Move &txn, Player *peer)
{
    addTransaction(txn, peer); // Reuse your validation logic
}

void Player::receiveBlock(const BlockGame &block, Player *peer)
{
    // Validate and add block if valid
    std::cout << "Received block from Node " << peer->nodeId << verifyNewBlock(block) << endl;
    if (verifyNewBlock(block))
    {
        std::cout << "Valid block received from Node " << peer->nodeId << endl;
        blockchain.addBlock(block);
        if (blockchain.getChain().size() == 3)
        {
            logMessage("Game ended " + blockchain.toString());
            blockchain.endGame();
            this->addCompleteGame(blockchain);
            blockchain = *(new Game());
            opponent = nullptr;
            string filename = "./data/" + nodeId + "_blockchain.json";
            ofstream blockchainInitFile(filename);
            if (blockchainInitFile.is_open())
            {
                blockchainInitFile << "[]"; // Initialize with an empty array
                blockchainInitFile.close();
            }
        }
        broadcastBlock(block, peer->nodeId);
        // Remove transactions in the block from the transaction queue
        {
            lock_guard<mutex> lock(mtx);
            queue<Move> tempQueue;
            while (!transactionQueue.empty())
            {
                const auto &txn = transactionQueue.front();
                bool found = false;
                for (const auto &blockTxn : block.moves)
                {
                    if (txn.id == blockTxn.id &&
                        txn.sender == blockTxn.sender &&
                        txn.receiver == blockTxn.receiver &&
                        txn.data == blockTxn.data)
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
        string mempoolFilename = "./data/" + nodeId + "_mempool.json";
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

        for (const auto &txn : block.moves)
        {
            auto it = remove_if(mempool.begin(), mempool.end(),
                                [&txn](const json &mempoolTxn)
                                {
                                    return mempoolTxn["id"] == txn.id &&
                                           mempoolTxn["sender"] == txn.sender &&
                                           mempoolTxn["receiver"] == txn.receiver &&
                                           mempoolTxn["data"] == txn.data;
                                });
            mempool.erase(it, mempool.end());
        }

        ofstream mempoolOutFile(mempoolFilename, ios::trunc);
        if (mempoolOutFile.is_open())
        {
            mempoolOutFile << mempool.dump(4); // Pretty print with 4 spaces
            mempoolOutFile.close();
        }
        string filename = "./data/" + nodeId + "_blockchain.json";
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
            {"moves", json::array()},
            {"nonce", block.nonce},
            {"gameId", blockchain.gameId}};

        for (const auto &txn : block.moves)
        {
            blockJson["moves"].push_back({{"id", txn.id},
                                          {"sender", txn.sender},
                                          {"receiver", txn.receiver},
                                          {"data", txn.data}});
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

bool Player::verifyNewBlock(const BlockGame &block)
{
    std::cout << blockchain.getLastBlock().hash << " " << block.previousHash << endl;
    if (blockchain.getLastBlock().hash != block.previousHash)
        return false;

    for (const auto &txn : block.moves)
    {
        if (!isValidMove(txn))
            return false;
    }

    string target(block.difficulty, '0');
    if (block.hash.substr(0, block.difficulty) != target)
        return false;

    return true;
}

void Player::addMove(const Move &txn)
{
    if (isValidMove(txn))
    {

        queue<Move> tempQueue = transactionQueue;
        while (!tempQueue.empty())
        {
            const auto &queuedTxn = tempQueue.front();
            tempQueue.pop();

            if (queuedTxn.id == txn.id &&
                queuedTxn.sender == txn.sender &&
                queuedTxn.receiver == txn.receiver &&
                queuedTxn.data == txn.data)
            {
                std::cout << "Transaction already exists in the transactionQueue" << endl;
                return;
            }
        }

        {
            lock_guard<mutex> lock(mtx);
            std::cout << "here" << endl;
            std::cout << txn.data << endl;
            transactionQueue.push(txn);
        }
        cv.notify_all();

        string filename = "./data/" + nodeId + "_mempool.json";
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
                cerr << "Error parsing JSON 5: " << e.what() << endl;
            }
            inFile.close();
        }

        json txnJson = {
            {"id", txn.id},
            {"sender", txn.sender},
            {"receiver", txn.receiver},
            {"data", txn.data}};

        mempool.push_back(txnJson);

        std::cout << mempool.size() << endl;
        ofstream outFile(filename, ios::trunc);
        if (outFile.is_open())
        {
            outFile << mempool.dump(4);
            outFile.close();
        }

        // logMessage("Transaction added to Node " + nodeId + ": " + txn.toString());
        broadcastTransaction(txn);
    }
    else
    {
        std::cout << "P1: Invalid transaction" << endl;
    }
}
void Player::addTransaction(const Move &txn, Player *peer)
{
    if (isValidMove(txn))
    {

        queue<Move> tempQueue = transactionQueue;
        while (!tempQueue.empty())
        {
            const auto &queuedTxn = tempQueue.front();
            tempQueue.pop();

            if (queuedTxn.id == txn.id &&
                queuedTxn.sender == txn.sender &&
                queuedTxn.receiver == txn.receiver &&
                queuedTxn.data == txn.data)
            {
                std::cout << "Transaction already exists in the transactionQueue" << endl;
                return;
            }
        }
        {
            lock_guard<mutex> lock(mtx);
            std::cout << txn.data << endl;
            transactionQueue.push(txn);
        }
        cv.notify_all();
        broadcastTransaction(txn, peer);
        std::cout << "Node2 created successfully!" << endl;

        string filename = "./data/" + nodeId + "_mempool.json";
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
                cerr << "Error parsing JSON 6: " << e.what() << endl;
            }
            inFile.close();
        }

        json txnJson = {
            {"id", txn.id},
            {"sender", txn.sender},
            {"receiver", txn.receiver},
            {"data", txn.data}};

        mempool.push_back(txnJson);

        std::cout << mempool.size() << endl;
        ofstream outFile(filename, ios::trunc);
        if (outFile.is_open())
        {
            outFile << mempool.dump(4);
            outFile.close();
        }

        // logMessage("Transaction added to Node " + nodeId + ": " + txn.toString());
        std::cout << "Node2 created successfully!" << endl;
    }
    else
    {
        std::cout << "P2: Invalid transaction" << endl;
    }
}

void Player::stop()
{
    {
        lock_guard<mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
}

Player::~Player()
{
    stop();
}
