#ifndef _BASE64_ENCODE_H_
#define _BASE64_ENCODE_H_
////
////#if defined(__cplusplus)
////extern "C"
////{
////#endif
#include <ctype.h>
#pragma pack(push,1)
static
const char * base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";
static 
char find_index(unsigned char ch_64) {
    if ( (unsigned char)((unsigned char)ch_64 - 'A') < 26 ) {
        return ch_64 - 'A';
    }
    if ( (unsigned char)((unsigned char)ch_64 - 'a') < 26 ) {
        return 26 + ch_64 - 'a';
    }
    if ( (unsigned char)((unsigned char)ch_64 - '0') < 10 ) {
        return 52 + ch_64 - '0';
    }
    if ( ch_64 == '+' ) return 62;
    if ( ch_64 == '/' ) return 63;
    return -1;
}

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static int base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len, char *base64_buf) {
  int retval = 0;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++) {
          base64_buf[retval++] = base64_chars[char_array_4[i]];
      }
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
        base64_buf[retval++] = base64_chars[char_array_4[j]];

    while((i++ < 3)) 
        base64_buf[retval++] = '=';
  }

  base64_buf[retval] = '\0';
  return retval;
}

static
int base64_decode(const char *encoded_string, int in_len, char *raw_data) {
    int retval = 0;
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = find_index(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
          raw_data[retval++] = char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = find_index(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) raw_data[retval++] = char_array_3[j];
  }

  return retval;
}
////#if defined(__cplusplus)
////}
////
////#endif
#pragma pack(pop)
#endif
