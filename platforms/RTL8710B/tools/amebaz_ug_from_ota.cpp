#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>

struct update_file_hdr {
    uint32_t FwVer;
    uint32_t HdrNum;
};

struct update_file_img_hdr {
    char     ImgId[4];      // "OTA1"/"OTA2"
    uint32_t ImgHdrLen;     // 24
    uint32_t Checksum;      // 32-bit sum of image bytes
    uint32_t ImgLen;        // image length
    uint32_t Offset;        // offset in file where image starts
    uint32_t FlashOffset;   // flash address
};

static uint32_t checksum_u32(const std::vector<uint8_t> &buf) {
    uint64_t s = 0;
    for (uint8_t b : buf) {
        s += b;
    }
    return static_cast<uint32_t>(s & 0xFFFFFFFFu);
}

static void die(const char *msg) {
    std::fprintf(stderr, "ERROR: %s\n", msg);
    std::exit(1);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr,
            "Usage: %s <input_obk_ota.img> <output_tuya_ug.bin>\n",
            argv[0]);
        return 1;
    }

    std::string inPath  = argv[1];
    std::string outPath = argv[2];

    // --- read input OBK ota.img ---
    std::FILE *fi = std::fopen(inPath.c_str(), "rb");
    if (!fi) die("Cannot open input file");

    std::fseek(fi, 0, SEEK_END);
    long fsize = std::ftell(fi);
    std::fseek(fi, 0, SEEK_SET);
    if (fsize < 8) die("Input too small to be a valid OBK ota.img");

    std::vector<uint8_t> in(static_cast<size_t>(fsize));
    if (std::fread(in.data(), 1, in.size(), fi) != in.size()) {
        std::fclose(fi);
        die("Failed to read input file");
    }
    std::fclose(fi);

    // --- parse OBK header [len1,len2] ---
    uint32_t len1 = 0, len2 = 0;
    std::memcpy(&len1, &in[0], 4);
    std::memcpy(&len2, &in[4], 4);

    size_t expectedSize = 8ull + len1 + len2;
    if (expectedSize != in.size()) {
        die("Input size mismatch: (8 + len1 + len2) != file size");
    }

    // --- extract images ---
    const uint8_t *img1_ptr = &in[8];
    const uint8_t *img2_ptr = &in[8 + len1];

    std::vector<uint8_t> img1(img1_ptr, img1_ptr + len1);
    std::vector<uint8_t> img2(img2_ptr, img2_ptr + len2);

    // Optional sanity: check "81958711"
    if (!(img1.size() >= 8 && std::memcmp(img1.data(), "81958711", 8) == 0)) {
        std::fprintf(stderr, "WARNING: image1 does not start with \"81958711\"\n");
    }
    if (!(img2.size() >= 8 && std::memcmp(img2.data(), "81958711", 8) == 0)) {
        std::fprintf(stderr, "WARNING: image2 does not start with \"81958711\"\n");
    }

    // --- build Tuya/Realtek UG container ---

    // Constants from Tuya RTL8710BN 2M samples
    const uint32_t FW_VER           = 0x20170111;   // FwVer
    const uint32_t HDR_NUM          = 2;            // OTA1 + OTA2
    const uint32_t IMG_HDR_LEN      = 24;           // sizeof(update_file_img_hdr)
    const uint32_t FLASH_OTA1       = 0x0800B000;   // slot 1
    const uint32_t FLASH_OTA2       = 0x080D0000;   // slot 2

    const uint32_t FILE_HDR_SIZE    = 8;
    const uint32_t IMG_HDR_SIZE     = 24;
    const uint32_t TOTAL_HDR_SIZE   = FILE_HDR_SIZE + 2 * IMG_HDR_SIZE; // 56 (0x38)

    const uint32_t offset_ota1      = TOTAL_HDR_SIZE;
    const uint32_t offset_ota2      = TOTAL_HDR_SIZE + len1;

    update_file_hdr fh;
    fh.FwVer  = FW_VER;
    fh.HdrNum = HDR_NUM;

    update_file_img_hdr ih1{};
    std::memcpy(ih1.ImgId, "OTA1", 4);
    ih1.ImgHdrLen  = IMG_HDR_LEN;
    ih1.Checksum   = checksum_u32(img1);
    ih1.ImgLen     = len1;
    ih1.Offset     = offset_ota1;
    ih1.FlashOffset= FLASH_OTA1;

    update_file_img_hdr ih2{};
    std::memcpy(ih2.ImgId, "OTA2", 4);
    ih2.ImgHdrLen  = IMG_HDR_LEN;
    ih2.Checksum   = checksum_u32(img2);
    ih2.ImgLen     = len2;
    ih2.Offset     = offset_ota2;
    ih2.FlashOffset= FLASH_OTA2;

    // --- serialize UG file ---
    std::vector<uint8_t> out;
    out.reserve(TOTAL_HDR_SIZE + len1 + len2);

    auto append = [&](const void *p, size_t n) {
        const uint8_t *b = static_cast<const uint8_t*>(p);
        out.insert(out.end(), b, b + n);
    };

    append(&fh, sizeof(fh));
    append(&ih1, sizeof(ih1));
    append(&ih2, sizeof(ih2));
    append(img1.data(), img1.size());
    append(img2.data(), img2.size());

    // --- write output ---
    std::FILE *fo = std::fopen(outPath.c_str(), "wb");
    if (!fo) die("Cannot open output file");

    if (std::fwrite(out.data(), 1, out.size(), fo) != out.size()) {
        std::fclose(fo);
        die("Failed to write output file");
    }
    std::fclose(fo);

    std::fprintf(stderr,
        "Created Tuya-style UG from %s -> %s (size %zu bytes)\n",
        inPath.c_str(), outPath.c_str(), out.size());

    return 0;
}
