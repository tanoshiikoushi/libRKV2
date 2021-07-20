#ifndef RKV2_H_INCLUDED

#include "types/typedefs.h"

class RKV2File {
    // these are private since you should not interface directly with the underlying structure I use
    private:
        u8* data;
        u64 data_size;

        u32 entry_count;
        u32 entry_name_string_length; // one big string that is properly zero terminated -- libKoushiCommon will contain an amortized O(1) loader for this
        u32 filepath_addendum_count;
        u32 filepath_addendum_string_length; // same as above
        u32 metadata_table_offset;
        u32 metadata_table_length; // we don't really use this, but it doesn't hurt to track so we can update it properly later

        RKV2Entry* entries;
        RKV2FilePathAddendum* addendums;

    // these are public since you should use the functions I provide
    public:
        // functions that I will need
        bool load(const u8* buf_to_copy, const u32 buf_size);
        bool extract(const u8* output_path); // this is going to be temporary and will use std::fstream, but I really do need to clean it up x.x
        // load_filesystem();
        // repack();
        // obviously more to come

    ~RKV2File() {
        delete data;
        delete[] entries;
        delete[] addendums;
    };
};

class RKV2Entry {
    // these are public so that you can interact with the underlying entires if desired
    public:
        u32 entry_name_string_offset;
        u32 padding; // could be an entry/addendum relationship according to Pixel, but seems to be unused from what I can tell
        u32 entry_size;
        u32 entry_offset;
        u32 entry_crc32eth;
};

class RKV2FilePathAddendum {
    // these are public so that you can interact with the underlying addendums if desired
    public:
        u64 filepath_addendum_string_offset;
        u32 timestamp;
        u32 entry_name_string_offset; // this ties the addendums to the entry
};

#define RKV2_H_INCLUDED

#endif // RKV2_H_INCLUDED
