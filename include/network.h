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

class NetworkConnection final : public StreamRW
{
    friend class NetworkServer;
private:
    shared_ptr<Reader> readerInternal;
    shared_ptr<Writer> writerInternal;
    NetworkConnection(int readFd, int writeFd)
        : readerInternal(new FileReader(fdopen(readFd, "r"))), writerInternal(new FileWriter(fdopen(writeFd, "w")))
    {
    }
public:
    explicit NetworkConnection(wstring url, uint16_t port);
    shared_ptr<Reader> preader() override
    {
        return readerInternal;
    }
    shared_ptr<Writer> pwriter() override
    {
        return writerInternal;
    }
};

class NetworkServer final : public StreamServer
{
    NetworkServer(const NetworkServer &) = delete;
    const NetworkServer & operator =(const NetworkServer &) = delete;
private:
    int fd;
public:
    explicit NetworkServer(uint16_t port);
    ~NetworkServer();
    shared_ptr<StreamRW> accept() override;
};

#endif // NETWORK_H_INCLUDED
