#include "Utils.h"

void Utils::array_to_string(byte array[], unsigned int len, char buffer[])
{
	for (size_t i = 0; i < len; i++)
	{
		byte nib1 = (array[i] >> 4) & 0x0F;
		byte nib2 = (array[i] >> 0) & 0x0F;
		buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
		buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
	}
	buffer[len * 2] = '\0';
}