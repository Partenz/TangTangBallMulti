#pragma once
#include "EventQueue.h"

bool QueueTcpPacket(BYTE packetType, const std::vector<BYTE>& body);
