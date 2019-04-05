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

#include "windowmanager.hpp"
#include "kernel.hpp"

namespace NumeRe
{

    //
    //
    // CLASS WINDOW
    //
    //

    // Default constructor. Initializes an invalid window
    Window::Window()
    {
        nType = WINDOW_NONE;
        m_graph = nullptr;
        m_manager = nullptr;
        nWindowID = string::npos;
    }

    // Copy constructor
    Window::Window(const Window& window) : Window()
    {
        assignWindow(window);
    }

    // Generic window constructor
    Window::Window(WindowType type, WindowManager* manager, const WindowSettings& settings) : Window()
    {
        nType = type;
        m_manager = manager;
        m_settings = settings;

        registerWindow();
    }

    // Graph window constructor
    Window::Window(WindowType type, WindowManager* manager, GraphHelper* graph) : Window()
    {
        nType = type;
        m_manager = manager;
        m_graph = graph;

        registerWindow();
    }

    // Destructor. Unregisters the current window
    // in the window manager
    Window::~Window()
    {
        unregisterWindow();
    }

    // This private member function registers the
    // current window in the window manager. This
    // will automatically take the ownership from
    // a copied window
    void Window::registerWindow()
    {
        if (m_manager)
            nWindowID = m_manager->registerWindow(this, nWindowID);
    }

    // This private member function will unregister
    // the current window in the window manager
    void Window::unregisterWindow()
    {
        if (m_manager)
            m_manager->unregisterWindow(this, nWindowID);
    }

    // This private member function is the generalisation
    // of the assignment operator and the copy constructor
    void Window::assignWindow(const Window& window)
    {
        // Ensure that the current window is
        // not already registered to avoid
        // orphan windows in the window manager
        unregisterWindow();

        // Copy the passed windows contents
        nType = window.nType;
        m_graph = window.m_graph;
        m_manager = window.m_manager;
        m_settings = window.m_settings;
        nWindowID = window.nWindowID;

        // Register this new window
        registerWindow();
    }

    // This member function is the public overload of
    // the assignment operator
    Window& Window::operator=(const Window& window)
    {
        assignWindow(window);

        return *this;
    }

    // This public member function can be used to
    // update the stored window information in the
    // window manager for the current window
    void Window::updateWindowInformation(int status, const string& _return)
    {
        if (m_manager)
        {
            m_manager->updateWindowInformation(WindowInformation(nWindowID, this, status, _return));
        }
    }

    // This private member function is used by the
    // window manager to detach itself from this
    // window to avoid segmentation faults at
    // application shutdown. This is needed, because
    // wxWidgets will not automatically destruct
    // windows after they are closed (rather they
    // are closed in some idle time)
    void Window::detach()
    {
        m_manager = nullptr;
    }

    //
    //
    // CLASS WINDOWMANAGER
    //
    //

    // This function is used by the registered windows to
    // inform the window manager about new window statuses
    // or return values
    void WindowManager::updateWindowInformation(const WindowInformation& information)
    {
        if (m_windowMap.find(information.nWindowID) != m_windowMap.end())
            m_windowMap[information.nWindowID] = information;
    }

    // This member function registers a new window or changes
    // the registration to a new window
    size_t WindowManager::registerWindow(Window* window, size_t id)
    {
        if (id == string::npos)
            id = m_windowMap.size();

        m_windowMap[id] = WindowInformation(id, window, 0, "");

        return id;
    }

    // This member function will unregister a window. This is done
    // only if the ID and the window pointer are both matching
    // the internal mapping
    void WindowManager::unregisterWindow(Window* window, size_t id)
    {
        // Search for the passed window ID
        auto iter = m_windowMap.find(id);

        // If the ID exists and the pointer is equal
        if (iter != m_windowMap.end() && iter->second.window == window)
        {
            // If the window type is a graph window
            // delete it directly - it's not necessary
            // to keep it in memory
            if (window->getType() == WINDOW_GRAPH)
                m_windowMap.erase(iter);
            else if (iter->second.nStatus == STATUS_RUNNING)
            {
                iter->second.nStatus = STATUS_CANCEL;
                iter->second.window = nullptr;
            }
        }
    }

    // Empty window manager constructor
    WindowManager::WindowManager()
    {
        //
    }

    // Destructor. It will detach all registered
    // windows from the manager. This is done to
    // avoid segmentation faults during application
    // shut down
    WindowManager::~WindowManager()
    {
        for (auto iter = m_windowMap.begin(); iter != m_windowMap.end(); ++iter)
        {
            if (iter->second.window)
            {
                iter->second.window->detach();
                iter->second.nStatus = STATUS_CANCEL;
            }
        }
    }

    // This public member function will create a
    // window object containing the passed graph
    size_t WindowManager::createWindow(GraphHelper* graph)
    {
        Window window(WINDOW_GRAPH, this, graph);

        NumeReKernel::getInstance()->showWindow(window);

        return window.getId();
    }

    // This public member function will create an
    // arbitrary window, which is described by the
    // passed WindowSettings object
    size_t WindowManager::createWindow(WindowType type, const WindowSettings& settings)
    {
        Window window(type, this, settings);

        NumeReKernel::getInstance()->showWindow(window);

        return window.getId();
    }

    // This public member function will return the
    // window information stored in the internal
    // map. If the window was closed by the user
    // and this was noted in the window information
    // then the object is deleted from the internal
    // map
    WindowInformation WindowManager::getWindowInformation(size_t windowId)
    {
        // Search for the passed window ID
        auto iter = m_windowMap.find(windowId);

        if (iter != m_windowMap.end())
        {
            // Get the window information
            WindowInformation winInfo = iter->second;

            // Erase the window from the map
            // if it has been closed
            if (winInfo.nStatus)
            {
                m_windowMap.erase(iter);
            }

            return winInfo;
        }

        return WindowInformation();
    }

    // This public member function will return the
    // window information stored in the internal
    // map. The access is modal, i.e. it's checked
    // repeatedly, whether a window has been closed
    // until the information is returned. The object
    // is then deleted from the internal map
    WindowInformation WindowManager::getWindowInformationModal(size_t windowId)
    {
        WindowInformation winInfo;

        // Repeatedly check for the closing of
        // the window
        do
        {
            Sleep(100);

            // Search for the passed window ID
            auto iter = m_windowMap.find(windowId);

            if (iter != m_windowMap.end())
            {
                // Get the window information
                winInfo = iter->second;

                // Erase the window from the map
                // if it has been closed
                if (winInfo.nStatus != STATUS_RUNNING)
                {
                    m_windowMap.erase(iter);
                    return winInfo;
                }
            }
            else
                break;

        }
        while (winInfo.nStatus == STATUS_RUNNING);

        // Return an empty window information
        return winInfo;
    }



}


