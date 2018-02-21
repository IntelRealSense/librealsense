/* noc_file_dialog library
 *
 * Copyright (c) 2015 Guillaume Chereau <guillaume@noctua-software.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* A portable library to create open and save dialogs on linux, osx and
 * windows.
 *
 * The library define a single function : noc_file_dialog_open.
 * With three different implementations.
 *
 * Usage:
 *
 * The library does not automatically select the implementation, you need to
 * define one of those macros before including this file:
 *
 *  NOC_FILE_DIALOG_GTK
 *  NOC_FILE_DIALOG_WIN32
 *  NOC_FILE_DIALOG_OSX
 */

enum {
    NOC_FILE_DIALOG_OPEN    = 1 << 0,   // Create an open file dialog.
    NOC_FILE_DIALOG_SAVE    = 1 << 1,   // Create a save file dialog.
    NOC_FILE_DIALOG_DIR     = 1 << 2,   // Open a directory.
    NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION = 1 << 3,
};

// There is a single function defined.

/* flags            : union of the NOC_FILE_DIALOG_XXX masks.
 * filters          : a list of strings separated by '\0' of the form:
 *                      "name1 reg1 name2 reg2 ..."
 *                    The last value is followed by two '\0'.  For example,
 *                    to filter png and jpeg files, you can use:
 *                      "png\0*.png\0jpeg\0*.jpeg\0"
 *                    You can also separate patterns with ';':
 *                      "jpeg\0*.jpg;*.jpeg\0"
 *                    Set to NULL for no filter.
 * default_path     : the default file to use or NULL.
 * default_name     : the default file name to use or NULL.
 *
 * The function return a C string.  There is no need to free it, as it is
 * managed by the library.  The string is valid until the next call to
 * no_dialog_open.  If the user canceled, the return value is NULL.
 */
const char *noc_file_dialog_open(int flags,
                                 const char *filters,
                                 const char *default_path,
                                 const char *default_name);

#ifdef NOC_FILE_DIALOG_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

static char *g_noc_file_dialog_ret = NULL;

#ifdef NOC_FILE_DIALOG_GTK

#include <gtk/gtk.h>

const char *noc_file_dialog_open(int flags,
                                 const char *filters,
                                 const char *default_path,
                                 const char *default_name)
{
    GtkWidget *dialog;
    GtkFileFilter *filter;
    GtkFileChooser *chooser;
    GtkFileChooserAction action;
    gint res;
    char buf[128], *patterns;

    action = flags & NOC_FILE_DIALOG_SAVE ? GTK_FILE_CHOOSER_ACTION_SAVE :
                                            GTK_FILE_CHOOSER_ACTION_OPEN;
    if (flags & NOC_FILE_DIALOG_DIR)
        action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;

    gtk_init_check(NULL, NULL);
    dialog = gtk_file_chooser_dialog_new(
            flags & NOC_FILE_DIALOG_SAVE ? "Save File" : "Open File",
            NULL,
            action,
            "_Cancel", GTK_RESPONSE_CANCEL,
            flags & NOC_FILE_DIALOG_SAVE ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT,
            NULL );
    chooser = GTK_FILE_CHOOSER(dialog);
    if (flags & NOC_FILE_DIALOG_OVERWRITE_CONFIRMATION)
        gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

    if (default_path)
        gtk_file_chooser_set_filename(chooser, default_path);
    if (default_name)
        gtk_file_chooser_set_current_name(chooser, default_name);

    while (filters && *filters) {
        filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, filters);
        filters += strlen(filters) + 1;

        // Split the filter pattern with ';'.
        strcpy(buf, filters);
        buf[strlen(buf)] = '\0';
        for (patterns = buf; *patterns; patterns++)
            if (*patterns == ';') *patterns = '\0';
        patterns = buf;
        while (*patterns) {
            gtk_file_filter_add_pattern(filter, patterns);
            patterns += strlen(patterns) + 1;
        }

        gtk_file_chooser_add_filter(chooser, filter);
        filters += strlen(filters) + 1;
    }

    res = gtk_dialog_run(GTK_DIALOG(dialog));

    free(g_noc_file_dialog_ret);
    g_noc_file_dialog_ret = NULL;

    if (res == GTK_RESPONSE_ACCEPT)
        g_noc_file_dialog_ret = gtk_file_chooser_get_filename(chooser);
    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();
    return g_noc_file_dialog_ret;
}

#endif

#ifdef NOC_FILE_DIALOG_WIN32

#include <windows.h>
#include <commdlg.h>

const char *noc_file_dialog_open(int flags,
                                 const char *filters,
                                 const char *default_path,
                                 const char *default_name)
{
    OPENFILENAMEA ofn;       // common dialog box structure
    char szFile[260];       // buffer for file name
    int ret;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filters;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (flags & NOC_FILE_DIALOG_OPEN)
        ret = GetOpenFileNameA(&ofn);
    else
        ret = GetSaveFileNameA(&ofn);

    free(g_noc_file_dialog_ret);
    g_noc_file_dialog_ret = ret ? _strdup(szFile) : NULL;
    return g_noc_file_dialog_ret;
}

#endif

#ifdef NOC_FILE_DIALOG_OSX

#ifdef __OBJC__
#include <AppKit/AppKit.h>

const char *noc_file_dialog_open(int flags,
                                 const char *filters,
                                 const char *default_path,
                                 const char *default_name)
{
    NSURL *url;
    const char *utf8_path;
    NSSavePanel *panel;
    NSOpenPanel *open_panel;
    NSMutableArray *types_array;
    NSURL *default_url;
    char buf[128], *patterns;
    // XXX: I don't know about memory management with cococa, need to check
    // if I leak memory here.
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if (flags & NOC_FILE_DIALOG_OPEN) {
        panel = open_panel = [NSOpenPanel openPanel];
    } else {
        panel = [NSSavePanel savePanel];
    }

    if (flags & NOC_FILE_DIALOG_DIR) {
        [open_panel setCanChooseDirectories:YES];
        [open_panel setCanChooseFiles:NO];
    }

    if (default_path) {
        default_url = [NSURL fileURLWithPath:
            [NSString stringWithUTF8String:default_path]];
        [panel setDirectoryURL:default_url];
        [panel setNameFieldStringValue:default_url.lastPathComponent];
    }

    if (filters) {
        types_array = [NSMutableArray array];
        while (*filters) {
            filters += strlen(filters) + 1; // skip the name
            // Split the filter pattern with ';'.
            strcpy(buf, filters);
            buf[strlen(buf) + 1] = '\0';
            for (patterns = buf; *patterns; patterns++)
                if (*patterns == ';') *patterns = '\0';
            patterns = buf;
            while (*patterns) {
                assert(strncmp(patterns, "*.", 2) == 0);
                patterns += 2; // Skip the "*."
                [types_array addObject:[NSString stringWithUTF8String: patterns]];
                patterns += strlen(patterns) + 1;
            }
            filters += strlen(filters) + 1;
        }
        [panel setAllowedFileTypes:types_array];
    }

    free(g_noc_file_dialog_ret);
    g_noc_file_dialog_ret = NULL;
    if ( [panel runModal] == NSModalResponseOK ) {
        url = [panel URL];
        utf8_path = [[url path] UTF8String];
        g_noc_file_dialog_ret = strdup(utf8_path);
    }

    [pool release];
    return g_noc_file_dialog_ret;
}	

#else

// Patch until proper solution is found

const char *noc_file_dialog_open(int flags,
                                 const char *filters,
                                 const char *default_path,
                                 const char *default_name)
{
    return nullptr;
}

#endif

#endif

#endif
