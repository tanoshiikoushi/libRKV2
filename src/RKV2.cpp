#include <fstream>
#include <cstring>
#include <stdio.h>
#include "string_utilities/string_buf_readers.h"
#include "rw_utilities/mem_read_utilities.h"
#include "RKV2.h"

bool RKV2File::load(const u8* buf_to_copy, const u64 buf_size) {
    std::fstream log_file;
    log_file.open("D:\\Ty 2 RE\\projs\\libRKV2\\log.txt", std::ios_base::out);
    char* out_buf = new char[0x100];
    u32 out_size = 0;

    // copy into our internal buffer
    this->data = new u8[buf_size];
    memcpy(this->data, buf_to_copy, buf_size);
    log_file << "past copy\n";

    // header check
    if (read_32BE(this->data) != 0x524B5632) { // "RKV2"
        log_file << "header bad";
        delete this->data;
        return false;
    }
    log_file << "header good\n";

    // read in the following useful info
    this->entry_count = read_32LE(&this->data[0x04]);
    this->entry_name_string_length = read_32LE(&this->data[0x08]);
    this->filepath_addendum_count = read_32LE(&this->data[0x0C]);
    this->filepath_addendum_string_length = read_32LE(&this->data[0x10]);
    this->metadata_table_offset = read_32LE(&this->data[0x14]);
    this->metadata_table_length = read_32LE(&this->data[0x18]);
    log_file << "metadata read\n";

    // allocate buffers for our entries and addendums
    this->entries = new RKV2Entry[this->entry_count];
    this->addendums = new RKV2FilePathAddendum[this->filepath_addendum_count];
    log_file << "buffers alloced\n";

    // now read in the entries
    u64 curr_file_pos = this->metadata_table_offset;

    u64 name_string_pos = this->metadata_table_offset + (RKV2ENTRY_SIZE * this->entry_count);

    out_size = snprintf(out_buf, 0x100, "entering entry cycle with curr_file_pos: 0x%.8X - name_string_pos: 0x%.8X\n", curr_file_pos, name_string_pos);
    log_file.write(out_buf, out_size);

    for (u32 i = 0; i < this->entry_count; i++) {
        out_size = snprintf(out_buf, 0x100, "pre-entry at 0x%.8X\n", curr_file_pos);
        log_file.write(out_buf, out_size);

        this->entries[i].entry_name_string_offset = read_32LE(&this->data[curr_file_pos]);
        // next 4 bytes are padding
        this->entries[i].entry_size = read_32LE(&this->data[curr_file_pos + 0x8]);
        this->entries[i].entry_offset = read_32LE(&this->data[curr_file_pos + 0xC]);
        this->entries[i].entry_crc32eth = read_32LE(&this->data[curr_file_pos + 0x10]);

        out_size = snprintf(out_buf, 0x100, "entry with details - str-off: 0x%.8X - size: 0x%.8X - offset: 0x%.8X - crc: 0x%.8X\n", this->entries[i].entry_name_string_offset,
                            this->entries[i].entry_size, this->entries[i].entry_offset, this->entries[i].entry_crc32eth);
        log_file.write(out_buf, out_size);

        curr_file_pos += RKV2ENTRY_SIZE;

        u8* name = nullptr;

        if (this->entries[i].entry_name_string_offset != 0x0) {
            name = read_dynamic_string(&this->data[name_string_pos + this->entries[i].entry_name_string_offset], BASE_ENTRY_STRING_SIZE);
        } else {
            name = new u8[0x32];
            strcpy((char*)name, "invalid-name");
        }
        log_file << "past entry name\n";

        out_size = snprintf(out_buf, 0x100, "Name: %p - String Offset: 0x%.4X - Entry Offset: 0x%.4X\n", name, this->entries[i].entry_name_string_offset, this->entries[i].entry_offset);
        log_file.write(out_buf, out_size);

        if (name != nullptr) {
            delete name;
        }
    }
    log_file << "entries complete\n";

    // skip over the size of the entry name string
    curr_file_pos += this->entry_name_string_length;

    u64 addendum_name_string_pos = curr_file_pos + (RKV2FILEPATHADDENDUM_SIZE * this->filepath_addendum_count);

    out_size = snprintf(out_buf, 0x100, "entering addendum cycle with curr_file_pos: 0x%.8X - addendum_name_string_pos: 0x%.8X\n", curr_file_pos, addendum_name_string_pos);
    log_file.write(out_buf, out_size);

    // now read in the addendums
    for (u32 j = 0; j < this->filepath_addendum_count; j++) {
        out_size = snprintf(out_buf, 0x100, "pre-addendum at 0x%.8X\n", curr_file_pos);
        log_file.write(out_buf, out_size);

        this->addendums[j].filepath_addendum_string_offset = read_64LE(&this->data[curr_file_pos]);
        this->addendums[j].timestamp = read_32LE(&this->data[curr_file_pos + 0x8]);
        this->addendums[j].entry_name_string_offset = read_32LE(&this->data[curr_file_pos + 0xC]);

        out_size = snprintf(out_buf, 0x100, "addendum with details - str-off: 0x%.8X - ts: 0x%.8X - entry-str-off: 0x%.8X\n", this->addendums[j].filepath_addendum_string_offset,
                            this->addendums[j].timestamp, this->addendums[j].entry_name_string_offset);
        log_file.write(out_buf, out_size);

        curr_file_pos += RKV2FILEPATHADDENDUM_SIZE;

        u8* filepath_name = nullptr;
        if (this->addendums[j].filepath_addendum_string_offset != 0x0) {
            filepath_name = read_dynamic_string(&this->data[addendum_name_string_pos + this->addendums[j].filepath_addendum_string_offset], BASE_FILEPATHADDENDUM_STRING_SIZE);
        } else {
            filepath_name = new u8[0x02];
            filepath_name[0x0] = '-';
            filepath_name[0x1] = 0x00;
        }
        log_file << "past filepath_name\n";

        u8* name = nullptr;
        if (this->addendums[j].entry_name_string_offset != 0x0) {
            name = read_dynamic_string(&this->data[name_string_pos + this->addendums[j].entry_name_string_offset], BASE_ENTRY_STRING_SIZE);
        } else {
            name = new u8[0x32];
            strcpy((char*)name, "invalid-name");
        }
        log_file << "past entry name\n";

        out_size = snprintf(out_buf, 0x100, "Addendum Name: %p - String Offset: 0x%.8X - Linked Name: %p\n", filepath_name, this->addendums[j].filepath_addendum_string_offset, name);
        log_file.write(out_buf, out_size);

        log_file << "pre-deletes\n";

        if (filepath_name != nullptr) {
            delete filepath_name;
        }
        log_file << "deleted filepath_name\n";

        if (name != nullptr) {
            delete name;
        }
        log_file << "delete name\n";
    }
    log_file << "addendums complete\n";

    log_file << "all good :)\n";

    log_file.close();
    delete out_buf;

    return true;
}

bool RKV2File::extract(const u8* output_path) {
    return false;
}
