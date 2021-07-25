#include <cstring>
#include <fstream>
#include <filesystem>
#include <stdio.h>
#include "string_utilities/string_buf_readers.h"
#include "rw_utilities/mem_read_utilities.h"
#include "RKV2.h"

bool RKV2File::load(const u8* buf_to_copy, const u64 buf_size) {
    // copy into our internal buffer
    this->data = new u8[buf_size];
    this->data_size = buf_size;
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

    this->entry_name_string_pos = this->metadata_table_offset + (RKV2ENTRY_SIZE * this->entry_count);

    printf("entering entry cycle with curr_file_pos: 0x%.8X - name_string_pos: 0x%.8X\n", curr_file_pos, this->entry_name_string_pos);
    for (u32 i = 0; i < this->entry_count; i++) {
        this->entries[i].entry_name_string_offset = read_32LE(&this->data[curr_file_pos]);
        // next 4 bytes are padding
        this->entries[i].entry_size = read_32LE(&this->data[curr_file_pos + 0x8]);
        this->entries[i].entry_offset = read_32LE(&this->data[curr_file_pos + 0xC]);
        this->entries[i].entry_crc32eth = read_32LE(&this->data[curr_file_pos + 0x10]);

        curr_file_pos += RKV2ENTRY_SIZE;

        //printf("Entry String Offset: 0x%.8X - Entry Offset: 0x%.8X\n", entry_name_string_pos + this->entries[i].entry_name_string_offset, this->entries[i].entry_offset);
    }
    printf("entries complete\n");

    // skip over the size of the entry name string
    curr_file_pos += this->entry_name_string_length;

    this->filepath_addendum_string_pos = curr_file_pos + (RKV2FILEPATHADDENDUM_SIZE * this->filepath_addendum_count);

    printf("entering addendum cycle with curr_file_pos: 0x%.8X - addendum_name_string_pos: 0x%.8X\n", curr_file_pos, this->filepath_addendum_string_pos);
    // now read in the addendums
    for (u32 j = 0; j < this->filepath_addendum_count; j++) {
        this->addendums[j].filepath_addendum_string_offset = read_64LE(&this->data[curr_file_pos]);
        this->addendums[j].timestamp = read_32LE(&this->data[curr_file_pos + 0x8]);
        this->addendums[j].entry_name_string_offset = read_32LE(&this->data[curr_file_pos + 0xC]);

        curr_file_pos += RKV2FILEPATHADDENDUM_SIZE;

        //printf("Addendum String Pos: 0x%.8X\n", addendum_name_string_pos + this->addendums[j].filepath_addendum_string_offset);
    }
    printf("addendums complete\n");

    return true;
}

RKV2Entry* RKV2File::get_entry_by_string_offset(u32 off) {
    // if we don't find it, we just return a nullptr
    RKV2Entry* ret = nullptr;

    // we just loop until we find it
    for (int i = 0; i < this->entry_count; i++) {
        if (this->entries[i].entry_offset == off) {
            ret = &(this->entries[i]);
            break;
        }
    }

    return ret;
}

bool RKV2File::extract(const u8* output_path) {
    std::string out_path((char*)output_path);
    std::string dir_path;
    std::string file_path;
    std::string file_name;

    // remove trailing slash for directory creation
    if (out_path.back() == '/' || out_path.back() == '\\') {
        printf("removing slash from end of in path");
        out_path.pop_back();
    }

    if (!std::filesystem::create_directories(out_path)) {
        return false;
    }

    // add slash back since RKV2 paths don't have preceeding slashes
    out_path.append("\\");

    std::fstream out_file;

    bool* entries_touched = new bool[this->entry_name_string_length];
    for (int e = 0; e < this->entry_name_string_length; e++) {
        entries_touched[e] = false;
    }

    bool file_path_assigned = false;

    // first, we iterate through the addendums
    for (int i = 0; i < 10; i++) {
        if (!entries_touched[this->addendums[i].entry_name_string_offset] &&
            this->data[this->entry_name_string_pos + this->addendums[i].entry_name_string_offset] != 0x0) {

            entries_touched[this->addendums[i].entry_name_string_offset] = true;

            if (this->data[this->filepath_addendum_string_pos + this->addendums[i].filepath_addendum_string_offset] != 0x0) {
                file_path.assign((char*)&(this->data[this->filepath_addendum_string_pos + this->addendums[i].filepath_addendum_string_offset]));
                dir_path = file_path.substr(0, file_path.find_last_of("/\\"));

                if (!std::filesystem::create_directories(out_path + dir_path)) {
                    delete entries_touched;
                    return false;
                }

                file_path_assigned = true;
            } else {
                file_path_assigned = false;
                printf("No addendum path provided, extracting entry content regardless\n");
            }

            RKV2Entry* curr_entry = this->get_entry_by_string_offset(this->addendums[i].entry_name_string_offset);
            if (curr_entry == nullptr) {
                delete entries_touched;
                return false;
            }

            if (file_path_assigned) {
                out_file.open(out_path + file_path, std::ios_base::out | std::ios_base::binary);
            } else {
                file_name.assign((char*)&(this->data[this->entry_name_string_pos + curr_entry->entry_name_string_offset]));
                out_file.open(out_path + file_name, std::ios_base::out | std::ios_base::binary);
            }

            if (!out_file) {
                delete entries_touched;
                return false;
            }

            out_file.write((char*)&(this->data[curr_entry->entry_offset]), curr_entry->entry_size);
            out_file.close();

        } else {
            printf("Skipping addendum with no or redundant content\n");
        }
    }

    bool touched = false;
    // now, we iterate through any remaining entries
    for (int j = 0; j < 10; j++) {
        touched = entries_touched[this->entries[j].entry_name_string_offset];
        if(!touched &&
            this->data[this->entry_name_string_pos + this->entries[j].entry_name_string_offset] != 0x0) {
            // we have something that wasn't covered in an addendum
            entries_touched[this->entries[j].entry_name_string_offset] = true;
            file_name.assign((char*)&(this->data[this->entry_name_string_pos + this->entries[j].entry_name_string_offset]));

            out_file.open(out_path + file_name, std::ios_base::out | std::ios_base::binary);

            if (!out_file) {
                delete entries_touched;
                return false;
            }

            out_file.write((char*)&(this->data[this->entries[j].entry_offset]), this->entries[j].entry_size);
            out_file.close();

        } else {
            if (!touched) {
                printf("Skipping entry with no filename\n");
            }
        }
    }

    delete entries_touched;

    return true;
}
