#ifndef DsdPlugin_H
#define DsdPlugin_H

#include "MetadataPlugin.h"
#include "Data.h"

/* The DSF plugin read metadata from in files
 * in DSD format. Most of code in this plugin
 * is initially copied from MPD source code.
 * You can find original code in :
 * http://www.musicpd.org/
 */

class DsdPlugin : public MetadataPlugin
{
    Q_OBJECT

public:
    DsdPlugin();
    ~DsdPlugin();

    bool update(EMSTrack *track);

private:
    bool getDsfMetadata(EMSTrack *track);
    bool getDsdiffMetadata(EMSTrack *track);
};

/* Structures representing DSF header */
struct DsdId {
    char value[4];
    bool Equals(const char *s) const
    {
        return memcmp(value, s, sizeof(value)) == 0;
    }
};

/* Use a struct of 2x32bits to avoid alignment */
struct DsdUint64 {
    uint32_t lo;
    uint32_t hi;
};

struct DsfHeader {
    /** DSF header id: "DSD " */
    DsdId id;
    /** DSD chunk size, including id = 28 */
    DsdUint64 size;
    /** total file size */
    DsdUint64 fsize;
    /** pointer to id3v2 metadata, should be at the end of the file */
    DsdUint64 pmeta;
};

/** DSF file fmt chunk */
struct DsfFmtChunk {
    /** id: "fmt " */
    DsdId id;
    /** fmt chunk size, including id, normally 52 */
    DsdUint64 size;
    /** version of this format = 1 */
    uint32_t version;
    /** 0: DSD raw */
    uint32_t formatid;
    /** channel type, 1 = mono, 2 = stereo, 3 = 3 channels, etc */
    uint32_t channeltype;
    /** Channel number, 1 = mono, 2 = stereo, ... 6 = 6 channels */
    uint32_t channelnum;
    /** sample frequency: 2822400, 5644800 */
    uint32_t sample_freq;
    /** bits per sample 1 or 8 */
    uint32_t bitssample;
    /** Sample count per channel in bytes */
    DsdUint64 scnt;
    /** block size per channel = 4096 */
    uint32_t block_size;
    /** reserved, should be all zero */
    uint32_t reserved;
};

struct DsdiffHeader {
    DsdId id;
    DsdUint64 size;
    DsdId format;
};

struct DsdiffChunkHeader {
    DsdId id;
    DsdUint64 size;
};

#endif // DsdPlugin_H
