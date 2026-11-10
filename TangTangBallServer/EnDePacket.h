#pragma once
#include <vector>

using BYTE = unsigned char;

// 전체 길이(2바이트), 종류(1바이트), 본문 순서로 저장
// 길이는 큰 자리 바이트부터 기록
// ExtractTcpPackets도 같은 순서를 사용해야 함
std::vector<BYTE> EncodePacket(BYTE packetType, const std::vector<BYTE>& body);

// 완성된 패킷 하나의 길이, 종류, 본문 형식을 검사
// 실패하면 false를 반환하며 출력 인수는 변경 X 
bool DecodePacket(const std::vector<BYTE>& bytes, BYTE& packetType,
    std::vector<BYTE>& body);