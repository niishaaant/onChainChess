#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <queue>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// Forward declarations
class Transaction;
class Block;
class Blockchain;
class Player;
class MainNode;

// Utility functions
std::string sha256(const std::string &str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Simplified key pair for digital signatures
class KeyPair
{
private:
    EVP_PKEY *pkey = nullptr;

public:
    KeyPair()
    {
        // Generate RSA key pair
        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
        EVP_PKEY_keygen_init(ctx);
        EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
        EVP_PKEY_keygen(ctx, &pkey);
        EVP_PKEY_CTX_free(ctx);
    }

    ~KeyPair()
    {
        if (pkey)
            EVP_PKEY_free(pkey);
    }

    std::string getPublicKeyPEM() const
    {
        BIO *bio = BIO_new(BIO_s_mem());
        PEM_write_bio_PUBKEY(bio, pkey);

        BUF_MEM *bufferPtr;
        BIO_get_mem_ptr(bio, &bufferPtr);
        std::string publicKey(bufferPtr->data, bufferPtr->length);
        BIO_free(bio);

        return publicKey;
    }

    std::string sign(const std::string &message) const
    {
        EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
        EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey);
        EVP_DigestSignUpdate(mdctx, message.c_str(), message.length());

        size_t siglen;
        EVP_DigestSignFinal(mdctx, nullptr, &siglen);
        unsigned char *sig = (unsigned char *)OPENSSL_malloc(siglen);

        EVP_DigestSignFinal(mdctx, sig, &siglen);
        EVP_MD_CTX_free(mdctx);

        std::string signature((char *)sig, siglen);
        OPENSSL_free(sig);

        return signature;
    }

    bool verify(const std::string &message, const std::string &signature) const
    {
        EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
        EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey);
        EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.length());

        int result = EVP_DigestVerifyFinal(
            mdctx,
            (unsigned char *)signature.c_str(),
            signature.length());

        EVP_MD_CTX_free(mdctx);
        return result == 1;
    }

    bool verifyWithPublicKey(const std::string &message, const std::string &signature, const std::string &publicKeyPEM)
    {
        BIO *bio = BIO_new_mem_buf(publicKeyPEM.c_str(), -1);
        EVP_PKEY *pubkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);

        EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
        EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pubkey);
        EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.length());

        int result = EVP_DigestVerifyFinal(
            mdctx,
            (unsigned char *)signature.c_str(),
            signature.length());

        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pubkey);

        return result == 1;
    }
};

// Transaction class representing a move in the chess game
class Transaction
{
public:
    std::string from;      // Public key of the sender
    std::string to;        // Public key of the receiver
    std::string moveData;  // Chess move in standard notation
    std::string signature; // Digital signature of the transaction

    Transaction(const std::string &from, const std::string &to, const std::string &moveData)
        : from(from), to(to), moveData(moveData) {}

    std::string toString() const
    {
        return from + to + moveData;
    }

    std::string getHash() const
    {
        return sha256(toString() + signature);
    }

    void sign(const KeyPair &keyPair)
    {
        signature = keyPair.sign(toString());
    }

    bool verifySignature(const std::string &publicKey) const
    {
        KeyPair dummy; // Just used for the verify method
        return dummy.verifyWithPublicKey(toString(), signature, publicKey);
    }
};

// Block class containing multiple transactions
class Block
{
public:
    int index;
    std::string previousHash;
    std::vector<Transaction> transactions;
    long long nonce;
    std::string hash;
    std::string merkleRoot;
    long long timestamp;
    int difficulty;

    Block(int index, const std::string &previousHash, int difficulty = 4)
        : index(index), previousHash(previousHash), nonce(0), timestamp(0), difficulty(difficulty) {}

    void addTransaction(const Transaction &transaction)
    {
        transactions.push_back(transaction);
    }

    std::string calculateMerkleRoot()
    {
        std::vector<std::string> hashes;
        for (const auto &tx : transactions)
        {
            hashes.push_back(tx.getHash());
        }

        while (hashes.size() > 1)
        {
            std::vector<std::string> newHashes;
            for (size_t i = 0; i < hashes.size(); i += 2)
            {
                if (i + 1 < hashes.size())
                {
                    newHashes.push_back(sha256(hashes[i] + hashes[i + 1]));
                }
                else
                {
                    newHashes.push_back(sha256(hashes[i] + hashes[i]));
                }
            }
            hashes = newHashes;
        }

        return hashes.empty() ? "0" : hashes[0];
    }

    std::string calculateHash() const
    {
        std::stringstream ss;
        ss << index << previousHash << merkleRoot << nonce << timestamp;
        return sha256(ss.str());
    }

    void mineBlock()
    {
        merkleRoot = calculateMerkleRoot();
        timestamp = std::chrono::system_clock::now().time_since_epoch().count();

        std::string target(difficulty, '0');

        do
        {
            nonce++;
            hash = calculateHash();
        } while (hash.substr(0, difficulty) != target);

        std::cout << "Block mined: " << hash << std::endl;
    }

    bool isValid() const
    {
        if (hash.substr(0, difficulty) != std::string(difficulty, '0'))
        {
            return false;
        }

        if (hash != calculateHash())
        {
            return false;
        }

        return true;
    }
};

// Blockchain class
class Blockchain
{
protected:
    std::vector<Block> chain;
    mutable std::mutex chainMutex;
    int difficulty;

public:
    Blockchain(int difficulty = 4) : difficulty(difficulty)
    {
        // Create genesis block
        Block genesisBlock(0, "0", difficulty);
        genesisBlock.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        genesisBlock.merkleRoot = "0";
        genesisBlock.mineBlock();

        chain.push_back(genesisBlock);
    }

    Block &getLatestBlock()
    {
        std::lock_guard<std::mutex> lock(chainMutex);
        return chain.back();
    }

    void addBlock(Block block)
    {
        std::lock_guard<std::mutex> lock(chainMutex);
        block.previousHash = chain.back().hash;
        block.index = chain.size();
        block.mineBlock();
        chain.push_back(block);
    }

    bool isChainValid() const
    {
        std::lock_guard<std::mutex> lock(chainMutex);

        for (size_t i = 1; i < chain.size(); i++)
        {
            const Block &currentBlock = chain[i];
            const Block &previousBlock = chain[i - 1];

            if (currentBlock.hash != currentBlock.calculateHash())
            {
                return false;
            }

            if (currentBlock.previousHash != previousBlock.hash)
            {
                return false;
            }

            if (!currentBlock.isValid())
            {
                return false;
            }
        }

        return true;
    }

    std::vector<Block> getChain() const
    {
        std::lock_guard<std::mutex> lock(chainMutex);
        return chain;
    }

    // Replace our chain with the longest valid chain
    bool replaceChain(const std::vector<Block> &newChain)
    {
        std::lock_guard<std::mutex> lock(chainMutex);

        if (newChain.size() <= chain.size())
        {
            return false;
        }

        // Verify the new chain
        for (size_t i = 1; i < newChain.size(); i++)
        {
            const Block &currentBlock = newChain[i];
            const Block &previousBlock = newChain[i - 1];

            if (currentBlock.hash != currentBlock.calculateHash())
            {
                return false;
            }

            if (currentBlock.previousHash != previousBlock.hash)
            {
                return false;
            }

            if (!currentBlock.isValid())
            {
                return false;
            }
        }

        chain = newChain;
        return true;
    }
};

// Generate a random chess move
std::string generateRandomChessMove()
{
    const std::vector<std::string> pieces = {"", "N", "B", "R", "Q", "K"};
    const std::vector<char> files = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    const std::vector<char> ranks = {'1', '2', '3', '4', '5', '6', '7', '8'};

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<> piecesDist(0, pieces.size() - 1);
    std::uniform_int_distribution<> filesDist(0, files.size() - 1);
    std::uniform_int_distribution<> ranksDist(0, ranks.size() - 1);
    std::uniform_int_distribution<> captureDist(0, 5);

    std::string move = pieces[piecesDist(gen)];
    move += files[filesDist(gen)];
    move += ranks[ranksDist(gen)];

    // Sometimes add captures
    if (captureDist(gen) == 0)
    {
        move += "x";
        move += files[filesDist(gen)];
        move += ranks[ranksDist(gen)];
    }

    return move;
}

// Player class
class Player
{
private:
    std::string id;
    KeyPair keyPair;
    Blockchain blockchain;
    std::vector<Transaction> pendingTransactions;
    std::mutex txMutex;
    bool isPlaying;

public:
    Player(const std::string &id) : id(id), isPlaying(false)
    {
        std::cout << "Player " << id << " initialized with public key: "
                  << keyPair.getPublicKeyPEM().substr(0, 40) << "..." << std::endl;
    }

    std::string getId() const
    {
        return id;
    }

    std::string getPublicKey() const
    {
        return keyPair.getPublicKeyPEM();
    }

    Transaction createMove(const std::string &toPlayerKey)
    {
        std::string move = generateRandomChessMove();
        Transaction tx(getPublicKey(), toPlayerKey, move);
        tx.sign(keyPair);

        std::lock_guard<std::mutex> lock(txMutex);
        pendingTransactions.push_back(tx);

        std::cout << "Player " << id << " created move: " << move << std::endl;
        return tx;
    }

    bool receiveMove(const Transaction &tx)
    {
        if (!tx.verifySignature(tx.from))
        {
            std::cout << "Player " << id << " rejected invalid transaction signature!" << std::endl;
            return false;
        }

        std::lock_guard<std::mutex> lock(txMutex);
        pendingTransactions.push_back(tx);
        std::cout << "Player " << id << " received move: " << tx.moveData << std::endl;

        // Mine a block when we have 5 transactions
        if (pendingTransactions.size() >= 5)
        {
            mineBlock();
        }

        return true;
    }

    void mineBlock()
    {
        std::lock_guard<std::mutex> lock(txMutex);
        if (pendingTransactions.empty())
        {
            return;
        }

        Block newBlock(blockchain.getLatestBlock().index + 1, blockchain.getLatestBlock().hash);

        // Add up to 5 transactions to the block
        int count = 0;
        auto it = pendingTransactions.begin();
        while (it != pendingTransactions.end() && count < 5)
        {
            newBlock.addTransaction(*it);
            it = pendingTransactions.erase(it);
            count++;
        }

        std::cout << "Player " << id << " mining a block with " << count << " moves..." << std::endl;
        blockchain.addBlock(newBlock);
    }

    void play(Player *opponent, int numMoves)
    {
        isPlaying = true;
        int movesMade = 0;

        while (isPlaying && movesMade < numMoves)
        {
            // Create and send a move
            Transaction tx = createMove(opponent->getPublicKey());
            opponent->receiveMove(tx);

            // Wait a bit before the next move
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            movesMade++;
        }

        // Make sure all remaining moves are mined
        if (!pendingTransactions.empty())
        {
            mineBlock();
        }

        isPlaying = false;
        std::cout << "Player " << id << " finished playing." << std::endl;
    }

    void stopPlaying()
    {
        isPlaying = false;
    }

    const std::vector<Block> getGameBlocks() const
    {
        return blockchain.getChain();
    }
};

// Main Node class representing a full node in the blockchain network
class MainNode : public Blockchain
{
private:
    std::string nodeId;
    std::vector<std::shared_ptr<MainNode>> peers;
    std::mutex peersMutex;
    std::queue<std::vector<Block>> pendingGames;
    std::mutex pendingGamesMutex;

public:
    MainNode(const std::string &id, int difficulty = 4)
        : Blockchain(difficulty), nodeId(id)
    {
        std::cout << "Main Node " << nodeId << " initialized" << std::endl;
    }

    std::string getId() const
    {
        return nodeId;
    }

    void addPeer(std::shared_ptr<MainNode> peer)
    {
        std::lock_guard<std::mutex> lock(peersMutex);
        peers.push_back(peer);
    }

    void receiveGame(const std::vector<Block> &gameBlocks)
    {
        std::lock_guard<std::mutex> lock(pendingGamesMutex);
        pendingGames.push(gameBlocks);
        std::cout << "Main Node " << nodeId << " received a game with "
                  << gameBlocks.size() << " blocks" << std::endl;
    }

    void processGames()
    {
        while (true)
        {
            std::vector<Block> game;

            {
                std::lock_guard<std::mutex> lock(pendingGamesMutex);
                if (pendingGames.empty())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }

                game = pendingGames.front();
                pendingGames.pop();
            }

            // Validate the game
            bool valid = true;
            for (size_t i = 1; i < game.size(); i++)
            {
                const Block &currentBlock = game[i];
                const Block &previousBlock = game[i - 1];

                if (currentBlock.previousHash != previousBlock.hash)
                {
                    valid = false;
                    break;
                }

                if (!currentBlock.isValid())
                {
                    valid = false;
                    break;
                }

                // Verify all transactions in the block
                for (const auto &tx : currentBlock.transactions)
                {
                    if (!tx.verifySignature(tx.from))
                    {
                        valid = false;
                        break;
                    }
                }

                if (!valid)
                    break;
            }

            if (!valid)
            {
                std::cout << "Main Node " << nodeId << " rejected invalid game" << std::endl;
                continue;
            }

            // Create a game block (a block containing the game's final state)
            Block gameBlock(getLatestBlock().index + 1, getLatestBlock().hash);
            gameBlock.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

            // Mine the game block
            std::cout << "Main Node " << nodeId << " mining game block..." << std::endl;
            gameBlock.mineBlock();

            // Add the block to our chain
            {
                std::lock_guard<std::mutex> lock(chainMutex);
                chain.push_back(gameBlock);
            }

            // Broadcast to peers
            broadcastGameBlock(gameBlock);
        }
    }

    void broadcastGameBlock(const Block &gameBlock)
    {
        std::lock_guard<std::mutex> lock(peersMutex);
        std::cout << "Main Node " << nodeId << " broadcasting game block to "
                  << peers.size() << " peers" << std::endl;

        for (auto &peer : peers)
        {
            peer->receiveBlock(gameBlock);
        }
    }

    void receiveBlock(const Block &block)
    {
        std::cout << "Main Node " << nodeId << " received block: " << block.hash.substr(0, 10) << std::endl;

        // Verify the block
        if (!block.isValid())
        {
            std::cout << "Main Node " << nodeId << " rejected invalid block" << std::endl;
            return;
        }

        // Add the block to our chain if it's valid
        std::lock_guard<std::mutex> lock(chainMutex);
        if (block.index == chain.size() && block.previousHash == chain.back().hash)
        {
            chain.push_back(block);
            std::cout << "Main Node " << nodeId << " added block to chain" << std::endl;
        }
        else
        {
            // Potential fork, initiate synchronization
            std::cout << "Main Node " << nodeId << " detected potential fork, syncing..." << std::endl;
            // In a real implementation, we would request the full chain from peers
        }
    }

    void syncWithPeers()
    {
        // In a real implementation, we would periodically sync with peers
        // by requesting their chains and replacing ours if a longer valid chain exists
        std::lock_guard<std::mutex> peersLock(peersMutex);

        if (peers.empty())
        {
            return;
        }

        for (auto &peer : peers)
        {
            const std::vector<Block> peerChain = peer->getChain();

            if (peerChain.size() > chain.size())
            {
                bool valid = true;

                for (size_t i = 1; i < peerChain.size(); i++)
                {
                    const Block &currentBlock = peerChain[i];
                    const Block &previousBlock = peerChain[i - 1];

                    if (currentBlock.previousHash != previousBlock.hash || !currentBlock.isValid())
                    {
                        valid = false;
                        break;
                    }
                }

                if (valid)
                {
                    std::lock_guard<std::mutex> chainLock(chainMutex);
                    chain = peerChain;
                    std::cout << "Main Node " << nodeId << " synced with peer "
                              << peer->getId() << ", new chain length: " << chain.size() << std::endl;
                }
            }
        }
    }
};

int main()
{
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    // Create players
    std::shared_ptr<Player> player1 = std::make_shared<Player>("Player1");
    std::shared_ptr<Player> player2 = std::make_shared<Player>("Player2");

    // Create main nodes
    std::shared_ptr<MainNode> node1 = std::make_shared<MainNode>("Node1");
    std::shared_ptr<MainNode> node2 = std::make_shared<MainNode>("Node2");
    std::shared_ptr<MainNode> node3 = std::make_shared<MainNode>("Node3");

    // Connect nodes as peers
    node1->addPeer(node2);
    node1->addPeer(node3);
    node2->addPeer(node1);
    node2->addPeer(node3);
    node3->addPeer(node1);
    node3->addPeer(node2);

    // Start node processing threads
    std::thread node1Thread(&MainNode::processGames, node1);
    std::thread node2Thread(&MainNode::processGames, node2);
    std::thread node3Thread(&MainNode::processGames, node3);

    // Start the game with 20 moves
    std::thread player1Thread(&Player::play, player1.get(), player2.get(), 10); // Player 1 makes 10 moves
    std::thread player2Thread(&Player::play, player2.get(), player1.get(), 10); // Player 2 makes 10 moves

    // Wait for the game to finish
    player1Thread.join();
    player2Thread.join();

    std::cout << "Game completed, submitting to main nodes..." << std::endl;

    // Submit the game to the main nodes
    node1->receiveGame(player1->getGameBlocks());
    node2->receiveGame(player2->getGameBlocks());

    // Let the nodes process for a while
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Clean up (in a real application, we would have a proper shutdown sequence)
    node1Thread.detach();
    node2Thread.detach();
    node3Thread.detach();

    // Clean up OpenSSL
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();

    std::cout << "Blockchain chess game demonstration completed" << std::endl;
    return 0;
}