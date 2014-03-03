#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include "stream.h"

constexpr int GenerateThreadCount = 5;

void runServer(StreamServer & server);

#endif // SERVER_H_INCLUDED
