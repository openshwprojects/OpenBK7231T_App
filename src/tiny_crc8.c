

char Tiny_CRC8(const char *data,int length) 
{
	char crc = 0x00;
	char extract;
	char sum;
	int i;
	char tempI;

	for(i=0;i<length;i++)
	{
		extract = *data;
		for (tempI = 8; tempI; tempI--) 
		{
			sum = (crc ^ extract) & 0x01;
			crc >>= 1;
			if (sum)
				crc ^= 0x8C;
			extract >>= 1;
		}
		data++;
	}
	return crc;
}
