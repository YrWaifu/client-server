#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

void set_clipboard(Display* display, Window window, const std::string& content, Atom format) {
    // Получаем атомы для PRIMARY и CLIPBOARD
    Atom clipboard_atom = XInternAtom(display, "CLIPBOARD", 0);

    XSetSelectionOwner(display, XA_PRIMARY, window, CurrentTime);
    XSetSelectionOwner(display, clipboard_atom, window, CurrentTime);

    if (XGetSelectionOwner(display, XA_PRIMARY) != window ||
        XGetSelectionOwner(display, clipboard_atom) != window) {
        std::cerr << "Failed to set selection owner" << std::endl;
        return;
    }

    Atom targets_atom = XInternAtom(display, "TARGETS", 0);
    std::map<Atom, std::string> atom_data;

    // Создание атомов для различных форматов изображений
    Atom image_jpeg = XInternAtom(display, "image/jpeg", 0);
    Atom image_png = XInternAtom(display, "image/png", 0);
    Atom image_webp = XInternAtom(display, "image/webp", 0);
    Atom image_bmp = XInternAtom(display, "image/bmp", 0);
    Atom image_icon = XInternAtom(display, "image/icon", 0);

    // Заполнение atom_data для каждого формата изображения
    atom_data[image_jpeg] = content;
    atom_data[image_png] = content;
    atom_data[image_webp] = content;
    atom_data[image_bmp] = content;
    atom_data[image_icon] = content;

    // Цикл обработки запросов на выбор
    XEvent event;
    while (true) {
        XNextEvent(display, &event);
        if (event.type == SelectionRequest) {
            XSelectionRequestEvent* req = &event.xselectionrequest;
            XSelectionEvent ev = {0};
            ev.type = SelectionNotify;
            ev.display = req->display;
            ev.requestor = req->requestor;
            ev.selection = req->selection;
            ev.target = req->target;
            ev.time = req->time;
            ev.property = None;

            if (req->target == targets_atom) {
                std::vector<Atom> targets;
                for (const auto& pair : atom_data) {
                    targets.push_back(pair.first);
                }
                XChangeProperty(display, req->requestor, req->property, XA_ATOM, 32, PropModeReplace,
                                reinterpret_cast<unsigned char*>(targets.data()), targets.size());
                ev.property = req->property;
            } else if (atom_data.find(req->target) != atom_data.end()) {
                const std::string& data = atom_data[req->target];
                XChangeProperty(display, req->requestor, req->property, req->target, 8, PropModeReplace,
                                reinterpret_cast<const unsigned char*>(data.c_str()), data.size());
                ev.property = req->property;
            }

            XSendEvent(display, req->requestor, True, 0, reinterpret_cast<XEvent*>(&ev));
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image file>" << std::endl;
        return 1;
    }

    std::ifstream image_file(argv[1], std::ios::binary);
    if (!image_file) {
        std::cerr << "Failed to open image file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << image_file.rdbuf();
    std::string image_content = buffer.str();

    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Unable to open X display" << std::endl;
        return 1;
    }

    Window window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 1, 1, 0, 0, 0);
    if (!window) {
        std::cerr << "Unable to create X window" << std::endl;
        XCloseDisplay(display);
        return 1;
    }

    set_clipboard(display, window, image_content, XA_STRING);

    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
