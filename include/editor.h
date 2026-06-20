#ifndef EDITOR_H
#define EDITOR_H

/* Opens a simple full-screen text editor for the given file.
   Creates the file if it doesn't exist. Returns when the user
   saves and exits (Ctrl+S to save, Ctrl+Q or ESC to quit). */
void editor_open(const char* filename);

#endif
