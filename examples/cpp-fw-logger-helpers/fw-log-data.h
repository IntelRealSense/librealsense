#pragma once
#include <stdint.h>
#include <string>
#include <vector>


struct FWlogsBinaryData
{
	// TODO: Make const char []
    std::vector<uint8_t> logsBuffer;
};

#pragma pack(1)

typedef union
{
    uint32_t value;
	struct
	{
        uint32_t magicNumber : 8;
        uint32_t severity : 5;
        uint32_t threadId : 3;
        uint32_t fileId : 11;
        uint32_t groupId : 5;
	} bits;
} FWlogHeaderDWord1;

typedef union
{
    uint32_t value;
	struct
	{
        uint32_t eventId : 16;
        uint32_t lineId : 12;
        uint32_t seqId : 4;
	} bits;
} FWlogHeaderDWord2;

struct FWlogHeaderDWord3
{
    uint16_t P1;
    uint16_t P2;
};

struct FWlogHeaderDWord4
{
    uint32_t P3;
};

struct FWlogHeaderDWord5
{
    uint32_t timeStamp;
};

struct FwLogBinary
{
	FWlogHeaderDWord1 Dword1;
	FWlogHeaderDWord2 Dword2;
	FWlogHeaderDWord3 Dword3;
	FWlogHeaderDWord4 Dword4;
	FWlogHeaderDWord5 Dword5;
};

#pragma pack()

class FWlogData
{
public:
	FWlogData(void);
	~FWlogData(void);

    uint32_t magicNumber;
    uint32_t severity;
    uint32_t fileId;
    uint32_t groupId;
    uint32_t eventId;
    uint32_t line;
    uint32_t sequence;
    uint32_t p1;
    uint32_t p2;
    uint32_t p3;
    uint64_t timeStamp;
	double delta;

	std::string message;
	std::string fileName;
	std::string threadName;

	std::string ToString();
};
