#ifdef WINDOWS

#include "selftest_local.h"

void Test_Flash_Search() {
	/*int tr;
	int check;
	int testAdr = 26293;
	byte testData[] = { 0xFF, 0xFE, 0x01, 0x02, 0x03, 0x04, 0x05, 0xFC, 0xFB, 0xFA, 0xBA, 0xAD, 0xF0, 0x0D, 0xAB, 0xCD };
	int testDataLen = sizeof(testData);
	SIM_ClearOBK(0);
	SIM_ClearAndPrepareForMQTTTesting("myTestDevice", "bekens");

	flash_write(testData, testDataLen, testAdr);


	SELFTEST_ASSERT(Flash_FindPattern(testData, testDataLen, 0x0, 0x200000) == testAdr);

	for (tr = 0; tr < 200; tr++) {
		// random ofs
		testAdr = 0x1 + rand() % 0x200000 - testDataLen;
		printf("Loop %i, adr %i\n", tr, testAdr);
		// random content with attempt index encoded inside so it's unique
		// (so we won't get it by pure chance somewhere else in the test flash)
		for (int j = 0; j < 4; j++) {
			testData[2 + j] = tr;
		}
		// write
		flash_write(testData, testDataLen, testAdr);
		// checksearch
		int at = Flash_FindPattern(testData, testDataLen, 0x0, 0x200000);
		printf("Found at %i\n", at);
		SELFTEST_ASSERT(at == testAdr);
	}*/
}

#endif
