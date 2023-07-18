#pragma once
#include <memory>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

struct EVP_PKEY_CTX_deleter {
    void operator()(EVP_PKEY_CTX *ptr) const {
        EVP_PKEY_CTX_free(ptr);
    }
};

struct EVP_PKEY_deleter {
    void operator()(EVP_PKEY *ptr) const {
        EVP_PKEY_free(ptr);
    }
};

struct EVP_MD_CTX_deleter {
    void operator()(EVP_MD_CTX *ptr) const {
        EVP_MD_CTX_free(ptr);
    }
};

using EVP_PKEY_CTX_ptr = std::unique_ptr<EVP_PKEY_CTX, EVP_PKEY_CTX_deleter>;
using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, EVP_PKEY_deleter>;
using EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, EVP_MD_CTX_deleter>;
using BIO_ptr = std::unique_ptr<BIO, decltype(&BIO_free)>;
using BUF_MEM_ptr = std::unique_ptr<BUF_MEM, decltype(&BUF_MEM_free)>;
