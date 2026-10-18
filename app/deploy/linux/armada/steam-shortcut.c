// Adds a non-Steam game shortcut to a Steam user's binary shortcuts.vdf.
//
// Usage: steam-shortcut <shortcuts.vdf> <name> <exe> <start dir> <icon>
// Exits 0 if the shortcut was added, 2 if one for <exe> already exists, 1 on error.
//
// The file is a binary KeyValues document: a "shortcuts" map holding one map per
// shortcut, keyed "0", "1", ... Each node is a type byte, a NUL-terminated key and
// a value; 0x00 opens a map, 0x01 is a string, 0x02 is an int32 and 0x08 ends a map.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char* g_End;

static unsigned char* skipString(unsigned char* p)
{
    while (p < g_End && *p) p++;
    return p < g_End ? p + 1 : NULL;
}

// Skips the body of a map (after its key) through the closing 0x08
static unsigned char* skipMap(unsigned char* p)
{
    while (p && p < g_End && *p != 0x08) {
        unsigned char type = *p++;
        p = skipString(p);
        if (!p) return NULL;
        switch (type) {
        case 0x00: p = skipMap(p); break;
        case 0x01: p = skipString(p); break;
        case 0x02: case 0x03: case 0x04: case 0x06: p += 4; break;
        case 0x07: p += 8; break;
        default: return NULL;
        }
    }
    return (p && p < g_End) ? p + 1 : NULL;
}

static uint32_t crc32(const char* s)
{
    uint32_t crc = 0xFFFFFFFF;
    for (; *s; s++) {
        crc ^= (unsigned char)*s;
        for (int i = 0; i < 8; i++) crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
    return ~crc;
}

static void putStr(FILE* f, const char* key, const char* val)
{
    fputc(0x01, f); fputs(key, f); fputc(0, f); fputs(val, f); fputc(0, f);
}

static void putInt(FILE* f, const char* key, uint32_t val)
{
    unsigned char b[4] = { val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, val >> 24 };
    fputc(0x02, f); fputs(key, f); fputc(0, f); fwrite(b, 1, 4, f);
}

int main(int argc, char** argv)
{
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <shortcuts.vdf> <name> <exe> <start dir> <icon>\n", argv[0]);
        return 1;
    }
    const char *path = argv[1], *name = argv[2], *exe = argv[3], *dir = argv[4], *icon = argv[5];

    unsigned char* data = NULL;
    long len = 0;
    FILE* in = fopen(path, "rb");
    if (in) {
        fseek(in, 0, SEEK_END);
        len = ftell(in);
        fseek(in, 0, SEEK_SET);
        data = malloc(len + 1);
        if (!data || fread(data, 1, len, in) != (size_t)len) { fclose(in); return 1; }
        fclose(in);
    }

    char quotedExe[4096], quotedDir[4096];
    snprintf(quotedExe, sizeof(quotedExe), "\"%s\"", exe);
    snprintf(quotedDir, sizeof(quotedDir), "\"%s\"", dir);

    // Find the end of the "shortcuts" map and the next free index
    long insertAt = 0;
    int nextIndex = 0;
    if (len > 0) {
        static const unsigned char header[] = "\0shortcuts";
        if (len < (long)sizeof(header) + 2 || memcmp(data, header, sizeof(header)) != 0) {
            fprintf(stderr, "%s: not a shortcuts.vdf file\n", path);
            return 1;
        }
        g_End = data + len;
        unsigned char* p = data + sizeof(header);
        while (p && p < g_End && *p != 0x08) {
            if (*p != 0x00) { p = NULL; break; }
            int index = atoi((char*)p + 1);
            if (index >= nextIndex) nextIndex = index + 1;
            unsigned char* body = skipString(p + 1);
            unsigned char* next = skipMap(body);
            if (body && next) {
                // Already added?
                for (unsigned char* q = body; q + strlen(quotedExe) <= next; q++) {
                    if (memcmp(q, quotedExe, strlen(quotedExe)) == 0) return 2;
                }
            }
            p = next;
        }
        if (!p || p >= g_End) {
            fprintf(stderr, "%s: unable to parse\n", path);
            return 1;
        }
        insertAt = p - data;
    }

    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE* out = fopen(tmp, "wb");
    if (!out) return 1;

    if (len > 0) {
        fwrite(data, 1, insertAt, out);
    }
    else {
        fputc(0x00, out); fputs("shortcuts", out); fputc(0, out);
    }

    char key[16];
    snprintf(key, sizeof(key), "%d", nextIndex);
    char idSource[8192];
    snprintf(idSource, sizeof(idSource), "%s%s", quotedExe, name);

    fputc(0x00, out); fputs(key, out); fputc(0, out);
    putInt(out, "appid", crc32(idSource) | 0x80000000);
    putStr(out, "AppName", name);
    putStr(out, "Exe", quotedExe);
    putStr(out, "StartDir", quotedDir);
    putStr(out, "icon", icon);
    putStr(out, "ShortcutPath", "");
    putStr(out, "LaunchOptions", "");
    putInt(out, "IsHidden", 0);
    putInt(out, "AllowDesktopConfig", 1);
    putInt(out, "AllowOverlay", 1);
    putInt(out, "OpenVR", 0);
    putInt(out, "Devkit", 0);
    putStr(out, "DevkitGameID", "");
    putInt(out, "DevkitOverrideAppID", 0);
    putInt(out, "LastPlayTime", 0);
    putStr(out, "FlatpakAppID", "");
    fputc(0x00, out); fputs("tags", out); fputc(0, out); fputc(0x08, out);
    fputc(0x08, out);

    if (len > 0) {
        fwrite(data + insertAt, 1, len - insertAt, out);
    }
    else {
        fputc(0x08, out); fputc(0x08, out);
    }

    if (fclose(out) != 0 || rename(tmp, path) != 0) {
        remove(tmp);
        return 1;
    }
    return 0;
}
