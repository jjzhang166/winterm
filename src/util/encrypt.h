#ifndef	_ENCRYPT_H
#define	_ENCRYPT_H

#ifdef __cplusplus
extern "C" {
#endif

int encrypt_3des(char *key1, char *key2, char *key3, unsigned char *in,
		unsigned char *out);

int decrypt_3des(char *key1, char *key2, char *key3, unsigned char *in,
		unsigned char *out);

int md5_string(unsigned char *data, unsigned long len, char *strMd5);

#ifdef __cplusplus
}
#endif

#endif	//_ENCRYPT_H
