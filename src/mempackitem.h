#pragma once

#include <qbytearray.h>
#include <qfile.h>
#include "endian.h"

#pragma pack(push, 1)
struct ItemHeader
{
	char     maker[2];     // 0xFB0
	uint32le programType;  // 0xFB2
	char     dummy[10];    // 0xFB6
	char     title[16];    // 0xFC0
	uint32le blocks;       // 0xFD0
	uint16le starts;       // 0xFD4
	uint8    month;        // 0xFD6
	uint8    day;          // 0xFD7
	uint8    mapMode;      // 0xFD8
	uint8    execFlags;    // 0xFD9
	uint8    makerFixed;   // 0xFDA
	uint8    version;      // 0xFDB
	uint16le checksumComp; // 0xFDC
	uint16le checksum;     // 0xFDE
};
#pragma pack(pop)

struct MemPackItem
{
	ItemHeader header;

	QString title;
	unsigned blocks = 0;
	int starts = -1;
	bool deleted = false;
	
	QByteArray data;

	bool tryLoadFrom(QFile& file, unsigned offset);
	bool saveToFile(QFile& file, unsigned offset = 0);

	static unsigned countBits(unsigned val);
	static unsigned normalizeBlocks(quint32 blocks, unsigned packSize);
};

typedef QList<MemPackItem> MemPackItems;
