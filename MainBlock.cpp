#include "MainBlock.hpp"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

MainBlock::MainBlock(int idx, std::string prevHash, std::vector<Game> games)
{
    this->index = idx;
    this->previousHash = prevHash;
    this->games = games;
    this->timestamp = time(nullptr);
    this->nonce = 0;
    this->hash = calculateHash();
    this->difficulty = 3; // Default difficulty
}

std::string MainBlock::calculateHash()
{
    std::stringstream ss;
    ss << index << previousHash << timestamp << nonce;
    for (const auto &tx : games)
    {
        ss << tx.toString();
    }
    std::string input = ss.str();

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.c_str()), input.size(), hash);

    std::stringstream hashString;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        hashString << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return hashString.str();
}

void MainBlock::mineBlock(int difficulty)
{
    std::string target(difficulty, '0');
    do
    {
        nonce++;
        hash = calculateHash();
    } while (hash.substr(0, difficulty) != target);
}
