#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TARGETS_ATOM_NAME "TARGETS"
#define UTF8_STRING_ATOM_NAME "UTF8_STRING"
#define FILE_URI_ATOM_NAME "text/uri-list"
#define GNOME_COPIED_FILES_ATOM_NAME "x-special/gnome-copied-files"
#define MATE_COPIED_FILES_ATOM_NAME "x-special/mate-copied-files"

void set_clipboard(Display *dpy, Window win, Atom selection, const char *filename) {
    Atom targets_atom = XInternAtom(dpy, TARGETS_ATOM_NAME, False);
    Atom utf8_string_atom = XInternAtom(dpy, UTF8_STRING_ATOM_NAME, False);
    Atom file_uri_atom = XInternAtom(dpy, FILE_URI_ATOM_NAME, False);
    Atom gnome_copied_files_atom = XInternAtom(dpy, GNOME_COPIED_FILES_ATOM_NAME, False);
    Atom mate_copied_files_atom = XInternAtom(dpy, MATE_COPIED_FILES_ATOM_NAME, False);

    XEvent evt;
    while (1) {
        XNextEvent(dpy, &evt);
        if (evt.type == SelectionRequest) {
            XSelectionRequestEvent *req = &evt.xselectionrequest;
            XSelectionEvent ev = {0};
            ev.type = SelectionNotify;
            ev.display = req->display;
            ev.requestor = req->requestor;
            ev.selection = req->selection;
            ev.target = req->target;
            ev.time = req->time;
            ev.property = None;

            if (req->target == targets_atom) {
                Atom targets[] = {targets_atom, utf8_string_atom, file_uri_atom, gnome_copied_files_atom,
                                  mate_copied_files_atom};
                XChangeProperty(dpy, req->requestor, req->property, XA_ATOM, 32, PropModeReplace,
                                (unsigned char *)targets, 5);
                ev.property = req->property;
            } else if (req->target == file_uri_atom) {
                char uri[2048];
                snprintf(uri, sizeof(uri), "file://%s", filename);
                XChangeProperty(dpy, req->requestor, req->property, file_uri_atom, 8, PropModeReplace,
                                (unsigned char *)uri, strlen(uri));
                ev.property = req->property;
            } else if (req->target == utf8_string_atom) {
                XChangeProperty(dpy, req->requestor, req->property, utf8_string_atom, 8, PropModeReplace,
                                (unsigned char *)filename, strlen(filename));
                ev.property = req->property;
            } else if (req->target == gnome_copied_files_atom || req->target == mate_copied_files_atom) {
                char copied_files[2048];
                snprintf(copied_files, sizeof(copied_files), "copy\nfile://%s", filename);
                XChangeProperty(dpy, req->requestor, req->property, req->target, 8, PropModeReplace,
                                (unsigned char *)copied_files, strlen(copied_files));
                ev.property = req->property;
            }

            if (ev.property == None) {
                ev.property = req->property;
            }
            XSendEvent(dpy, req->requestor, False, 0, (XEvent *)&ev);
            XFlush(dpy);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Unable to open display\n");
        return 1;
    }

    Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0, 0);

    Atom clipboard = XInternAtom(dpy, "CLIPBOARD", False);

    XSetSelectionOwner(dpy, clipboard, win, CurrentTime);
    if (XGetSelectionOwner(dpy, clipboard) != win) {
        fprintf(stderr, "Unable to set clipboard owner\n");
        return 1;
    }

    printf("File '%s' is now in clipboard. Press Ctrl+V to paste it.\n", filename);

    set_clipboard(dpy, win, clipboard, filename);

    XCloseDisplay(dpy);
    return 0;
}
