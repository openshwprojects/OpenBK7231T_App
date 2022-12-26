
#define DHT11 11
#define DHT12 12
#define DHT21 21
#define DHT22 22
#define AM2301 21

typedef struct dht_s { 
	byte data[5];
	byte _pin, _type;
	uint32_t _lastreadtime, _maxcycles;
	bool _lastresult;
	uint8_t pullTime; // Time (in usec) to pull up data line before reading

} dht_t;

dht_t *DHT_Create(byte pin, byte type);
float DHT_readHumidity(dht_t *dht, bool force);
float DHT_readTemperature(dht_t *dht, bool S, bool force);

