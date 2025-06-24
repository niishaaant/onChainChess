#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "Move.hpp"

using namespace std;

Move::Move(std::string sender, std::string receiver, string data)
{
    this->sender = sender;
    this->receiver = receiver;
    this->data = data;
    this->id = 1000000000 + rand() % 9000000000;
}

void Move::signTransaction(const std::string &privateKey)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_PKEY *pkey = nullptr;
    BIO *bio = BIO_new_mem_buf(privateKey.c_str(), -1);

    if (!PEM_read_bio_PrivateKey(bio, &pkey, nullptr, nullptr))
    {
        std::cerr << "Error reading private key" << std::endl;
        BIO_free(bio);
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0)
    {
        std::cerr << "Error initializing digest sign" << std::endl;
        EVP_PKEY_free(pkey);
        BIO_free(bio);
        EVP_MD_CTX_free(ctx);
        return;
    }

    std::stringstream ss;
    ss << sender << receiver << data;
    std::string data = ss.str();

    if (EVP_DigestSignUpdate(ctx, data.c_str(), data.size()) <= 0)
    {
        std::cerr << "Error updating digest sign" << std::endl;
        EVP_PKEY_free(pkey);
        BIO_free(bio);
        EVP_MD_CTX_free(ctx);
        return;
    }

    size_t sigLen = 0;
    if (EVP_DigestSignFinal(ctx, nullptr, &sigLen) <= 0)
    {
        std::cerr << "Error finalizing digest sign (getting length)" << std::endl;
        EVP_PKEY_free(pkey);
        BIO_free(bio);
        EVP_MD_CTX_free(ctx);
        return;
    }

    unsigned char *sig = new unsigned char[sigLen];
    if (EVP_DigestSignFinal(ctx, sig, &sigLen) <= 0)
    {
        std::cerr << "Error finalizing digest sign" << std::endl;
        delete[] sig;
        EVP_PKEY_free(pkey);
        BIO_free(bio);
        EVP_MD_CTX_free(ctx);
        return;
    }

    signature = std::string(reinterpret_cast<char *>(sig), sigLen);

    delete[] sig;
    EVP_PKEY_free(pkey);
    BIO_free(bio);
    EVP_MD_CTX_free(ctx);
}

bool Move::isValid() const
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_PKEY *pkey = nullptr;
    BIO *bio = BIO_new_mem_buf(sender.c_str(), -1);

    if (!PEM_read_bio_PUBKEY(bio, &pkey, nullptr, nullptr))
    {
        std::cerr << "Error reading public key" << std::endl;
        BIO_free(bio);
        EVP_MD_CTX_free(ctx);
        return false;
    }

    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0)
    {
        std::cerr << "Error initializing digest verify" << std::endl;
        EVP_PKEY_free(pkey);
        BIO_free(bio);
        EVP_MD_CTX_free(ctx);
        return false;
    }

    std::stringstream ss;
    ss << sender << receiver << data;
    std::string data = ss.str();

    if (EVP_DigestVerifyUpdate(ctx, data.c_str(), data.size()) <= 0)
    {
        std::cerr << "Error updating digest verify" << std::endl;
        EVP_PKEY_free(pkey);
        BIO_free(bio);
        EVP_MD_CTX_free(ctx);
        return false;
    }

    bool result = EVP_DigestVerifyFinal(ctx, reinterpret_cast<const unsigned char *>(signature.c_str()), signature.size()) == 1;

    EVP_PKEY_free(pkey);
    BIO_free(bio);
    EVP_MD_CTX_free(ctx);

    return result;
}

std::string Move::toString() const
{
    std::ostringstream ss;
    ss << sender << receiver << data;

    for (unsigned char c : signature)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }

    return ss.str();
}
