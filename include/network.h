#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include "stream.h"
#include <memory>

class NetworkException : public IOException
{
public:
    explicit NetworkException(string msg)
        : IOException(msg)
    {
    }
};

class NetworkConnection final
{
    friend class NetworkServer;
private:
    unique_ptr<Reader> readerInternal;
    unique_ptr<Writer> writerInternal;
    NetworkConnection(int readFd, int writeFd)
        : readerInternal(new FileReader(fdopen(readFd, "r"))), writerInternal(new FileWriter(fdopen(writeFd, "w")))
    {
    }
public:
    explicit NetworkConnection(wstring url, uint16_t port);
    Reader & reader()
    {
        return *readerInternal;
    }
    Writer & writer()
    {
        return *writerInternal;
    }
};

class NetworkServer final
{
    NetworkServer(const NetworkServer &) = delete;
    const NetworkServer & operator =(const NetworkServer &) = delete;
private:
    int fd;
public:
    explicit NetworkServer(uint16_t port);
    ~NetworkServer();
    shared_ptr<NetworkConnection> accept();
};

#endif // NETWORK_H_INCLUDED
