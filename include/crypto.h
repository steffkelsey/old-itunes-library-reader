#ifndef CRYPTO_H
#define CRYPTO_H

#define AES_KEY_SIZE 32 // for AES-256
#define AES_BLOCK_SIZE 16

int aes_ecb_nopad_decrypt(unsigned char *ciphertext, int ciphertext_len, 
                          unsigned char *key, unsigned char *plaintext);

#endif
