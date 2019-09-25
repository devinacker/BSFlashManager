#include "mempackitem.h"

#include <cstring>
#include <qtextcodec.h>

// ----------------------------------------------------------------------------
bool MemPackItem::tryLoadFrom(QFile& file, unsigned offset)
{
	file.seek(offset);
	// try to read header
	if (file.read((char*)&header, sizeof header) < sizeof header) return false;

	// try to detect a valid file...
	// TODO: make this able to reject normal SNES ROMs...
	bool titleValid = false;
	for (int i = 0; i < sizeof header.title; i++)
	{
		if (header.title[i] != (char)0xff)
		{
			titleValid = true;
			break;
		}
	}
	if (!titleValid) return false;

	if (header.blocks != 0
	    && (((header.checksum ^ header.checksumComp) == 0xffff && header.makerFixed == 0x33)
			|| ((header.checksum | header.checksumComp) == 0x0000)
			|| header.blocks == 0xFFFFFFFF))
	{
		data.clear();

		// read blocks based on the mapping bits
		quint32 blocksLeft = normalizeBlocks(header.blocks, file.size());
		for (int i = 0; i < 32 && blocksLeft; i++)
		{
			if (blocksLeft & (1 << i))
			{
				blocksLeft &= ~(1 << i);

				// TODO: possibly handle non-po2 file sizes here...
				file.seek(i << 17);
				data += file.read(1 << 17);
			}
		}

		char tempTitle[17];
		memcpy(tempTitle, header.title, 16);
		tempTitle[16] = '\0';

		title = QTextCodec::codecForName("Shift-JIS")->toUnicode(tempTitle);
		blocks = data.size() >> 17;
		if (header.starts & (1 << 15))
		{
			starts = countBits(header.starts & 0x7fff);
		}
		else
		{
			starts = -1;
		}

		deleted = header.makerFixed != 0x33;
		return true;
	}

	return false;
}

// ----------------------------------------------------------------------------
bool MemPackItem::saveToFile(QFile& file, unsigned offset)
{
	QByteArray encodedTitle = QTextCodec::codecForName("Shift-JIS")->fromUnicode(title);
	memset(header.title, 0, 16);
	strncpy(header.title, encodedTitle.constData(), 16);

	const unsigned blockStart = offset >> 17;
	unsigned newBlocks = 0;
	for (unsigned i = blockStart; i < blockStart + (data.size() >> 17); i++)
	{
		newBlocks |= (1 << i);
	}
	header.blocks = newBlocks;

	unsigned headerPos = 0x7fb0;
	if (header.mapMode & 1)
	{
		headerPos |= 1 << 15;
	}
	memset(data.data() + headerPos, 0, sizeof header);

	if (deleted)
	{
		header.makerFixed = 0;
	}
	else
	{
		header.makerFixed = 0x33;

		quint16 checksum = 0;
		for (int i = 0; i < data.size(); i++)
		{
			checksum += (uint8)data[i];
		}

		header.checksum = checksum;
		header.checksumComp = ~checksum;
	}

	memcpy(data.data() + headerPos, &header, sizeof header);

	file.seek(offset);
	return file.write(data) == data.size();
}

// ----------------------------------------------------------------------------
unsigned MemPackItem::countBits(unsigned val)
{
	unsigned bits = 0;
	while (val)
	{
		bits += (val & 1);
		val >>= 1;
	}
	return bits;
}

// ----------------------------------------------------------------------------
unsigned MemPackItem::normalizeBlocks(quint32 blocks, unsigned packSize)
{
	unsigned totalBlocks = packSize >> 17;
	quint32 mask = 0xffffffff;
	uint shift = 16;
	while (totalBlocks <= shift)
	{
		blocks |= (blocks >> shift);
		mask >>= shift;
		shift >>= 1;
	}

	return blocks & mask;
}
