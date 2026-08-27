#include <stdint.h>

#define COEF0 0x510FB093u
#define COEF1 0xA3CBEADCu
#define COEF2 0x5993A17Eu
#define COEF3 0xC7ADEB03u

typedef int (*fal_partition_write_fn)(void *part, uint32_t offset, const uint8_t *buf, uint32_t size);

static uint32_t rol32(uint32_t v, unsigned int bits)
{
    return (v << bits) | (v >> (32u - bits));
}

static uint32_t pn15(uint32_t v, uint32_t disabled)
{
    uint32_t part1;
    uint32_t x;
    uint32_t part2;

    if (disabled)
        return 0;

    v &= 0xffffu;
    part1 = ((v << 9) + (v >> 7)) & 0xffffu;
    x = v >> 5;
    part2 = ((x << 12) + ((x & 0xfu) << 8) + ((x << 4) & 0xffu) + (x & 0xfu)) & 0x6371u;
    return part1 ^ part2;
}

static uint32_t pn16(uint32_t v, uint32_t disabled)
{
    uint32_t part1;
    uint32_t seed;
    uint32_t part2;

    if (disabled)
        return 0;

    v &= 0x1ffffu;
    part1 = (((v & 0x3ffu) << 7) + ((v >> 10) & 0x7fu)) & 0x1ffffu;
    seed = (((v >> 1) & 1u) << 3) + (((v >> 5) & 1u) << 2) + (((v >> 9) & 1u) << 1) + ((v >> 13) & 1u);
    part2 = ((((v >> 4) & 1u) << 16) + (seed << 12) + (seed << 8) + (seed << 4) + seed) & 0x13659u;
    return part1 ^ part2;
}

static uint32_t pn32(uint32_t v, uint32_t disabled)
{
    uint32_t seed;
    uint32_t part2;

    if (disabled)
        return 0;

    seed = (v >> 2) & 0xfu;
    part2 = (seed << 28) + (seed << 24) + (seed << 20) + (seed << 16) + (seed << 12) + (seed << 8) + (seed << 4) + seed;
    return rol32(v, 17) ^ (part2 & 0xE519A4F1u);
}

static uint32_t enc_word(uint32_t addr, uint32_t data)
{
    uint32_t all_disabled;
    uint32_t high_a;
    uint32_t high_b;
    uint32_t low_b;
    uint32_t pn15_input;
    uint32_t pn16_input;
    uint32_t pn32_input;
    uint32_t word_mode;
    uint32_t rotate_mode;
    uint32_t mask;

    all_disabled = (((COEF3 & 0xff000000u) == 0xff000000u) || ((COEF3 & 0xff000000u) == 0));
    word_mode = (COEF3 >> 5) & 3u;
    rotate_mode = (COEF3 >> 11) & 3u;

    high_a = (((addr >> 24) << 8) + ((addr >> 16) & 0xffu)) & 0xffffu;
    high_b = (((addr >> 16) << 8) + ((addr >> 24) & 0xffu)) & 0xffffu;
    low_b = ((addr << 8) + ((addr >> 8) & 0xffu)) & 0xffffu;

    if (word_mode == 0)
        pn15_input = high_a ^ (addr & 0xffffu);
    else if (word_mode == 1)
        pn15_input = high_a ^ low_b;
    else if (word_mode == 2)
        pn15_input = high_b ^ (addr & 0xffffu);
    else
        pn15_input = high_b ^ low_b;

    pn16_input = (addr >> ((COEF3 >> 8) & 3u)) & 0x1ffffu;
    if (rotate_mode == 0)
        pn32_input = addr;
    else if (rotate_mode == 1)
        pn32_input = rol32(addr, 24);
    else if (rotate_mode == 2)
        pn32_input = rol32(addr, 16);
    else
        pn32_input = rol32(addr, 8);

    mask = (pn15(((COEF1 >> 16) ^ pn15_input) & 0xffffu, all_disabled || (COEF3 & 1u)) & 0xffffu) << 16;
    mask |= pn16(((((COEF1 >> 8) & 0xffu) << 9) + (((COEF3 >> 4) & 1u) << 8) + (COEF1 & 0xffu)) ^ pn16_input,
                 all_disabled || (COEF3 & 2u)) & 0xffffu;
    mask ^= pn32(COEF0 ^ pn32_input, all_disabled || (COEF3 & 4u));
    if (!(all_disabled || (COEF3 & 8u)))
        mask ^= COEF2;

    return data ^ mask;
}

int tuya_fal_partition_write(void *part, uint32_t offset, uint8_t *buf, uint32_t size)
{
    uint32_t padded_size;
    uint32_t addr;
    uint32_t i;

    padded_size = (size + 31u) & ~31u;
    for (i = size; i < padded_size; i++)
        buf[i] = 0xffu;

    addr = *(const uint32_t *)((const uint8_t *)part + 0x34u) + offset;
    for (i = 0; i < padded_size; i += 4) {
        uint32_t word = (uint32_t)buf[i] | ((uint32_t)buf[i + 1] << 8) | ((uint32_t)buf[i + 2] << 16) | ((uint32_t)buf[i + 3] << 24);
        word = enc_word(addr + i, word);
        buf[i] = (uint8_t)word;
        buf[i + 1] = (uint8_t)(word >> 8);
        buf[i + 2] = (uint8_t)(word >> 16);
        buf[i + 3] = (uint8_t)(word >> 24);
    }

    return ((fal_partition_write_fn)0x7779u)(part, offset, buf, padded_size);
}
