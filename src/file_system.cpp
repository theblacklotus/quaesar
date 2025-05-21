// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "memory.h"

#include "fsusage.h"
#include "fsdb.h"
#include "zfile.h"
// clang-format on

#include <SDL_filesystem.h>
#include <SDL_log.h>
#include <SDL_rwops.h>
#include <stdio.h>
#include <sys/stat.h>
#include <cstring>  // For strcpy, strcat, etc.
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <Shlwapi.h>
#include <Windows.h>
#pragma comment(lib, "Shlwapi.lib")
#else  // POSIX

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/mount.h>
#elif defined(__linux__)
#include <sys/statvfs.h>
#endif

#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#define FILE_ATTRIBUTE_SYSTEM 0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020

#endif  // !WIN32

#undef min

static int fsdb_debug = 0;

#ifdef TRACE
#undef TRACE
#endif
#define TRACE() SDL_Log("WARN: Using of unimplemented function: '%s()'", __func__)
extern void debug(const char* x, ...);


/* these are deadly (but I think allowed on the Amiga): */
#define NUM_EVILCHARS 7
static char evilchars[NUM_EVILCHARS] = {'\\', '*', '?', '\"', '<', '>', '|'};
#define PATHPREFIX _T("\\\\?\\")


#define UAEFSDB2_LEN 1632
#define UAEFSDB_LEN 604


#ifdef _WIN32
TCHAR start_path_data[MAX_DPATH];
#else
char start_path_data[MAX_DPATH];
#endif


int pissoff_value = 15000 * CYCLE_UNIT;
int pause_emulation = 0;


int my_existsdir(const TCHAR* directoryPath) {
    debug("dir: '%s'", directoryPath);
    try {
        std::filesystem::path dirPath(directoryPath);
        return std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath) ? 1 : 0;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_existsdir: Error checking directory %s: %s", directoryPath, e.what());
        return 0;
    }
}


int my_existsfile(const TCHAR* name) {
    debug("name: '%s'", name);
    try {
        std::filesystem::path filePath(name);
        return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath) ? 1 : 0;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_existsfile: Error checking file %s: %s", name, e.what());
        return 0;
    }
}


bool my_chmod(const char* name, unsigned int mode) {
    try {
        std::filesystem::path filePath(name);
        auto perms = std::filesystem::status(filePath).permissions();

        // Adjust permissions based on mode
        if (mode & FILEFLAG_WRITE) {
            perms |= std::filesystem::perms::owner_write;
        } else {
            perms &= ~std::filesystem::perms::owner_write;
        }

        std::filesystem::permissions(filePath, perms);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_chmod: Error changing permissions for %s: %s", name, e.what());
        return false;
    }
}


int my_getvolumeinfo(const char* root) {
    debug("root: %s", root);
    std::filesystem::path filePath = root;

    try {
        if (!std::filesystem::exists(filePath))
            return -1;
        if (!std::filesystem::is_directory(filePath))
            return -2;
    } catch (const std::filesystem::filesystem_error& e) {
        debug("Filesystem error for %s: %s", root, e.what());
        return -1;
    }

    int ret = 0;

    // Check write permissions
    try {
        auto perms = std::filesystem::status(filePath).permissions();
        if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
            ret |= MYVOLUMEINFO_READONLY;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        debug("Permission check error for %s: %s", root, e.what());
    }

    // Check for named streams support (platform-specific)
#if defined(_WIN32)
    TCHAR volume[MAX_PATH];
    if (GetVolumePathNameA(root, volume, MAX_PATH)) {
        TCHAR fsname[MAX_PATH];
        DWORD flags;
        if (GetVolumeInformationA(volume, NULL, 0, NULL, NULL, &flags, fsname, MAX_PATH)) {
            if (flags & FILE_NAMED_STREAMS)
                ret |= MYVOLUMEINFO_STREAMS;
        }
    }
#elif defined(__APPLE__)
    struct statfs sfs;
    if (statfs(root, &sfs) == 0) {
        if (strcmp(sfs.f_fstypename, "apfs") == 0 || strcmp(sfs.f_fstypename, "hfs") == 0)
            ret |= MYVOLUMEINFO_STREAMS;
    }
#elif defined(__linux__)
    // Named streams are not generally supported on Linux.
    // This can be extended for specific filesystems like ZFS if needed.
#endif

    // Use SDL2 to get base path and check if it matches the root
    char* basePath = SDL_GetBasePath();
    if (basePath) {
        debug("SDL Base Path: %s", basePath);
        try {
            if (std::filesystem::equivalent(filePath, basePath)) {
                // Example logic, actual stream support is checked above.
            }
        } catch (const std::filesystem::filesystem_error& e) {
            debug("Filesystem equivalent check error: %s", e.what());
        }
        SDL_free(basePath);
    }

    return ret;
}


int my_rmdir(const char* name) {
    try {
        std::filesystem::remove(name);
        return 0;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_rmdir: Error removing directory %s: %s", name, e.what());
        return -1;
    }
}

void getpathpart(char* out, int size, const char* path) {
    if (!path || !out || size <= 0)
        return;

    try {
        std::filesystem::path p(path);
        std::string parent = p.parent_path().string();
        strncpy(out, parent.c_str(), size - 1);
        out[size - 1] = '\0';
    } catch (const std::exception& e) {
        SDL_Log("getpathpart: Error processing path %s: %s", path, e.what());
        out[0] = '\0';
    }
}

int my_unlink(const char* name, bool /*dontrecycle*/) {
    try {
        std::filesystem::remove(name);
        return 0;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_unlink: Error deleting file %s: %s", name, e.what());
        return -1;
    }
}


int get_fs_usage(const TCHAR* path, const TCHAR* disk, struct fs_usage* fsp) {
    try {
        std::filesystem::space_info spaceInfo = std::filesystem::space(path);
        fsp->total = spaceInfo.capacity;
        fsp->avail = spaceInfo.available;
        return 0;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("Filesystem error for %s: %s", path, e.what());
    }

    // Fallback or error
    SDL_Log("Warn using of unimplemented function: `%s` for path %s", __FUNCTION__, path);
    fsp->total = 1000000;  // Default dummy values
    fsp->avail = 1000000;
    return -1;  // Indicate error or unimplemented
}


int isprinter() {
    return 0;
}


void to_lower(TCHAR* s, int len) {
    for (int i = 0; i < len && s[i]; i++) {  // Added s[i] check for safety
        s[i] = tolower(s[i]);
    }
}

TCHAR* utf8u(const char* s) {
    if (s == NULL)
        return NULL;
    return ua(s);  // Assuming ua handles char* to TCHAR* (potentially wchar_t*)
}

char* uutf8(const TCHAR* s) {
    if (s == NULL)
        return NULL;
    return ua(s);  // Assuming ua handles TCHAR* (potentially wchar_t*) to char*
}

TCHAR* my_strdup_ansi(const char* src) {
    return strdup(src);
}


#define NO_TRANSLATION

TCHAR* au_fs(const char* src) {
#ifdef NO_TRANSLATION
    if (src == NULL)
        return NULL;
    return strdup(src);  // Assumes TCHAR is char or compatible
#else
    gsize read, written;
    gchar* result = g_convert(src, -1, "UTF-8", "ISO-8859-1", &read, &written, NULL);
    if (result == NULL) {
        write_log("WARNING: au_fs_copy failed to convert string %s", src);
        return strdup("");
    }
    return result;
#endif
}


char* ua_fs(const TCHAR* s, int defchar) {
#ifdef NO_TRANSLATION
    if (s == NULL)
        return NULL;
    return strdup(s);  // Assumes TCHAR is char or compatible
#else
    // we convert from fs-uae's internal encoding (UTF-8) to latin-1 here,
    // so file names can be read properly in the amiga

    char def[] = "?";
    if (defchar < 128 && defchar >= 0) {  // Added defchar >= 0 check
        def[0] = (char)defchar;
    }

    gsize read, written;
    gchar* result = g_convert_with_fallback(s, -1, "ISO-8859-1", "UTF-8", def, &read, &written, NULL);
    if (result == NULL) {
        write_log("WARNING: ua_fs failed to convert string %s", s);
        return strdup("");
    }

    // duplicate with libc malloc
    char* result_malloced = strdup(result);
    free(result);
    return result_malloced;
#endif
}


TCHAR* au_fs_copy(TCHAR* dst, int maxlen, const char* src) {
#ifdef NO_TRANSLATION
    if (!dst || maxlen <= 0)
        return NULL;
    dst[0] = 0;
    strncpy(dst, src, maxlen - 1);
    dst[maxlen - 1] = 0;  // Ensure null termination
    return dst;
#else
    gsize read, written;
    gchar* result = g_convert(src, -1, "UTF-8", "ISO-8859-1", &read, &written, NULL);
    if (result == NULL) {
        write_log("WARNING: au_fs_copy failed to convert string %s", src);
        if (dst && maxlen > 0)
            dst[0] = '\0';
        return dst;
    }

    strncpy(dst, result, maxlen - 1);
    dst[maxlen - 1] = 0;  // Ensure null termination
    free(result);
    return dst;
#endif
}


char* ua_fs_copy(char* dst, int maxlen, const TCHAR* src, int defchar) {
#ifdef NO_TRANSLATION
    if (!dst || maxlen <= 0)
        return NULL;
    dst[0] = 0;
    strncpy(dst, src, maxlen - 1);
    dst[maxlen - 1] = 0;  // Ensure null termination
    return dst;
#else
    char def[] = "?";
    if (defchar < 128 && defchar >= 0) {
        def[0] = (char)defchar;
    }

    gsize read, written;
    gchar* result = g_convert_with_fallback(src, -1, "ISO-8859-1", "UTF-8", def, &read, &written, NULL);
    if (result == NULL) {
        write_log("WARNING: ua_fs_copy failed to convert string %s", src);
        if (dst && maxlen > 0)
            dst[0] = '\0';
        return dst;
    }

    strncpy(dst, result, maxlen - 1);
    dst[maxlen - 1] = 0;  // Ensure null termination
    free(result);
    return dst;
#endif
}


TCHAR* target_expand_environment(const TCHAR* path, TCHAR* out, int maxlen) {
    debug("path:'%s'", path);
    if (!path)
        return NULL;

    std::string s_path = path;
    size_t pos = 0;

    while ((pos = s_path.find('$', pos)) != std::string::npos) {
        size_t end_pos;
        std::string var_name;

        if (s_path[pos + 1] == '{') {
            end_pos = s_path.find('}', pos + 2);
            if (end_pos == std::string::npos)
                break;
            var_name = s_path.substr(pos + 2, end_pos - (pos + 2));
            end_pos++;
        } else {
            end_pos = pos + 1;
            while (end_pos < s_path.length() && (isalnum(s_path[end_pos]) || s_path[end_pos] == '_')) {
                end_pos++;
            }
            var_name = s_path.substr(pos + 1, end_pos - (pos + 1));
        }

        const char* var_value = SDL_getenv(var_name.c_str());
        std::string value_str = var_value ? var_value : "";
        s_path.replace(pos, end_pos - pos, value_str);
        pos += value_str.length();
    }

    try {
        std::filesystem::path canonical_path = std::filesystem::weakly_canonical(s_path);
        s_path = canonical_path.string();
    } catch (const std::filesystem::filesystem_error& e) {
        debug("Filesystem error: %s", e.what());
    }

    if (out == NULL) {
        return strdup(s_path.c_str());
    } else {
        strncpy(out, s_path.c_str(), maxlen - 1);
        out[maxlen - 1] = '\0';
        return out;
    }
}


bool my_stat(const TCHAR* name, struct mystat* statbuf) {
    try {
        std::filesystem::path filePath(name);
        if (!std::filesystem::exists(filePath)) {
            return false;
        }

        auto fileStatus = std::filesystem::status(filePath);
        statbuf->size = std::filesystem::is_regular_file(filePath) ? std::filesystem::file_size(filePath) : 0;

        statbuf->mode = 0;
        if (std::filesystem::is_directory(filePath)) {
            statbuf->mode |= FILEFLAG_DIR;
        }
        if ((int)(fileStatus.permissions() & std::filesystem::perms::owner_write) == 0) {
            statbuf->mode |= FILEFLAG_READ;
        } else {
            statbuf->mode |= FILEFLAG_READ | FILEFLAG_WRITE;
        }

        auto ftime = std::filesystem::last_write_time(filePath);
        auto sdlTime = std::chrono::time_point_cast<std::chrono::microseconds>(ftime).time_since_epoch();
        statbuf->mtime.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(sdlTime).count();
        statbuf->mtime.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(sdlTime).count() % 1000000;

        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_stat: Error accessing %s: %s", name, e.what());
        return false;
    }
}


struct my_opendir_s {
    SDL_RWops* dir;
    std::vector<std::string> entries;
    size_t index;
};

int my_readdir(struct my_opendir_s* mod, TCHAR* name) {
    if (!mod || mod->index >= mod->entries.size())
        return 0;

    strncpy(name, mod->entries[mod->index].c_str(), MAX_DPATH - 1);
    name[MAX_DPATH - 1] = '\0';
    mod->index++;
    return 1;
}

struct my_opendir_s* my_opendir(const TCHAR* name, const TCHAR* mask) {
    struct my_opendir_s* mod = (struct my_opendir_s*)calloc(1, sizeof(struct my_opendir_s));
    if (!mod)
        return NULL;

    try {
        std::filesystem::path dirPath(name);
        std::string maskStr = mask ? mask : "*";

        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            std::string filename = entry.path().filename().string();
            if (maskStr == "*" || maskStr == "*.*" || filename == maskStr) {
                mod->entries.push_back(filename);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_opendir: Error opening directory %s: %s", name, e.what());
        free(mod);
        return NULL;
    }

    mod->index = 0;
    return mod;
}


struct my_opendir_s* my_opendir(const TCHAR* name) {
    return my_opendir(name, "*");
}


void my_closedir(struct my_opendir_s* mod) {
    if (mod) {
        free(mod);
    }
}


int hdf_write_target(struct hardfiledata* hfd, void* buffer, uae_u64 offset, int len, uint32_t* error) {
    UNIMPLEMENTED();
    if (error)
        *error = 1;
    return 0;
}


static void create_uaefsdb(a_inode* aino, uae_u8* buf, int winmode) {
    std::string nn = std::filesystem::path(aino->nname).filename().string();
    std::string aname = aino->aname ? aino->aname : "";
    std::string comment = aino->comment ? aino->comment : "";

    buf[0] = 1;
    do_put_mem_long((uae_u32*)(buf + 1), aino->amigaos_mode);

    strncpy((char*)buf + 5, aname.c_str(), 256);
    buf[5 + 256] = '\0';

    strncpy((char*)buf + 5 + 257, nn.c_str(), 256);
    buf[5 + 257 + 256] = '\0';

    strncpy((char*)buf + 5 + 2 * 257, comment.c_str(), 80);
    buf[5 + 2 * 257 + 80] = '\0';

    do_put_mem_long((uae_u32*)(buf + 5 + 2 * 257 + 81), winmode);

    strncpy((char*)(buf + 604), aname.c_str(), 256);
    strncpy((char*)(buf + 1118), nn.c_str(), 256);

    aino->has_dbentry = 0;
}


static std::string make_uaefsdbpath(const char* dir, const char* name = nullptr) {
    std::filesystem::path basePath(dir);
    if (name && *name) {
        basePath /= name;
    }
    basePath /= FSDB_FILE;
    return basePath.string();
}


DWORD GetFileAttributesSafe(const char* name) {
    if (!name) {
        return -1;  // INVALID_FILE_ATTRIBUTES
    }
    namespace fs = std::filesystem;

    fs::path filePath(name);
    try {
        if (!fs::exists(filePath)) {
            return -1;  // INVALID_FILE_ATTRIBUTES
        }

        DWORD attributes = 0;

        if (fs::is_directory(filePath)) {
            attributes |= FILE_ATTRIBUTE_DIRECTORY;
        }

        fs::file_status fst = fs::status(filePath);
        if ((fst.permissions() & fs::perms::owner_write) != fs::perms::none) {
            attributes |= FILE_ATTRIBUTE_READONLY;
        }

        // Check for hidden files (POSIX: files starting with '.')
        std::string filename = filePath.filename().string();
        if (!filename.empty() && filename[0] == '.') {
            attributes |= FILE_ATTRIBUTE_HIDDEN;
        }

        return attributes;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("GetFileAttributesSafe: Error accessing %s: %s", name, e.what());
        return -1;  // INVALID_FILE_ATTRIBUTES
    }
}


bool SetFileAttributesSafe(const char* name, uint32_t attr) {
    // Only "hidden" attribute is supported on POSIX (by renaming file)
    bool ok = true;
    bool want_hidden = (attr & FILE_ATTRIBUTE_HIDDEN) != 0;
    std::filesystem::path p(name);
    std::string filename = p.filename().string();
    if (want_hidden && filename[0] != '.') {
        std::filesystem::path newpath = p.parent_path() / ("." + filename);
        std::error_code ec;
        std::filesystem::rename(p, newpath, ec);
        ok = !ec;
    } else if (!want_hidden && filename[0] == '.') {
        std::filesystem::path newpath = p.parent_path() / filename.substr(1);
        std::error_code ec;
        std::filesystem::rename(p, newpath, ec);
        ok = !ec;
    }
    // Other attributes (readonly, archive, system) are not supported here
    return ok;
}


static int write_uaefsdb(const TCHAR* item, uae_u8* fsdb) {
    // Only supports writing the .uaefsdb file as a regular file in the same directory as 'item'
    // No Windows-specific file attributes or timestamps are preserved

    // Build .uaefsdb path
    std::filesystem::path itemPath(item);
    std::filesystem::path dir = itemPath.parent_path();
    std::filesystem::path uaefsdbPath = dir / FSDB_FILE;

    // Open file for writing (overwrite)
    SDL_RWops* file = SDL_RWFromFile(uaefsdbPath.string().c_str(), "wb");
    if (!file) {
        SDL_Log("write_uaefsdb: failed to open %s for writing: %s", uaefsdbPath.string().c_str(), SDL_GetError());
        return 0;
    }

    size_t written = SDL_RWwrite(file, fsdb, 1, UAEFSDB2_LEN);
    SDL_RWclose(file);

    if (written != UAEFSDB2_LEN) {
        SDL_Log("write_uaefsdb: failed to write all data to %s", uaefsdbPath.string().c_str());
        // Try to remove incomplete file
        std::error_code ec;
        std::filesystem::remove(uaefsdbPath, ec);
        return 0;
    }

    return 1;
}

struct a_inode_struct;


int fsdb_set_file_attrs(a_inode_struct* aino) {
    if (!aino || !aino->nname)
        return 0;

    uae_u8 fsdb[UAEFSDB2_LEN];
    memset(fsdb, 0, sizeof(fsdb));
    int mode = 0;

    // Set file attributes based on AmigaOS mode
    if (!(aino->amigaos_mode & A_FIBF_WRITE))
        mode |= FILE_ATTRIBUTE_READONLY;
    if (aino->amigaos_mode & A_FIBF_ARCHIVE)
        mode |= FILE_ATTRIBUTE_ARCHIVE;
    if (aino->amigaos_mode & A_FIBF_HIDDEN)
        mode |= FILE_ATTRIBUTE_HIDDEN;
    if (aino->amigaos_mode & A_FIBF_PURE)
        mode |= FILE_ATTRIBUTE_SYSTEM;

    // Create the .uaefsdb metadata
    create_uaefsdb(aino, fsdb, mode);

    // Write the metadata to the .uaefsdb file
    try {
        std::filesystem::path filePath(aino->nname);
        std::filesystem::path dirPath = filePath.parent_path();
        std::filesystem::path uaefsdbPath = dirPath / FSDB_FILE;

        SDL_RWops* file = SDL_RWFromFile(uaefsdbPath.string().c_str(), "wb");
        if (!file) {
            SDL_Log("fsdb_set_file_attrs: Failed to open %s for writing: %s", uaefsdbPath.string().c_str(),
                    SDL_GetError());
            return 0;
        }

        size_t written = SDL_RWwrite(file, fsdb, 1, UAEFSDB2_LEN);
        SDL_RWclose(file);

        if (written != UAEFSDB2_LEN) {
            SDL_Log("fsdb_set_file_attrs: Failed to write all data to %s", uaefsdbPath.string().c_str());
            std::filesystem::remove(uaefsdbPath);  // Remove incomplete file
            return 0;
        }

        return 1;  // Success
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("fsdb_set_file_attrs: Filesystem error for %s: %s", aino->nname, e.what());
        return 0;
    }
}


void fetch_nvrampath(TCHAR* out, int size) {
    UNIMPLEMENTED();
    if (out && size > 0)
        out[0] = '\0';
}

void fetch_configurationpath(TCHAR* out, int size) {
    debug("out:'%s'", out);
    out[0] = _T('/');
    out[1] = _T('.');
    out[2] = 0;
}


/* Return nonzero for any name we can't create on the native filesystem.  */
static int fsdb_name_invalid_2x(const char* n, int dir) {
    size_t i;
    static char s1[MAX_DPATH];
    static char s2[MAX_DPATH];
    char a = n[0];
    char b = (a == '\0' ? a : n[1]);
    char c = (b == '\0' ? b : n[2]);
    char d = (c == '\0' ? c : n[3]);
    size_t l = _tcslen(n);
    int ll;

    /* the reserved fsdb filename */
    if (_tcscmp(n, FSDB_FILE) == 0)
        return -1;

    if (dir) {
        if (n[0] == '.' && l == 1)
            return -1;
        if (n[0] == '.' && n[1] == '.' && l == 2)
            return -1;
    }

    if (a >= 'a' && a <= 'z')
        a -= 32;
    if (b >= 'a' && b <= 'z')
        b -= 32;
    if (c >= 'a' && c <= 'z')
        c -= 32;

    s1[0] = 0;
    s2[0] = 0;
    ua_fs_copy(s1, MAX_DPATH, n, -1);
    au_fs_copy(s2, MAX_DPATH, s1);
    if (_tcscmp(s2, n) != 0)
        return 1;

    if (currprefs.win32_filesystem_mangle_reserved_names) {
        /* reserved dos devices */
        ll = 0;
        if (a == 'A' && b == 'U' && c == 'X')
            ll = 3; /* AUX  */
        if (a == 'C' && b == 'O' && c == 'N')
            ll = 3; /* CON  */
        if (a == 'P' && b == 'R' && c == 'N')
            ll = 3; /* PRN  */
        if (a == 'N' && b == 'U' && c == 'L')
            ll = 3; /* NUL  */
        if (a == 'L' && b == 'P' && c == 'T' && (d >= '0' && d <= '9'))
            ll = 4; /* LPT# */
        if (a == 'C' && b == 'O' && c == 'M' && (d >= '0' && d <= '9'))
            ll = 4; /* COM# */
        /* AUX.anything, CON.anything etc.. are also illegal names */
        if (ll && (l == ll || (l > ll && n[ll] == '.')))
            return 3;

        /* spaces and periods at the end are a no-no */
        i = l - 1;
        if (n[i] == '.' || n[i] == ' ')
            return 1;
    }

    /* these characters are *never* allowed */
    for (i = 0; i < NUM_EVILCHARS; i++) {
        if (_tcschr(n, evilchars[i]) != 0)
            return 2;
    }

    return 0; /* the filename passed all checks, now it should be ok */
}


static int fsdb_name_invalid_2(a_inode* aino, const char* n, int dir) {
    int v = fsdb_name_invalid_2x(n, dir);
    if (v <= 1 || !aino)
        return v;

    try {
        std::filesystem::path filePath = std::filesystem::path(aino->nname) / n;

        if (!std::filesystem::exists(filePath)) {
            return 1;  // File does not exist
        }

        if (!std::filesystem::is_regular_file(filePath) && !std::filesystem::is_directory(filePath)) {
            return 1;  // Not a regular file or directory
        }
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("fsdb_name_invalid_2: Filesystem error for '%s': %s", n, e.what());
        return 1;  // Treat as invalid if an error occurs
    }

    return 0;  // Valid name
}

int fsdb_name_invalid_dir(a_inode* aino, const TCHAR* n) {
    int v = fsdb_name_invalid_2(aino, (const char*)n, 1);
    if (v <= 0)
        return v;

    SDL_Log("FILESYS: '%s' illegal filename", n);
    return v;
}


int fsdb_mode_supported(const a_inode*) {
    UNIMPLEMENTED();
    return 0;
}


TCHAR* fsdb_create_unique_nname(a_inode_struct*, char const*) {
    UNIMPLEMENTED();
    return nullptr;
}

int fsdb_mode_representable_p(a_inode_struct const*, int) {
    UNIMPLEMENTED();
    return 0;
}

int fsdb_name_invalid(a_inode_struct*, char const*) {
    UNIMPLEMENTED();
    return 0;
}


void my_canonicalize_path(const TCHAR* path, TCHAR* out, int size) {
    if (!path || !out || size <= 0) {
        if (out && size > 0) {
            out[0] = '\0';
        }
        return;
    }

    try {
        // Use std::filesystem to canonicalize the path
        std::filesystem::path inputPath(path);

        // Handle special cases for invalid or fake paths
        if (path[0] == ':' || path[0] == '\0' || _tcscmp(path, _T("\\")) == 0 || _tcscmp(path, _T("/")) == 0) {
            _tcsncpy(out, path, size);
            out[size - 1] = '\0';
            return;
        }

        // Skip network paths to prevent delays
        if (path[0] == '\\' && path[1] == '\\') {
            _tcsncpy(out, path, size);
            out[size - 1] = '\0';
            return;
        }

        // Canonicalize the path
        std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(inputPath);
        std::string canonicalStr = canonicalPath.string();

        // Copy the result to the output buffer
        _tcsncpy(out, canonicalStr.c_str(), size - 1);
        out[size - 1] = '\0';
    } catch (const std::filesystem::filesystem_error& e) {
        // Handle errors by falling back to the original path
        SDL_Log("my_canonicalize_path: Error canonicalizing path %s: %s", path, e.what());
        _tcsncpy(out, path, size);
        out[size - 1] = '\0';
    }
}


bool my_resolvessymboliclink(TCHAR* linkfile, int size) {
    if (!linkfile || size <= 0) {
        return false;
    }

    try {
        std::filesystem::path linkPath(linkfile);
        if (!std::filesystem::exists(linkPath) || !std::filesystem::is_symlink(linkPath)) {
            return false;
        }

        std::filesystem::path resolvedPath = std::filesystem::read_symlink(linkPath);
        std::filesystem::path parentPath = linkPath.parent_path();
        std::filesystem::path fullPath = std::filesystem::absolute(parentPath / resolvedPath);

        std::string resolvedStr = fullPath.string();
        strncpy(linkfile, resolvedStr.c_str(), size - 1);
        linkfile[size - 1] = '\0';

        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_resolvessymboliclink: Error resolving symbolic link %s: %s", linkfile, e.what());
        return false;
    }
}

TCHAR* fsdb_search_dir(const TCHAR* dirname, TCHAR* rel, TCHAR** relalt) {
    *relalt = NULL;
    try {
        std::filesystem::path dirPath(dirname);
        std::filesystem::path relPath(rel);
        std::filesystem::path fullPath = dirPath / relPath;

        if (std::filesystem::exists(fullPath)) {
            return my_strdup(rel);  // Exact match
        }

        // Check for *.lnk shortcut
        std::filesystem::path shortcutPath = fullPath;
        shortcutPath += ".lnk";

        if (std::filesystem::exists(shortcutPath) && !std::filesystem::is_directory(shortcutPath)) {
            TCHAR resolvedPath[MAX_DPATH];
            if (my_resolvessymboliclink((TCHAR*)shortcutPath.string().c_str(), MAX_DPATH)) {
                std::filesystem::path resolvedFilePath(resolvedPath);
                if (resolvedFilePath.filename() == relPath.filename()) {
                    return my_strdup(rel);  // Exact match after resolving
                } else {
                    *relalt = my_strdup((resolvedFilePath.filename().string() + ".lnk").c_str());
                    return my_strdup(resolvedFilePath.filename().string().c_str());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("fsdb_search_dir: Error searching directory %s: %s", dirname, e.what());
    }

    return NULL;
}


int fsdb_exists(char const*) {
    UNIMPLEMENTED();
    return 0;
}


static int read_uaefsdb(const TCHAR* dir, const TCHAR* name, uae_u8* fsdb) {
    try {
        // Construct the path to the .uaefsdb file
        std::filesystem::path uaefsdbPath = make_uaefsdbpath(dir, name);

        if (fsdb_debug) {
            write_log(_T("read_uaefsdb '%s'\n"), uaefsdbPath.string().c_str());
        }

        // Open the file using SDL_RWops
        SDL_RWops* file = SDL_RWFromFile(uaefsdbPath.string().c_str(), "rb");
        if (!file) {
            if (fsdb_debug) {
                write_log(_T("->fail: Unable to open file: %s\n"), SDL_GetError());
            }
            memset(fsdb, 0, UAEFSDB2_LEN);
            return 0;
        }

        // Read the file content into the buffer
        size_t bytesRead = SDL_RWread(file, fsdb, 1, UAEFSDB2_LEN);
        SDL_RWclose(file);

        if (bytesRead == UAEFSDB_LEN || bytesRead == UAEFSDB2_LEN) {
            if (fsdb_debug) {
                TCHAR *an, *nn, *co;
                write_log(_T("->ok\n"));
                an = au_fs((char*)fsdb + 5);
                nn = au_fs((char*)fsdb + 262);
                co = au_fs((char*)fsdb + 519);
                write_log(_T("v=%02x flags=%08x an='%s' nn='%s' c='%s'\n"), fsdb[0], ((uae_u32*)(fsdb + 1))[0], an, nn,
                          co);
                xfree(co);
                xfree(nn);
                xfree(an);
            }
            return 1;
        }

        if (fsdb_debug) {
            write_log(_T("->fail: Incomplete read, bytesRead=%zu\n"), bytesRead);
        }
        memset(fsdb, 0, UAEFSDB2_LEN);
        return 0;
    } catch (const std::filesystem::filesystem_error& e) {
        if (fsdb_debug) {
            write_log(_T("->fail: Filesystem error: %s\n"), e.what());
        }
        memset(fsdb, 0, UAEFSDB2_LEN);
        return 0;
    }
}


uae_u32 filesys_parse_mask(uae_u32 mask) {
    return mask ^ 0xf;
}

static a_inode* aino_from_buf(a_inode* base, uae_u8* buf, int* winmode) {
    uae_u32 mode;
    a_inode* aino = (a_inode*)calloc(1, sizeof(a_inode));
    if (!aino)
        return nullptr;

    uae_u8* buf2 = buf + 604;
    mode = do_get_mem_long((uae_u32*)(buf + 1));
    buf += 5;

    if (buf2[0]) {
        aino->aname = strdup((char*)buf2);
    } else {
        aino->aname = strdup((char*)buf);
    }
    buf += 257;
    buf2 += 257 * 2;

    if (buf2[0]) {
        aino->nname = build_nname(base->nname, (char*)buf2);
    } else {
        char* s = strdup((char*)buf);
        aino->nname = build_nname(base->nname, s);
        free(s);
    }
    buf += 257;

    aino->comment = *buf != '\0' ? strdup((char*)buf) : nullptr;
    buf += 81;

    aino->amigaos_mode = mode;
    *winmode = do_get_mem_long((uae_u32*)buf);
    aino->dir = ((*winmode) & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
    *winmode &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;
    aino->has_dbentry = 0;
    aino->dirty = 0;
    aino->db_offset = 0;

    try {
        std::filesystem::path filePath(aino->nname);
        if (!std::filesystem::exists(filePath)) {
            SDL_Log("File does not exist: %s", aino->nname);
            return aino;
        }
        aino->dir = std::filesystem::is_directory(filePath) ? 1 : 0;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("Error accessing file attributes for %s: %s", aino->nname, e.what());
    }

    return aino;
}


/* For an a_inode we have newly created based on a filename we found on the
 * native fs, fill in information about this file/directory.  */
int fsdb_fill_file_attrs(a_inode* base, a_inode* aino) {
    int winmode, oldamode;
    uae_u8 fsdb[UAEFSDB2_LEN];
    int reset = 0;

    try {
        std::filesystem::path filePath(aino->nname);
        if (!std::filesystem::exists(filePath)) {
            SDL_Log("File does not exist: %s", aino->nname);
            return 0;
        }

        aino->dir = std::filesystem::is_directory(filePath) ? 1 : 0;

        if (std::filesystem::is_symlink(filePath)) {
            aino->softlink = 1;
        }

        int perms = (int)std::filesystem::status(filePath).permissions();
        bool isHidden = filePath.filename().string().front() == '.';

        if ((base->volflags & MYVOLUMEINFO_STREAMS) && read_uaefsdb(aino->nname, nullptr, fsdb)) {
            aino->amigaos_mode = do_get_mem_long((uae_u32*)(fsdb + 1));
            free(aino->comment);
            aino->comment = nullptr;
            if (fsdb[5 + 2 * 257]) {
                aino->comment = strdup((char*)fsdb + 5 + 2 * 257);
            }
            free(aino_from_buf(base, fsdb, &winmode));
            if (winmode == perms) {
                return 1;
            }
            SDL_Log("FS: '%s' protection flags edited from external source\n", aino->nname);
            reset = 1;
        }

        oldamode = aino->amigaos_mode;
        aino->amigaos_mode = A_FIBF_EXECUTE | A_FIBF_READ;
        if (((int)perms & (int)std::filesystem::perms::owner_write) == 0) {
            aino->amigaos_mode |= A_FIBF_WRITE | A_FIBF_DELETE;
        }
        if (isHidden) {
            aino->amigaos_mode |= A_FIBF_HIDDEN;
        }
        aino->amigaos_mode = filesys_parse_mask(aino->amigaos_mode);
        aino->amigaos_mode |= oldamode & A_FIBF_SCRIPT;

        if (reset && (base->volflags & MYVOLUMEINFO_STREAMS)) {
            create_uaefsdb(aino, fsdb, (int)perms);
            write_uaefsdb(aino->nname, fsdb);
        }

        return 1;
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("Error filling file attributes for %s: %s", aino->nname, e.what());
        return 0;
    }
}

struct my_openfile_s {
    SDL_RWops* h = nullptr;
};

void my_close(struct my_openfile_s* mos) {
    if (!mos)
        return;
    if (mos->h) {
        SDL_RWclose(mos->h);
        mos->h = nullptr;
    }
    free(mos);
}

bool my_createshortcut(const char* /*src*/, const char* /*dst*/, const char* /*desc*/) {
    // Cross-platform shortcut creation not implemented
    return false;
}

bool my_isfilehidden(const char* path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path);
    return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_HIDDEN);
#else
    // On POSIX, hidden files start with '.'
    const char* filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;
    return filename[0] == '.';
#endif
}

int my_issamevolume(const char* path1, const char* path2, char* volume) {
    // Use SDL2 and std::filesystem for cross-platform support
    if (!path1 || !path2)
        return 0;

    // Normalize paths using SDL and std::filesystem
    std::string abs1, abs2;
    {
        char* base1 = SDL_GetBasePath();
        char* base2 = SDL_GetBasePath();
        try {
            abs1 = std::filesystem::absolute(path1).string();
            abs2 = std::filesystem::absolute(path2).string();
        } catch (...) {
            if (base1)
                SDL_free(base1);
            if (base2)
                SDL_free(base2);
            return 0;
        }
        if (base1)
            SDL_free(base1);
        if (base2)
            SDL_free(base2);
    }

#if defined(_WIN32)
    // On Windows, use the root drive letter as the volume
    char root1[MAX_PATH] = {0}, root2[MAX_PATH] = {0};
    if (!GetVolumePathNameA(abs1.c_str(), root1, MAX_PATH) || !GetVolumePathNameA(abs2.c_str(), root2, MAX_PATH))
        return 0;
    if (volume)
        strncpy(volume, root1, MAX_PATH - 1);
    return _stricmp(root1, root2) == 0;
#else
    // On POSIX, compare device IDs
    struct stat st1, st2;
    if (stat(abs1.c_str(), &st1) != 0 || stat(abs2.c_str(), &st2) != 0)
        return 0;
    if (volume)
        volume[0] = 0;
    return st1.st_dev == st2.st_dev;
#endif
}

uae_s64 my_lseek(struct my_openfile_s* mos, uae_s64 offset, int whence) {
    if (!mos || !mos->h)
        return -1;
    Sint64 result = SDL_RWseek(mos->h, offset, whence);
    return (result < 0) ? -1 : (uae_s64)result;
}

uae_s64 my_fsize(struct my_openfile_s* mos) {
    if (!mos || !mos->h)
        return -1;
    Sint64 size = SDL_RWsize(mos->h);
    return (size < 0) ? -1 : (uae_s64)size;
}


int my_mkdir(const TCHAR* name) {
    try {
        std::filesystem::path dirPath(name);
        if (std::filesystem::create_directory(dirPath)) {
            return 0;  // Success
        } else {
            return -1;  // Directory already exists or failed
        }
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_mkdir: Error creating directory %s: %s", name, e.what());
        return -1;  // Error
    }
}


struct my_openfile_s* my_open(const TCHAR* name, int flags) {
    struct my_openfile_s* mos = (struct my_openfile_s*)calloc(1, sizeof(struct my_openfile_s));
    if (!mos)
        return NULL;

    const char* mode = nullptr;
    if ((flags & O_RDWR) == O_RDWR) {
        if (flags & O_CREAT)
            mode = "w+b";
        else
            mode = "r+b";
    } else if (flags & O_WRONLY) {
        if (flags & O_CREAT)
            mode = "wb";
        else
            mode = "rb";  // fallback, but writing to read-only is not allowed
    } else {
        mode = "rb";
    }

    // SDL_RWFromFile expects UTF-8 on all platforms
    mos->h = SDL_RWFromFile(name, mode);

    if (!mos->h) {
        free(mos);
        return NULL;
    }
    return mos;
}


FILE* my_opentext(const char* name) {
    FILE* f;
    uae_u8 tmp[4];
    size_t v;

    f = _tfopen(name, _T("rb"));
    if (!f)
        return NULL;
    v = fread(tmp, 1, sizeof tmp, f);
    fclose(f);
    if (v == 4) {
        if (tmp[0] == 0xef && tmp[1] == 0xbb && tmp[2] == 0xbf)
            return _tfopen(name, _T("r, ccs=UTF-8"));
        if (tmp[0] == 0xff && tmp[1] == 0xfe)
            return _tfopen(name, _T("r, ccs=UTF-16LE"));
    }
    return _tfopen(name, _T("r"));
}

unsigned int my_read(struct my_openfile_s* mos, void* b, unsigned int size) {
    if (!mos || !mos->h || !b || size == 0)
        return 0;
    Sint64 result = SDL_RWread(mos->h, b, 1, size);
    if (result <= 0)
        return 0;
    return static_cast<unsigned int>(result);
}

unsigned int my_write(struct my_openfile_s* mos, void* b, unsigned int size) {
    if (!mos || !mos->h || !b || size == 0)
        return 0;
    Sint64 result = SDL_RWwrite(mos->h, b, 1, size);
    if (result <= 0)
        return 0;
    return static_cast<unsigned int>(result);
}

void my_setfilehidden(const char* path, bool hidden) {
    try {
        std::filesystem::path filePath(path);
        if (!std::filesystem::exists(filePath)) {
            SDL_Log("my_setfilehidden: File does not exist: %s", path);
            return;
        }

        std::string filename = filePath.filename().string();
        std::filesystem::path parentPath = filePath.parent_path();

        if (hidden && filename[0] != '.') {
            // Add a dot to make the file hidden
            std::filesystem::path newPath = parentPath / ("." + filename);
            std::filesystem::rename(filePath, newPath);
        } else if (!hidden && filename[0] == '.') {
            // Remove the dot to unhide the file
            std::filesystem::path newPath = parentPath / filename.substr(1);
            std::filesystem::rename(filePath, newPath);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        SDL_Log("my_setfilehidden: Error setting hidden attribute for %s: %s", path, e.what());
    }
}
