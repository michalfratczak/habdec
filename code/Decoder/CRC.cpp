#include "CRC.h"

#include <string>

namespace
{

char _hex(char Character)
{
	char _hexTable[] = "0123456789ABCDEF";
	return _hexTable[int(Character)];
}

}


namespace habdec
{


std::string CRC(std::string i_str)
{
	using std::string;

	unsigned int CRC = 0xffff;
	// unsigned int xPolynomial = 0x1021;

	for (size_t i = 0; i < i_str.length(); i++)
	{
		CRC ^= (((unsigned int)i_str[i]) << 8);
		for (int j=0; j<8; j++)
		{
			if (CRC & 0x8000)
			    CRC = (CRC << 1) ^ 0x1021;
			else
			    CRC <<= 1;
		}
	}

	string result;
	result += _hex((CRC >> 12) & 15);
	result += _hex((CRC >> 8) & 15);
	result += _hex((CRC >> 4) & 15);
	result += _hex(CRC & 15);

	return result;
}

} // namespace habdec
