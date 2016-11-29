#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#endif
#include "../../L0/abstract/type.h"
#include <string.h>
#include <stdlib.h>
#ifndef DWORD 
#define DWORD type_l0::dword_t 
#define WORD type_l0::word_t
#endif
namespace network_l3 {
    inline bool is_ipaddress(const char *ip_address)
    {

        static const int pMarray[] = {0, 0, 10, 100};

        for ( int i = 0; i < 4; ++i ) {

            int iByte = 0;

            int j = 0;

            for ( j = 0; j < 4; ++j, ++ip_address ) {

                if ( *ip_address == '\0' ) break;

                if ( *ip_address == '.' ) {

                    if ( i == 3 ) return false;

                    ++ip_address; break;

                }

                char chCurrentChar = *ip_address - '0';

                if ( (unsigned char)chCurrentChar > 9 ) return false;

                if ( ( iByte = iByte * 10 + chCurrentChar ) > 255 ) return false;

            }

            if ( (j == 0) || (iByte < pMarray[j]) ) return false;

            if ( *ip_address == '\0' ) return i == 3;

        }

        return false;

    }

#ifndef _SERVICE_ADDR_DEF
#define _SERVICE_ADDR_DEF
    struct _SERVICE_ADDR {
        _SERVICE_ADDR():ip(0), port(0){}
        _SERVICE_ADDR(DWORD _ip, WORD _port):ip(_ip), port(_port){}
        _SERVICE_ADDR(const char *addr, WORD default_port):ip(0), port(0) {
            init(addr, default_port);
        }
        DWORD ip;
        WORD port;
        bool operator == (const _SERVICE_ADDR &_right) const {
            return (ip == _right.ip) && (port == _right.port);
        }
        bool operator != (const _SERVICE_ADDR &_right) const {
            return (ip != _right.ip) || (port != _right.port);
        }
        bool operator < (const _SERVICE_ADDR &_right) const {
            if ( ip == _right.ip ) return port < _right.port;
            return ip < _right.ip;
        }
        bool init(const char *_str, WORD _default_port) {

            char buffer[128];
            strncpy(buffer, _str, sizeof(buffer) - 1); buffer[sizeof(buffer) - 1] = '\0';
            char *token = strchr(buffer, ':');
            if ( token ) {
                *token = '\0'; port = (WORD)atol(token + 1);
            }
            else port = _default_port;
            if ( ! is_ipaddress(buffer) ) return false;
            ip = ntohl(inet_addr(buffer));
            return true;
        }
        bool valid() const { return ip || port ;}
        char *to_string(char *_buffer) const {
            in_addr addr; addr.s_addr = htonl(ip);
            sprintf(_buffer, "%s:%d", inet_ntoa(addr), port);
            return _buffer;
        }
    };
#endif

    class AutoNetwork {
    public:
        AutoNetwork() {
#ifdef _WIN32
            WSAData wsaData = {0};
            WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
        }
        ~AutoNetwork() {
#ifdef _WIN32
            WSACleanup();
#endif
        }
    };
#ifdef _WIN32
    static void sockblock(SOCKET s, bool isBlock)
    {

        unsigned long lArgp = ! isBlock;

        ioctlsocket(s, FIONBIO, &lArgp);

    }
#define socklen_t int
#else
    static void sockblock(SOCKET s, bool isBlock)
    {

        int flags = fcntl (s, F_GETFL);

        if (flags & O_NONBLOCK) {

            if ( isBlock ) fcntl (s, F_SETFL, flags - O_NONBLOCK);
        }
        else {

            if ( ! isBlock ) fcntl ( s, F_SETFL, flags | O_NONBLOCK);
        }

    }

#endif

    inline void set_reuse(SOCKET sConn, bool reuse = true) {
        DWORD bReUse = reuse ? 1: 0;
        setsockopt(sConn, SOL_SOCKET, SO_REUSEADDR, (char*)&bReUse, sizeof(bReUse));

    }
    inline SOCKET async_connect(DWORD remote_ip, WORD remote_port, DWORD local_ip, WORD local_port) {

        SOCKET sConn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if ( sConn == INVALID_SOCKET ) return INVALID_SOCKET;

        set_reuse(sConn, true);
        sockblock(sConn, false);

        if ( local_port || local_ip ) {

            SOCKADDR_IN localAddr = {0};
            localAddr.sin_family = AF_INET;
            localAddr.sin_port = htons(local_port);
            localAddr.sin_addr.s_addr = htonl(local_ip);

            bind(sConn, (sockaddr*)&localAddr, sizeof(localAddr));

        }

        SOCKADDR_IN remoteAddr = {0};
        remoteAddr.sin_addr.s_addr = htonl(remote_ip);
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port = htons(remote_port);

        if ( connect(sConn, (sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR ) {

#ifdef  WIN32

            if ( GetLastError() == 10035 ) return sConn;

#else
            if ( (EALREADY == errno) || (EINPROGRESS == errno) ) return sConn;
            printf("\r\t\t\t\t\tconnect error %d %s", errno, strerror(errno));
#endif

            closesocket(sConn); return INVALID_SOCKET;
        }

        printf("\nconnectOK");

        return sConn;

    }

}