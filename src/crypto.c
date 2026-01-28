#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "crypto.h"

int aes_ecb_nopad_decrypt(unsigned char *ciphertext, int ciphertext_len, 
                          unsigned char *key, unsigned char *plaintext) {
  EVP_CIPHER_CTX *ctx;
  int len, plaintext_len;

  // Create and initialize the context 
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    printf("Error creating cipher ctx\n");
    return STATUS_ERROR;
  }

  // Initialize the decryption operation. Use EVP_aes_128_ecb() 
  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL)) {
    printf("Error initializing decrypt operation\n");
    EVP_CIPHER_CTX_free(ctx);
    return STATUS_ERROR;
  }

  // Disable padding
  EVP_CIPHER_CTX_set_padding(ctx, 0);

  // Provide the ciphertext bytes to be decrypted
  if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
    printf("Error decrypt update\n");
    EVP_CIPHER_CTX_free(ctx);
    return STATUS_ERROR;
  }
  plaintext_len = len;

  // Finalize the decryption operation. Should be 1 for success if nopad and correct length
  if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
    printf("Error decrypt final\n");
    // Handle error (e.g., "wrong final block length" if input size is wrong)
    EVP_CIPHER_CTX_free(ctx);
    return STATUS_ERROR;
  }
  plaintext_len += len;

  // Clean up
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}
