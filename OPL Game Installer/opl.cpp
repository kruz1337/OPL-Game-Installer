#include "opl.h"
#include "utilities.h"

#include <curl/curl.h>
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "ws2_32.lib")

size_t curl_write_data_callback(void* contents, size_t size, size_t nmemb, std::vector<unsigned char>* data) {
    size_t total_size = size * nmemb;
    data->insert(data->end(), static_cast<unsigned char*>(contents), static_cast<unsigned char*>(contents) + total_size);
    return total_size;
}

bool OPL::fetchImage(const char* url, std::vector<unsigned char>& buffer) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);

    return true;
}

int OPL::downloadArts(const char* dir, const char* gameId) {
    std::vector<unsigned char> buffer;

    int fetched = 0;

    const char* gameArts[] = { "LGO.png", "LAB.jpg", "ICO.png", "COV.jpg", "COV.png", "COV2.jpg", "SCR_00.jpg", "SCR_01.jpg", "BG_00.jpg" };
    for (auto& art : gameArts) {
        buffer.clear();

        std::string erasedName = std::string(gameId) + "_" + replaceMultiStr(art, { "_00", "_01"}, {"", "2"});
        std::string url("https://requestx.dev/ARTS/" + std::string(gameId) + "/" + std::string(gameId) + "_" + art);

        bool result = OPL::fetchImage(url.c_str(), buffer);
        if (!result) {
            continue;
        }

        std::ofstream img(std::string(dir) + "\\" + erasedName, std::ios::binary);
        if (!img) {
            return -1;
        }

        img.write(reinterpret_cast<const char*>(&buffer[0]), buffer.size());
        img.close();

        fetched++;
    }

    return fetched > 0 ? 0 : -2;
}

bool OPL::writeCfg(const char* path, const char* key, const char* value) {
    std::ifstream input(path);    
    std::ofstream output(path);

    std::string cfg = ((std::string)key + '=' + value);

    if (input) {
        bool f = false;
        std::string line;

        while (input >> line)
        {
            if (line.find(key + std::string("=")) == 0) {
                line = cfg;
                f = true;
            }

            line += "\n";
            output << line;
        }

        if (!f) {
            output << cfg;
        }

        return true;
    }
    else if (output) {
        output << cfg;
    }
    else {
        return false;
    }

    input.close();
    output.close();

    return true;
}

//These following functions are taked from https://github.com/ps2homebrew/Open-PS2-Loader/
int OPL::writeUl(const char* drive, const char* game_name, const char* game_id, const char* media, int parts)
{
    typedef struct
    {
        char name[32];
        char image[15];
        unsigned char parts;
        unsigned char media;
        unsigned char pad[15];
    } cfg_t;

    FILE* fh_cfg;
    cfg_t cfg;
    char cfg_path[256];
    int r;

#ifdef _WIN32
    if (strlen(drive) == 1)
        sprintf(cfg_path, "%s:\\ul.cfg", drive);
    else
        sprintf(cfg_path, "%s\\ul.cfg", drive);
#else
    sprintf(cfg_path, "%s/ul.cfg", drive);
#endif
    memset(&cfg, 0, sizeof(cfg_t));

    strncpy(cfg.name, game_name, 32);
    sprintf(cfg.image, "ul.%s", game_id);
    cfg.parts = parts;
    cfg.pad[4] = 0x08;

    if (!strcmp(media, "CD"))
        cfg.media = 0x12;
    else if (!strcmp(media, "DVD"))
        cfg.media = 0x14;

    fh_cfg = fopen(cfg_path, "ab");
    if (!fh_cfg)
        return -1;

    r = fwrite(&cfg, 1, sizeof(cfg_t), fh_cfg);
    if (r != sizeof(cfg_t)) {
        fclose(fh_cfg);
        return -2;
    }

    fclose(fh_cfg);

    return 0;
}

int OPL::crc32Hex(const char* string)
{
    int crctab[0x400];
    int crc, table, count, byte;

    for (table = 0; table < 256; table++) {
        crc = table << 24;

        for (count = 8; count > 0; count--) {
            if (crc < 0)
                crc = crc << 1;
            else
                crc = (crc << 1) ^ 0x04C11DB7;
        }
        crctab[255 - table] = crc;
    }

    do {
        byte = string[count++];
        crc = crctab[byte ^ ((crc >> 24) & 0xFF)] ^ ((crc << 8) & 0xFFFFFF00);
    } while (string[count - 1] != 0);

    return crc;
}

typedef struct
{
    unsigned char magic[28];
    unsigned char version[12];
    unsigned short pagesize;
    unsigned short pages_per_cluster;
    unsigned short blocksize;
    unsigned short unused;
    unsigned int clusters_per_card;
    unsigned int alloc_offset;
    unsigned int alloc_end;
    unsigned int rootdir_cluster;
    unsigned int backup_block1;
    unsigned int backup_block2;
    unsigned char unused2[8];
    unsigned int ifc_list[32];
    int bad_block_list[32];
    unsigned char cardtype;
    unsigned char cardflags;
    unsigned short unused3;
    unsigned int cluster_size;
    unsigned int FATentries_per_cluster;
    unsigned int clusters_per_block;
    int cardform;
    unsigned int rootdir_cluster2;
    unsigned int unknown1;
    unsigned int unknown2;
    unsigned int max_allocatable_clusters;
    unsigned int unknown3;
    unsigned int unknown4;
    int unknown5;
} MCDevInfo;

static MCDevInfo devinfo;

typedef struct _sceMcStDateTime
{
    unsigned char Resv2;
    unsigned char Sec;
    unsigned char Min;
    unsigned char Hour;
    unsigned char Day;
    unsigned char Month;
    unsigned short Year;
} sceMcStDateTime;

static void long_multiply(unsigned int v1, unsigned int v2, unsigned int* HI, unsigned int* LO)
{
    register long a, b, c, d;
    register long x, y;

    a = (v1 >> 16) & 0xffff;
    b = v1 & 0xffff;
    c = (v2 >> 16) & 0xffff;
    d = v2 & 0xffff;

    *LO = b * d;
    x = a * d + c * b;
    y = ((*LO >> 16) & 0xffff) + x;

    *LO = (*LO & 0xffff) | ((y & 0xffff) << 16);
    *HI = (y >> 16) & 0xffff;

    *HI += a * c;
}

static int mc_getmcrtime(sceMcStDateTime* mctime)
{
    time_t rawtime;
    struct tm* ptm;

    time(&rawtime);
    ptm = gmtime(&rawtime);

    mctime->Resv2 = 0;
    mctime->Sec = ((((ptm->tm_sec >> 4) << 2) + (ptm->tm_sec >> 4)) << 1) + (ptm->tm_sec & 0xf);
    mctime->Min = ((((ptm->tm_min >> 4) << 2) + (ptm->tm_min >> 4)) << 1) + (ptm->tm_min & 0xf);
    mctime->Hour = ((((ptm->tm_hour >> 4) << 2) + (ptm->tm_hour >> 4)) << 1) + (ptm->tm_hour & 0xf);
    mctime->Day = ((((ptm->tm_mday >> 4) << 2) + (ptm->tm_mday >> 4)) << 1) + (ptm->tm_mday & 0xf);

    mctime->Month = (ptm->tm_mon + 1) & 0xf;

    mctime->Year = (((((ptm->tm_year - 100) >> 4) << 2) + ((ptm->tm_year - 100) >> 4)) << 1) + (((ptm->tm_year - 100) & 0xf) | 0x7d0);

    return 0;
}

static int mc_writecluster(FILE* fd, int cluster, void* buf, int dup)
{
    register int r, size;
    MCDevInfo* mcdi = (MCDevInfo*)&devinfo;

    fseek(fd, cluster * mcdi->cluster_size, SEEK_SET);
    size = mcdi->cluster_size * dup;
    r = fwrite(buf, 1, size, fd);
    if (r != size)
        return -1;

    return 0;
}

int OPL::createVmc(const char* filename, int size_kb, int blocksize) {
    typedef struct
    {
        unsigned short mode;
        unsigned short unused;
        unsigned int length;
        sceMcStDateTime created;
        unsigned int cluster;
        unsigned int dir_entry;
        sceMcStDateTime modified;
        unsigned int attr;
        unsigned int unused2[7];
        char name[32];
        unsigned char unused3[416];
    } McFsEntry;

    static char SUPERBLOCK_MAGIC[] = "Sony PS2 Memory Card Format ";
    static char SUPERBLOCK_VERSION[] = "1.2.0.0";
    static unsigned char cluster_buf[(16 * 1024) + 16];
    static FILE* genvmc_fh = NULL;

    register int i, r, b, ifc_index, fat_index;
    register int ifc_length, fat_length, alloc_offset;
    register int ret, j = 0, z = 0;
    MCDevInfo* mcdi = (MCDevInfo*)&devinfo;

    genvmc_fh = fopen(filename, "wb");
    if (genvmc_fh == NULL)
        return -101;

    memset((void*)&mcdi->magic, 0, sizeof(mcdi->magic) + sizeof(mcdi->version));
    strcpy((char*)&mcdi->magic, SUPERBLOCK_MAGIC);
    strcat((char*)&mcdi->magic, SUPERBLOCK_VERSION);

    mcdi->cluster_size = 1024;
    mcdi->blocksize = blocksize;
    mcdi->pages_per_cluster = 2;
    mcdi->pagesize = mcdi->cluster_size / mcdi->pages_per_cluster;
    mcdi->clusters_per_block = mcdi->blocksize / mcdi->pages_per_cluster;
    mcdi->clusters_per_card = (size_kb * 1024) / mcdi->cluster_size;
    mcdi->cardtype = 0x02;
    mcdi->cardflags = 0x2b;
    mcdi->cardform = -1;
    mcdi->FATentries_per_cluster = mcdi->cluster_size / sizeof(unsigned int);

    for (i = 0; i < 32; i++)
        mcdi->bad_block_list[i] = -1;

    memset(cluster_buf, 0xff, sizeof(cluster_buf));
    for (i = 0; i < mcdi->clusters_per_card; i += 16) {
        r = mc_writecluster(genvmc_fh, i, cluster_buf, 16);
        if (r < 0) {
            r = -102;
            ret = fclose(genvmc_fh);
            if (!(ret < 0))
                genvmc_fh = NULL;

            return r;
        }
    }

    fat_length = (((mcdi->clusters_per_card << 2) - 1) / mcdi->cluster_size) + 1;
    ifc_length = (((fat_length << 2) - 1) / mcdi->cluster_size) + 1;

    if (!(ifc_length <= 32)) {
        ifc_length = 32;
        fat_length = mcdi->FATentries_per_cluster << 5;
    }

    for (i = 0; i < 32; i++)
        mcdi->ifc_list[i] = -1;
    ifc_index = mcdi->blocksize / 2;
    i = ifc_index;
    for (j = 0; j < ifc_length; j++, i++)
        mcdi->ifc_list[j] = i;

    fat_index = i;

    unsigned char* ifc_mem = (unsigned char*)malloc((ifc_length * mcdi->cluster_size) + 0XFF);
    if (ifc_mem == NULL) {
        r = -103;
        ret = fclose(genvmc_fh);
        if (!(ret < 0))
            genvmc_fh = NULL;

        return r;
    }
    memset(ifc_mem, 0, ifc_length * mcdi->cluster_size);

    unsigned int* ifc = (unsigned int*)ifc_mem;
    for (j = 0; j < fat_length; j++, i++) {

        if (i >= mcdi->clusters_per_card) {
            free(ifc_mem);
            r = -104;
            ret = fclose(genvmc_fh);
            if (!(ret < 0))
                genvmc_fh = NULL;

            return r;
        }
        ifc[j] = i;
    }

    for (z = 0; z < ifc_length; z++) {
        r = mc_writecluster(genvmc_fh, mcdi->ifc_list[z], &ifc_mem[z * mcdi->cluster_size], 1);
        if (r < 0) {

            free(ifc_mem);
            r = -105;
            ret = fclose(genvmc_fh);
            if (!(ret < 0))
                genvmc_fh = NULL;

            return r;
        }
    }

    free(ifc_mem);

    alloc_offset = i;

    mcdi->backup_block1 = (mcdi->clusters_per_card / mcdi->clusters_per_block) - 1;
    mcdi->backup_block2 = (mcdi->clusters_per_card / mcdi->clusters_per_block) - 2;

    unsigned int hi, lo, temp;
    long_multiply(mcdi->clusters_per_card, 0x10624dd3, &hi, &lo);
    temp = (hi >> 6) - (mcdi->clusters_per_card >> 31);
    mcdi->max_allocatable_clusters = (((((temp << 5) - temp) << 2) + temp) << 3) + 1;
    j = alloc_offset;

    i = (mcdi->clusters_per_card / mcdi->clusters_per_block) - 2;
    for (z = 0; j < (i * mcdi->clusters_per_block); j += mcdi->FATentries_per_cluster) {

        memset(cluster_buf, 0, mcdi->cluster_size);
        unsigned int* fc = (unsigned int*)cluster_buf;
        int sz_u32 = (i * mcdi->clusters_per_block) - j;
        if (sz_u32 > mcdi->FATentries_per_cluster)
            sz_u32 = mcdi->FATentries_per_cluster;
        for (b = 0; b < sz_u32; b++)
            fc[b] = 0x7fffffff;

        if (z == 0) {
            mcdi->alloc_offset = j;
            mcdi->rootdir_cluster = 0;
            fc[0] = 0xffffffff;
        }
        z += sz_u32;

        r = mc_writecluster(genvmc_fh, fat_index++, cluster_buf, 1);
        if (r < 0) {
            r = -107;
            ret = fclose(genvmc_fh);
            if (!(ret < 0))
                genvmc_fh = NULL;

            return r;
        }
    }

    mcdi->alloc_end = (i * mcdi->clusters_per_block) - mcdi->alloc_offset;

    if (z < mcdi->clusters_per_block) {
        r = -108;
        ret = fclose(genvmc_fh);
        if (!(ret < 0))
            genvmc_fh = NULL;

        return r;
    }

    mcdi->unknown1 = 0;
    mcdi->unknown2 = 0;
    mcdi->unknown5 = -1;
    mcdi->rootdir_cluster2 = mcdi->rootdir_cluster;

    McFsEntry* rootdir_entry[2];
    sceMcStDateTime time;

    mc_getmcrtime(&time);
    rootdir_entry[0] = (McFsEntry*)&cluster_buf[0];
    rootdir_entry[1] = (McFsEntry*)&cluster_buf[sizeof(McFsEntry)];
    memset((void*)rootdir_entry[0], 0, sizeof(McFsEntry));
    memset((void*)rootdir_entry[1], 0, sizeof(McFsEntry));
    rootdir_entry[0]->mode = 0x8000 | 0x0400 | 0x20 | 0x01 | 0x02 | 0x04;
    rootdir_entry[0]->length = 2;
    memcpy((void*)&rootdir_entry[0]->created, (void*)&time, sizeof(sceMcStDateTime));
    memcpy((void*)&rootdir_entry[0]->modified, (void*)&time, sizeof(sceMcStDateTime));
    rootdir_entry[0]->cluster = 0;
    rootdir_entry[0]->dir_entry = 0;
    strcpy(rootdir_entry[0]->name, ".");
    rootdir_entry[1]->mode = 0x8000 | 0x2000 | 0x0400 | 0x20 | 0x02 | 0x04;
    rootdir_entry[1]->length = 2;
    memcpy((void*)&rootdir_entry[1]->created, (void*)&time, sizeof(sceMcStDateTime));
    memcpy((void*)&rootdir_entry[1]->modified, (void*)&time, sizeof(sceMcStDateTime));
    rootdir_entry[1]->cluster = 0;
    rootdir_entry[1]->dir_entry = 0;
    strcpy(rootdir_entry[1]->name, "..");

    r = mc_writecluster(genvmc_fh, mcdi->alloc_offset + mcdi->rootdir_cluster, cluster_buf, 1);
    if (r < 0) {
        r = -109;
        ret = fclose(genvmc_fh);
        if (!(ret < 0))
            genvmc_fh = NULL;

        return r;
    }

    mcdi->cardform = 1;

    memset(cluster_buf, 0xff, mcdi->cluster_size);
    memcpy(cluster_buf, (void*)mcdi, sizeof(MCDevInfo));
    r = mc_writecluster(genvmc_fh, 0, cluster_buf, 1);
    if (r < 0) {
        r = -110;
        ret = fclose(genvmc_fh);
        if (!(ret < 0))
            genvmc_fh = NULL;

        return r;
    }

    r = fclose(genvmc_fh);
    if (r < 0)
        return -111;
    genvmc_fh = NULL;

    return 0;
}