#include "fs.h"
#include "vga.h"

/* ============================================
   IN-MEMORY FILESYSTEM IMPLEMENTATION
   ============================================ */

static fs_file_t files[FS_MAX_FILES];
static fs_dir_t  dirs[FS_MAX_DIRS];
int fs_current_dir = 0;

/* ---- Tiny string helpers ---- */
static int fs_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static void fs_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* ---- Initialise: create root directory and some starter files ---- */
void fs_init(void) {
    /* Clear everything */
    for (int i = 0; i < FS_MAX_FILES; i++) files[i].used = 0;
    for (int i = 0; i < FS_MAX_DIRS;  i++) dirs[i].used  = 0;

    /* Create root directory */
    fs_strcpy(dirs[0].name, "/", FS_NAME_LEN);
    dirs[0].used   = 1;
    dirs[0].parent = 0;
    fs_current_dir = 0;

    /* Create a couple of starter dirs */
    fs_mkdir("docs");
    fs_mkdir("src");

    /* Create a welcome file */
    fs_create("readme.txt",
        "Welcome to Claudimon!\n"
        "This is your in-memory filesystem.\n"
        "Files are lost on reboot for now.\n"
        "Use 'help' to see all commands.\n");

    /* Go back to root after setup */
    fs_current_dir = 0;
}

/* ---- Create a directory in the current directory ---- */
int fs_mkdir(const char* name) {
    /* Check for duplicate */
    for (int i = 0; i < FS_MAX_DIRS; i++) {
        if (dirs[i].used && dirs[i].parent == fs_current_dir
            && fs_strcmp(dirs[i].name, name) == 0) {
            terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            terminal_write("mkdir: directory already exists: ");
            terminal_write(name);
            terminal_write("\n");
            return -1;
        }
    }
    for (int i = 0; i < FS_MAX_DIRS; i++) {
        if (!dirs[i].used) {
            fs_strcpy(dirs[i].name, name, FS_NAME_LEN);
            dirs[i].used   = 1;
            dirs[i].parent = fs_current_dir;
            terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            terminal_write("mkdir: created '");
            terminal_write(name);
            terminal_write("'\n");
            return i;
        }
    }
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_write("mkdir: no space left\n");
    return -1;
}

/* ---- Change directory ---- */
int fs_cd(const char* name) {
    if (fs_strcmp(name, "..") == 0) {
        if (fs_current_dir != 0)
            fs_current_dir = dirs[fs_current_dir].parent;
        return 0;
    }
    if (fs_strcmp(name, "/") == 0) {
        fs_current_dir = 0;
        return 0;
    }
    for (int i = 0; i < FS_MAX_DIRS; i++) {
        if (dirs[i].used && dirs[i].parent == fs_current_dir
            && fs_strcmp(dirs[i].name, name) == 0) {
            fs_current_dir = i;
            return 0;
        }
    }
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_write("cd: no such directory: ");
    terminal_write(name);
    terminal_write("\n");
    return -1;
}

/* ---- Print working directory ---- */
void fs_pwd(void) {
    /* Walk up to root building path */
    char path[256];
    int  parts[FS_MAX_DIRS];
    int  depth = 0;
    int  cur   = fs_current_dir;

    while (cur != 0) {
        parts[depth++] = cur;
        cur = dirs[cur].parent;
    }

    if (depth == 0) {
        terminal_write("/\n");
        return;
    }

    for (int i = depth - 1; i >= 0; i--) {
        terminal_write("/");
        terminal_write(dirs[parts[i]].name);
    }
    terminal_write("\n");
}

/* ---- List current directory ---- */
int fs_ls(void) {
    int found = 0;

    /* List subdirectories */
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    for (int i = 0; i < FS_MAX_DIRS; i++) {
        if (dirs[i].used && dirs[i].parent == fs_current_dir && i != 0) {
            terminal_write("[DIR]  ");
            terminal_write(dirs[i].name);
            terminal_write("\n");
            found++;
        }
    }

    /* List files */
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && files[i].dir_index == fs_current_dir) {
            terminal_write("[FILE] ");
            terminal_write(files[i].name);
            terminal_write("\n");
            found++;
        }
    }

    if (!found) {
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_write("(empty directory)\n");
    }
    return 0;
}

/* ---- Create a file in the current directory ---- */
int fs_create(const char* name, const char* data) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) {
            fs_strcpy(files[i].name, name, FS_NAME_LEN);
            int j = 0;
            while (data[j] && j < FS_MAX_FILE_SIZE - 1) {
                files[i].data[j] = data[j]; j++;
            }
            files[i].data[j]  = '\0';
            files[i].size      = j;
            files[i].used      = 1;
            files[i].dir_index = fs_current_dir;
            return i;
        }
    }
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_write("fs: no space for new file\n");
    return -1;
}

/* ---- Print a file's contents ---- */
int fs_cat(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && files[i].dir_index == fs_current_dir
            && fs_strcmp(files[i].name, name) == 0) {
            terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            terminal_write(files[i].data);
            return 0;
        }
    }
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_write("cat: file not found: ");
    terminal_write(name);
    terminal_write("\n");
    return -1;
}
