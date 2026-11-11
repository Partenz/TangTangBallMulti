#pragma once

#include <WinSock2.h>
#include <vector>

bool ApplySocketTimeouts(SOCKET socket, DWORD recvTimeoutMs, DWORD sendTimeoutMs);

std::vector<std::vector<BYTE>> ExtractTcpPackets(std::vector<BYTE>& buffer);