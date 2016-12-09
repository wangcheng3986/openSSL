//
// Created by Administrator on 2016/12/9.
//

#ifndef LOGIC_BASE64_ENCODE_H
#define LOGIC_BASE64_ENCODE_H

char * base64_encode( const unsigned char * bindata, char * base64, int binlength );
int base64_decode( const char * base64, unsigned char * bindata );

#endif //LOGIC_BASE64_ENCODE_H
