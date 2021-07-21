#include <cstring>
#include <stdio.h>
#include "string_utilities/string_buf_readers.h"
#include "rw_utilities/mem_read_utilities.h"
#include "RKV2.h"

bool RKV2File::load(const u8* buf_to_copy, const u64 buf_size) {
    // copy into our internal buffer
    this->data = new u8[buf_size];
    memcpy(this->data, buf_to_copy, buf_size);
    printf("past copy\n");

    // header check
    if (read_32BE(this->data) != 0x524B5632) { // "RKV2"
        printf("header bad");
        delete this->data;
        return false;
    }
    printf("header good\n");

    // read in the following useful info
    this->entry_count = read_32LE(&this->data[0x04]);
    this->entry_name_string_length = read_32LE(&this->data[0x08]);
    this->filepath_addendum_count = read_32LE(&this->data[0x0C]);
    this->filepath_addendum_string_length = read_32LE(&this->data[0x10]);
    this->metadata_table_offset = read_32LE(&this->data[0x14]);
    this->metadata_table_length = read_32LE(&this->data[0x18]);
    printf("metadata read\n");

    // allocate buffers for our entries and addendums
    this->entries = new RKV2Entry[this->entry_count];
    this->addendums = new RKV2FilePathAddendum[this->filepath_addendum_count];
    printf("buffers alloced\n");

    // now read in the entries
    u64 curr_file_pos = this->metadata_table_offset;

    u64 name_string_pos = this->metadata_table_offset + (RKV2ENTRY_SIZE * this->entry_count);

    printf("entering entry cycle with curr_file_pos: 0x%.8X - name_string_pos: 0x%.8X\n", curr_file_pos, name_string_pos);
    for (u32 i = 0; i < this->entry_count; i++) {
        this->entries[i].entry_name_string_offset = read_32LE(&this->data[curr_file_pos]);
        // next 4 bytes are padding
        this->entries[i].entry_size = read_32LE(&this->data[curr_file_pos + 0x8]);
        this->entries[i].entry_offset = read_32LE(&this->data[curr_file_pos + 0xC]);
        this->entries[i].entry_crc32eth = read_32LE(&this->data[curr_file_pos + 0x10]);

        curr_file_pos += RKV2ENTRY_SIZE;

        u8* name = read_dynamic_string(&this->data[name_string_pos + this->entries[i].entry_name_string_offset], BASE_ENTRY_STRING_SIZE);
        printf("Name: %s - String Offset: 0x%.4X - Entry Offset: 0x%.4X\n", name, this->entries[i].entry_name_string_offset, this->entries[i].entry_offset);
        delete name;
    }
    printf("entries complete\n");

    // skip over the size of the entry name string
    curr_file_pos += this->entry_name_string_length;

    u64 addendum_name_string_pos = curr_file_pos + (RKV2FILEPATHADDENDUM_SIZE * this->filepath_addendum_count);

    printf("entering addendum cycle with curr_file_pos: 0x%.8X - addendum_name_string_pos: 0x%.8X\n", curr_file_pos, addendum_name_string_pos);
    // now read in the addendums
    for (u32 j = 0; j < this->filepath_addendum_count; j++) {
        this->addendums[j].filepath_addendum_string_offset = read_64LE(&this->data[curr_file_pos]);
        this->addendums[j].timestamp = read_32LE(&this->data[curr_file_pos + 0x8]);
        this->addendums[j].entry_name_string_offset = read_32LE(&this->data[curr_file_pos + 0xC]);

        curr_file_pos += RKV2FILEPATHADDENDUM_SIZE;

        u8* filepath_name = read_dynamic_string(&this->data[addendum_name_string_pos + this->addendums[j].filepath_addendum_string_offset], BASE_FILEPATHADDENDUM_STRING_SIZE);
        u8* name = read_dynamic_string(&this->data[addendum_name_string_pos + this->addendums[j].entry_name_string_offset], BASE_ENTRY_STRING_SIZE);
        printf("Addendum Name: %s - String Offset: 0x%.8X - Linked Name: %s\n", filepath_name, this->addendums[j].filepath_addendum_string_offset, name);
        delete filepath_name;
        delete name;
    }
    printf("addendums complete\n");

    printf("all good :)\n");
    return true;
}

bool RKV2File::extract(const u8* output_path) {
    return false;
}
