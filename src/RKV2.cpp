#include "checksum/crc32eth.h"
#include "RKV2.h"

u32 test_func() {
	return crc32eth(data, sizeof(data));
}
