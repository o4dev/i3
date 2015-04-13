#undef I3__FILE__
#define I3__FILE__ "ewmh.c"
/*
 * vim:ts=4:sw=4:expandtab
 *
 * i3 - an improved dynamic tiling window manager
 * © 2009-2011 Michael Stapelberg and contributors (see also: LICENSE)
 *
 * ewmh.c: Get/set certain EWMH properties easily.
 *
 */
#include "all.h"

/*
 * Updates _NET_CURRENT_DESKTOP with the current desktop number.
 *
 * EWMH: The index of the current desktop. This is always an integer between 0
 * and _NET_NUMBER_OF_DESKTOPS - 1.
 *
 */
void ewmh_update_current_desktop(void) {
    Con *focused_ws = con_get_workspace(focused);
    Con *output;
    uint32_t idx = 0;
    /* We count to get the index of this workspace because named workspaces
     * don’t have the ->num property */
    TAILQ_FOREACH(output, &(croot->nodes_head), nodes) {
        Con *ws;
        TAILQ_FOREACH(ws, &(output_get_content(output)->nodes_head), nodes) {
            if (STARTS_WITH(ws->name, "__"))
                continue;

            if (ws == focused_ws) {
                xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root,
                                    A__NET_CURRENT_DESKTOP, XCB_ATOM_CARDINAL, 32, 1, &idx);
                return;
            }
            ++idx;
        }
    }
}

/*
 * Updates _NET_NUMBER_OF_DESKTOPS which we interpret as the number of
 * noninternal workspaces.
 */
void ewmh_update_number_of_desktops(void) {
    Con *output;
    uint32_t idx = 0;

    TAILQ_FOREACH(output, &(croot->nodes_head), nodes) {
        Con *ws;
        TAILQ_FOREACH(ws, &(output_get_content(output)->nodes_head), nodes) {
            if (STARTS_WITH(ws->name, "__"))
                continue;
            ++idx;
        }
    }

    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root,
                        A__NET_NUMBER_OF_DESKTOPS, XCB_ATOM_CARDINAL, 32, 1, &idx);
}

/*
 * Updates _NET_DESKTOP_NAMES: "The names of all virtual desktops. This is a
 * list of NULL-terminated strings in UTF-8 encoding"
 */
void ewmh_update_desktop_names(void) {
    Con *output;
    int msg_length = 0;

    /* count the size of the property message to set */
    TAILQ_FOREACH(output, &(croot->nodes_head), nodes) {
        Con *ws;
        TAILQ_FOREACH(ws, &(output_get_content(output)->nodes_head), nodes) {
            if (STARTS_WITH(ws->name, "__"))
                continue;
            msg_length += strlen(ws->name) + 1;
        }
    }

    char desktop_names[msg_length];
    int current_position = 0;

    /* fill the buffer with the names of the i3 workspaces */
    TAILQ_FOREACH(output, &(croot->nodes_head), nodes) {
        Con *ws;
        TAILQ_FOREACH(ws, &(output_get_content(output)->nodes_head), nodes) {
            if (STARTS_WITH(ws->name, "__"))
                continue;

            for (size_t i = 0; i < strlen(ws->name) + 1; i++) {
                desktop_names[current_position++] = ws->name[i];
            }
        }
    }

    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root,
                        A__NET_DESKTOP_NAMES, A_UTF8_STRING, 8, msg_length, desktop_names);
}

/*
 * Updates _NET_DESKTOP_VIEWPORT, which is an array of pairs of cardinals that
 * define the top left corner of each desktop's viewport.
 */
void ewmh_update_desktop_viewport(void) {
    Con *output;
    int num_desktops = 0;
    /* count number of desktops */
    TAILQ_FOREACH(output, &(croot->nodes_head), nodes) {
        Con *ws;
        TAILQ_FOREACH(ws, &(output_get_content(output)->nodes_head), nodes) {
            if (STARTS_WITH(ws->name, "__"))
                continue;

            num_desktops++;
        }
    }

    uint32_t viewports[num_desktops * 2];

    int current_position = 0;
    /* fill the viewport buffer */
    TAILQ_FOREACH(output, &(croot->nodes_head), nodes) {
        Con *ws;
        TAILQ_FOREACH(ws, &(output_get_content(output)->nodes_head), nodes) {
            if (STARTS_WITH(ws->name, "__"))
                continue;

            viewports[current_position++] = output->rect.x;
            viewports[current_position++] = output->rect.y;
        }
    }

    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root,
                        A__NET_DESKTOP_VIEWPORT, XCB_ATOM_CARDINAL, 32, current_position, &viewports);
}

/*
 * Updates _NET_ACTIVE_WINDOW with the currently focused window.
 *
 * EWMH: The window ID of the currently active window or None if no window has
 * the focus.
 *
 */
void ewmh_update_active_window(xcb_window_t window) {
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root,
                        A__NET_ACTIVE_WINDOW, XCB_ATOM_WINDOW, 32, 1, &window);
}

/*
 * i3 currently does not support _NET_WORKAREA, because it does not correspond
 * to i3’s concept of workspaces. See also:
 * http://bugs.i3wm.org/539
 * http://bugs.i3wm.org/301
 * http://bugs.i3wm.org/1038
 *
 * We need to actively delete this property because some display managers (e.g.
 * LightDM) set it.
 *
 * EWMH: Contains a geometry for each desktop. These geometries specify an area
 * that is completely contained within the viewport. Work area SHOULD be used by
 * desktop applications to place desktop icons appropriately.
 *
 */
void ewmh_update_workarea(void) {
    xcb_delete_property(conn, root, A__NET_WORKAREA);
}

/*
 * Updates the _NET_CLIENT_LIST hint.
 *
 */
void ewmh_update_client_list(xcb_window_t *list, int num_windows) {
    xcb_change_property(
        conn,
        XCB_PROP_MODE_REPLACE,
        root,
        A__NET_CLIENT_LIST,
        XCB_ATOM_WINDOW,
        32,
        num_windows,
        list);
}

/*
 * Updates the _NET_CLIENT_LIST_STACKING hint.
 *
 */
void ewmh_update_client_list_stacking(xcb_window_t *stack, int num_windows) {
    xcb_change_property(
        conn,
        XCB_PROP_MODE_REPLACE,
        root,
        A__NET_CLIENT_LIST_STACKING,
        XCB_ATOM_WINDOW,
        32,
        num_windows,
        stack);
}

/*
 * Set up the EWMH hints on the root window.
 *
 */
void ewmh_setup_hints(void) {
    xcb_atom_t supported_atoms[] = {
#define xmacro(atom) A_##atom,
#include "atoms.xmacro"
#undef xmacro
    };

    /* Set up the window manager’s name. According to EWMH, section "Root Window
     * Properties", to indicate that an EWMH-compliant window manager is
     * present, a child window has to be created (and kept alive as long as the
     * window manager is running) which has the _NET_SUPPORTING_WM_CHECK and
     * _NET_WM_ATOMS. */
    xcb_window_t child_window = xcb_generate_id(conn);
    xcb_create_window(
        conn,
        XCB_COPY_FROM_PARENT,        /* depth */
        child_window,                /* window id */
        root,                        /* parent */
        0, 0, 1, 1,                  /* dimensions (x, y, w, h) */
        0,                           /* border */
        XCB_WINDOW_CLASS_INPUT_ONLY, /* window class */
        XCB_COPY_FROM_PARENT,        /* visual */
        0,
        NULL);
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, child_window, A__NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW, 32, 1, &child_window);
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, child_window, A__NET_WM_NAME, A_UTF8_STRING, 8, strlen("i3"), "i3");
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root, A__NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW, 32, 1, &child_window);

    /* I’m not entirely sure if we need to keep _NET_WM_NAME on root. */
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root, A__NET_WM_NAME, A_UTF8_STRING, 8, strlen("i3"), "i3");

    /* only send the first 24 atoms (last one is _NET_CLOSE_WINDOW) increment that number when adding supported atoms */
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, root, A__NET_SUPPORTED, XCB_ATOM_ATOM, 32, 24, supported_atoms);
}
