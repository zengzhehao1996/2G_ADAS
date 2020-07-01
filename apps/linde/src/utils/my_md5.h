#ifndef md5_INCLUDED
#define md5_INCLUDED

typedef unsigned char md5_byte_t; /* 8-bit byte */
typedef unsigned int md5_word_t;  /* 32-bit word */

#define MD5_CHECK_SIZE 16
#define MD5_SIZE MD5_CHECK_SIZE

/* Define the state of the MD5 Algorithm. */
typedef struct md5_state_s {
	md5_word_t count[2]; /* message length in bits, lsw first */
	md5_word_t abcd[4];  /* digest buffer */
	md5_byte_t buf[64];  /* accumulate block */
} md5_state_t;

#ifdef __cplusplus
extern "C" {
#endif

void md5Init(md5_state_t* pms);

void md5Append(md5_state_t* pms, const md5_byte_t* data, int nbytes);

void md5Finish(md5_state_t* pms, md5_byte_t digest[16]);

int  md5File(const char *path, md5_byte_t digest[16]);

int  md5CheckBlock(unsigned char *ptr,  unsigned int len,md5_byte_t digest[16]);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* md5_INCLUDED */
