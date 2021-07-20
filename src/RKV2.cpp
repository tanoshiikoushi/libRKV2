#include <cstring>
#include "rw_utilities/mem_read_utilities.h"
#include "RKV2.h"

bool RKV2File::load(const u8* buf_to_copy, const u64 buf_size) {
    // copy into our internal buffer
    this.data = new u8[buf_size];
    strncpy(this.data, buf_to_copy, buf_size);

    // header check
    if (read_32BE(this.data) != 0x524B5632) { // "RKV2"
        delete this.data;
        return false;
    }

    // read in the following useful info
    this.entry_count = read_32LE(&this.data[0x04]);
    this.entry_name_string_length = read_32LE(&this.data[0x08]);
    this.filepath_addendum_count = read_32LE(&this.data[0x0C]);
    this.filepath_addendum_string_length = read_32LE(&this.data[0x10]);
    this.metadata_table_offset = read_32LE(&this.data[0x14]);
    this.metadata_table_length = read_32LE(&this.data[0x18]);

    // allocate buffers for our entries and addendums
    this.entries = new RKV2Entry[this.entry_count];
    this.addendums = new RKV2FilePathAddendum[this.filepath_addendum_count];

    // now process these entries and addendums using our string_buf_readers :D

    return true;
}

bool RKV2File::extract(const u8* output_path) {

}
