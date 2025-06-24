#include "BlockGame.hpp"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

BlockGame::BlockGame(int idx, std::string prevHash, std::vector<Move> moves)
{
    this->index = idx;
    this->previousHash = prevHash;
    this->moves = moves;
    this->timestamp = time(nullptr);
    this->nonce = 0;
    this->hash = calculateHash();
    this->difficulty = 4; // Default difficulty
}

std::string BlockGame::calculateHash()
{
    std::stringstream ss;
    ss << index << previousHash << timestamp << nonce;
    for (const auto &tx : moves)
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

void BlockGame::mineBlock(int difficulty)
{
    std::string target(difficulty, '0');
    do
    {
        nonce++;
        hash = calculateHash();
    } while (hash.substr(0, difficulty) != target);
}
