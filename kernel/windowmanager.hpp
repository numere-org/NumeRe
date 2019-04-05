/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#ifndef WINDOWMANAGER_HPP
#define WINDOWMANAGER_HPP

#include <map>
#include <string>
#include "core/plotting/graph_helper.hpp"

using namespace std;

namespace NumeRe
{
    // Enumeration for the window type
    enum WindowType
    {
        WINDOW_NONE,
        WINDOW_NON_MODAL,
        WINDOW_MODAL,
        WINDOW_GRAPH,
        WINDOW_LAST
    };


    // Enumeration for the contained
    // window controlss
    enum WindowControls
    {
        CTRL_NONE = 0x0,
        // Button selections
        CTRL_OKBUTTON = 0x1,
        CTRL_CANCELBUTTON = 0x2,
        CTRL_YESNOBUTTON = 0x4,
        // Icon selection
        CTRL_ICONINFORMATION = 0x10,
        CTRL_ICONQUESTION = 0x20,
        CTRL_ICONWARNING = 0x40,
        CTRL_ICONERROR = 0x80,
        // Dialog types
        CTRL_MESSAGEBOX = 0x400,
        CTRL_FILEDIALOG = 0x800,
        CTRL_FOLDERDIALOG = 0x1000,
        CTRL_LISTDIALOG = 0x2000,
        CTRL_SELECTIONDIALOG = 0x4000,
        CTRL_TEXTENTRY = 0x8000
    };


    // Enumeration for the status of
    // a displayed dialog
    enum WindowStatus
    {
        STATUS_RUNNING = 0x0,
        STATUS_CANCEL = 0x1,
        STATUS_OK = 0x2
    };


    // This class contains the window information
    // to create the dialog in the GUI
    struct WindowSettings
    {
        int nControls;
        bool isModal;
        string sMessage;
        string sExpression;
        string sTitle;

        WindowSettings(int ctrls = CTRL_NONE, bool modal = false, const string& message = "", const string& title = "NumeRe: Window", const string& expression = "") : nControls(ctrls), isModal(modal), sMessage(message), sExpression(expression), sTitle(title) {}
    };


    // Forward declaration of the window manager class
    class WindowManager;


    // This class represents an abstract window handled
    // by the window manager
    class Window
    {
        private:
            friend class WindowManager;
            WindowType nType;
            GraphHelper* m_graph;
            WindowManager* m_manager;
            WindowSettings m_settings;
            size_t nWindowID;

            void registerWindow();
            void unregisterWindow();
            void assignWindow(const Window& window);
            void detach();

        public:
            Window();
            Window(const Window& window);
            Window(WindowType type, WindowManager* manager, const WindowSettings& settings);
            Window(WindowType type, WindowManager* manager, GraphHelper* graph);
            ~Window();

            Window& operator=(const Window& window);

            WindowType getType() const
                {
                    return nType;
                }

            GraphHelper* getGraph()
                {
                    return m_graph;
                }

            const WindowSettings& getWindowSettings() const
                {
                    return m_settings;
                }

            size_t getId() const
                {
                    return nWindowID;
                }

            void updateWindowInformation(int status, const string& _return);
    };


    // This class is used by the window manager to handle
    // the information of an opened window and to store
    // its return value
    struct WindowInformation
    {
        size_t nWindowID;
        Window* window;
        int nStatus;
        string sReturn;

        WindowInformation(size_t id = string::npos, Window* win = nullptr, int status = STATUS_RUNNING, const string& _return = "")
                            : nWindowID(id), window(win), nStatus(status), sReturn(_return) {}
    };


    // This is the window manager of the kernel. All
    // windows opened by the kernel will be registered
    // here.
    class WindowManager
    {
        private:
            friend class Window;

            map<size_t, WindowInformation> m_windowMap;

            void updateWindowInformation(const WindowInformation& information);

            size_t registerWindow(Window* window, size_t id);
            void unregisterWindow(Window* window, size_t id);

        public:
            WindowManager();
            ~WindowManager();

            size_t createWindow(GraphHelper* graph);
            size_t createWindow(WindowType type, const WindowSettings& settings);

            WindowInformation getWindowInformation(size_t windowId);
            WindowInformation getWindowInformationModal(size_t windowId);
    };

}

#endif // WINDOWMANAGER_HPP

