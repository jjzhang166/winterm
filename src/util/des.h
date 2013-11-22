#ifndef _DES_H_
#define _DES_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DES_ENCRYPT = 0, DES_DECRYPT = 1
} DES_MODE;

void des(const unsigned char * p_data, const unsigned char * p_key,
		unsigned char * p_output, DES_MODE mode);

#ifdef __cplusplus
}
#endif

#endif //#ifndef _DES_H_
