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

#include "../gui/compositions/customwindow.hpp"
#include "kernel.hpp"
#include "core/utils/tinyxml2.h"
#include "core/plotting/graph_helper.hpp"
#include "windowmanager.hpp"

namespace NumeRe
{

    //
    //
    // CLASS WINDOW
    //
    //

    /////////////////////////////////////////////////
    /// \brief Default constructor. Initializes an
    /// invalid window.
    /////////////////////////////////////////////////
    Window::Window()
    {
        nType = WINDOW_NONE;
        m_graph = nullptr;
        m_manager = nullptr;
        m_customWindow = nullptr;
        nWindowID = std::string::npos;
        m_layout = nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief Copy constructor.
    ///
    /// \param window const Window&
    ///
    /////////////////////////////////////////////////
    Window::Window(const Window& window) : Window()
    {
        assignWindow(window);
    }


    /////////////////////////////////////////////////
    /// \brief Generic window constructor.
    ///
    /// \param type WindowType
    /// \param manager WindowManager*
    /// \param settings const WindowSettings&
    ///
    /////////////////////////////////////////////////
    Window::Window(WindowType type, WindowManager* manager, const WindowSettings& settings) : Window()
    {
        nType = type;
        m_manager = manager;
        m_settings = settings;

        registerWindow();
    }


    /////////////////////////////////////////////////
    /// \brief Graph window constructor.
    ///
    /// \param type WindowType
    /// \param manager WindowManager*
    /// \param graph GraphHelper*
    ///
    /////////////////////////////////////////////////
    Window::Window(WindowType type, WindowManager* manager, GraphHelper* graph) : Window()
    {
        nType = type;
        m_manager = manager;
        m_graph = graph;

        registerWindow();
    }


    /////////////////////////////////////////////////
    /// \brief Custom window constructor.
    ///
    /// \param type WindowType
    /// \param manager WindowManager*
    /// \param layoutString tinyxml2::XMLDocument*
    ///
    /////////////////////////////////////////////////
    Window::Window(WindowType type, WindowManager* manager, tinyxml2::XMLDocument* layoutString) : Window()
    {
        nType = type;
        m_manager = manager;
        m_layout = layoutString;

        registerWindow();
    }


    /////////////////////////////////////////////////
    /// \brief Destructor. Unregisters the current
    /// window in the window manager and deletes the
    /// layout pointer, if it is non-zero.
    /////////////////////////////////////////////////
    Window::~Window()
    {
        unregisterWindow();

        if (m_layout)
            delete m_layout;
    }


    /////////////////////////////////////////////////
    /// \brief This private member function registers
    /// the current window in the window manager.
    /// This will automatically take the ownership
    /// from a copied window.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Window::registerWindow()
    {
        if (m_manager)
            nWindowID = m_manager->registerWindow(this, nWindowID);
    }


    /////////////////////////////////////////////////
    /// \brief This private member function will
    /// unregister the current window in the window
    /// manager.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Window::unregisterWindow()
    {
        if (m_manager)
            m_manager->unregisterWindow(this, nWindowID);
    }


    /////////////////////////////////////////////////
    /// \brief This private member function is the
    /// generalisation of the assignment operator and
    /// the copy constructor.
    ///
    /// \param window const Window&
    /// \return void
    ///
    /////////////////////////////////////////////////
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
        m_layout = window.m_layout;
        nWindowID = window.nWindowID;
        window.m_layout = nullptr;

        // Register this new window (this will automatically
        // remove the copied window from the register)
        registerWindow();
    }


    /////////////////////////////////////////////////
    /// \brief This member function is the public
    /// overload of the assignment operator.
    ///
    /// \param window const Window&
    /// \return Window&
    ///
    /////////////////////////////////////////////////
    Window& Window::operator=(const Window& window)
    {
        assignWindow(window);

        return *this;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function can be
    /// used to update the stored window information
    /// in the window manager for the current window.
    ///
    /// \param status int
    /// \param _return const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void Window::updateWindowInformation(int status, const std::string& _return)
    {
        if (m_manager)
            m_manager->updateWindowInformation(WindowInformation(nWindowID, status == STATUS_RUNNING ? this : nullptr, status, _return));
    }


    /////////////////////////////////////////////////
    /// \brief Returns a vector of item ids
    /// corresponding to the available window items
    /// of this window.
    ///
    /// \param _selection const std::string&
    /// \return std::vector<int>
    ///
    /////////////////////////////////////////////////
    std::vector<int> Window::getWindowItems(const std::string& _selection) const
    {
        if (m_customWindow)
        {
            if (_selection == "checkbox")
                return m_customWindow->getWindowItems(CustomWindow::CHECKBOX);
            else if (_selection == "button")
                return m_customWindow->getWindowItems(CustomWindow::BUTTON);
            else if (_selection == "textfield")
                return m_customWindow->getWindowItems(CustomWindow::TEXTCTRL);
            else if (_selection == "dropdown")
                return m_customWindow->getWindowItems(CustomWindow::DROPDOWN);
            else if (_selection == "combobox")
                return m_customWindow->getWindowItems(CustomWindow::COMBOBOX);
            else if (_selection == "radio")
                return m_customWindow->getWindowItems(CustomWindow::RADIOGROUP);
            else if (_selection == "bitmap")
                return m_customWindow->getWindowItems(CustomWindow::IMAGE);
            else if (_selection == "tablegrid")
                return m_customWindow->getWindowItems(CustomWindow::TABLE);
            else if (_selection == "gauge")
                return m_customWindow->getWindowItems(CustomWindow::GAUGE);
            else if (_selection == "spinbut")
                return m_customWindow->getWindowItems(CustomWindow::SPINCTRL);
            else if (_selection == "statictext" || _selection == "text")
                return m_customWindow->getWindowItems(CustomWindow::TEXT);
            else if (_selection == "grapher")
                return m_customWindow->getWindowItems(CustomWindow::GRAPHER);
            else if (_selection == "treelist")
                return m_customWindow->getWindowItems(CustomWindow::TREELIST);
            else if (_selection == "slider")
                return m_customWindow->getWindowItems(CustomWindow::SLIDER);
            else if (_selection == "menuitem")
                return m_customWindow->getWindowItems(CustomWindow::MENUITEM);
        }

        return std::vector<int>();
    }


    /////////////////////////////////////////////////
    /// \brief Returns the value of the selected
    /// window item as a string.
    ///
    /// \param windowItemID int
    /// \return NumeRe::WinItemValue
    ///
    /////////////////////////////////////////////////
    NumeRe::WinItemValue Window::getItemValue(int windowItemID) const
    {
        NumeRe::WinItemValue val;

        if (m_customWindow)
        {
            WindowItemValue value = m_customWindow->getItemValue(windowItemID);

            val.stringValue = value.stringValue.ToStdString();
            val.tableValue = value.tableValue;
            val.type = value.type.ToStdString();
        }

        return val;
    }


    /////////////////////////////////////////////////
    /// \brief Returns the label of the selected
    /// window item as a string.
    ///
    /// \param windowItemID int
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Window::getItemLabel(int windowItemID) const
    {
        if (m_customWindow)
            return m_customWindow->getItemLabel(windowItemID).ToStdString();

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Returns the state of the selected
    /// window item as a string.
    ///
    /// \param windowItemID int
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Window::getItemState(int windowItemID) const
    {
        if (m_customWindow)
            return m_customWindow->getItemState(windowItemID).ToStdString();

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Returns the color of the selected
    /// window item as a string.
    ///
    /// \param windowItemID int
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Window::getItemColor(int windowItemID) const
    {
        if (m_customWindow)
            return m_customWindow->getItemColor(windowItemID).ToStdString();

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief Returns the value of the selected
    /// property as a string.
    ///
    /// \param varName const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Window::getPropValue(const std::string& varName) const
    {
        if (m_customWindow)
            return m_customWindow->getPropValue(varName).ToStdString();

        return "nan";
    }


    /////////////////////////////////////////////////
    /// \brief Returns a list of all available window
    /// properties (comp. \c prop) in this window.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Window::getProperties() const
    {
        if (m_customWindow)
            return m_customWindow->getProperties().ToStdString();

        return "\"\"";
    }


    /////////////////////////////////////////////////
    /// \brief Enables changing the value of the
    /// selected window item to the passed value.
    ///
    /// \param _value const NumeRe::WinItemValue&
    /// \param windowItemID int
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Window::setItemValue(const NumeRe::WinItemValue& _value, int windowItemID)
    {
        if (m_customWindow)
        {
            WindowItemValue val;
            val.stringValue = _value.stringValue;
            val.tableValue = _value.tableValue;
            val.type = _value.type;
            return m_customWindow->setItemValue(val, windowItemID);
        }

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Enables changing the label of the
    /// selected window item to the passed label.
    ///
    /// \param _label const std::string&
    /// \param windowItemID int
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Window::setItemLabel(const std::string& _label, int windowItemID)
    {
        if (m_customWindow)
            return m_customWindow->setItemLabel(_label, windowItemID);

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Enables changing the state of the
    /// selected window item to the passed state.
    ///
    /// \param _state const std::string&
    /// \param windowItemID int
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Window::setItemState(const std::string& _state, int windowItemID)
    {
        if (m_customWindow)
            return m_customWindow->setItemState(_state, windowItemID);

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Enables changing the color of the
    /// selected window item to the passed color.
    ///
    /// \param _color const std::string&
    /// \param windowItemID int
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Window::setItemColor(const std::string& _color, int windowItemID)
    {
        if (m_customWindow)
            return m_customWindow->setItemColor(_color, windowItemID);

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Updates the graph in the custom
    /// window.
    ///
    /// \param _helper GraphHelper*
    /// \param windowItemID int
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Window::setItemGraph(GraphHelper* _helper, int windowItemID)
    {
        if (m_customWindow)
            return m_customWindow->setItemGraph(_helper, windowItemID);

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This function sets the value of the
    /// selected window property.
    ///
    /// \param _value const std::string&
    /// \param varName const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Window::setPropValue(const std::string& _value, const std::string& varName)
    {
        if (m_customWindow)
            return m_customWindow->setPropValue(_value, varName);

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Closes the current window.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool Window::closeWindow()
    {
        if (m_customWindow)
            return m_customWindow->closeWindow();

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This private member function is used
    /// by the window manager to detach itself from
    /// this window to avoid segmentation faults at
    /// application shutdown.
    ///
    /// \return void
    ///
    /// \note This is needed, because wxWidgets will
    /// not automatically destruct windows after they
    /// are closed (rather they are closed in some
    /// idle time).
    /////////////////////////////////////////////////
    void Window::detach()
    {
        m_manager = nullptr;
    }

    //
    //
    // CLASS WINDOWMANAGER
    //
    //

    /////////////////////////////////////////////////
    /// \brief This function is used by the
    /// registered windows to inform the window
    /// manager about new window statuses or return
    /// values.
    ///
    /// \param information const WindowInformation&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void WindowManager::updateWindowInformation(const WindowInformation& information)
    {
        if (m_windowMap.find(information.nWindowID) != m_windowMap.end())
            m_windowMap[information.nWindowID] = information;
    }


    /////////////////////////////////////////////////
    /// \brief This member function registers a new
    /// window or changes the registration to a new
    /// window.
    ///
    /// \param window Window*
    /// \param id size_t
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t WindowManager::registerWindow(Window* window, size_t id)
    {
        if (id == std::string::npos)
        {
            id = m_windowMap.size();

            // Ensure the new ID is unique
            while (m_windowMap.find(id) != m_windowMap.end())
                id++;
        }


        m_windowMap[id] = WindowInformation(id, window, 0, "");

        return id;
    }


    /////////////////////////////////////////////////
    /// \brief This member function will unregister a
    /// window. This is done only if the ID and the
    /// window pointer are both matching the internal
    /// mapping.
    ///
    /// \param window Window*
    /// \param id size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief Empty window manager constructor.
    /////////////////////////////////////////////////
    WindowManager::WindowManager()
    {
        //
    }


    /////////////////////////////////////////////////
    /// \brief Destructor. It will detach all
    /// registered windows from the manager. This is
    /// done to avoid segmentation faults during
    /// application shut down.
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This public member function will
    /// create a window object containing the passed
    /// graph.
    ///
    /// \param graph GraphHelper*
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t WindowManager::createWindow(GraphHelper* graph)
    {
        Window window(WINDOW_GRAPH, this, graph);

        NumeReKernel::getInstance()->showWindow(window);

        return window.getId();
    }


    /////////////////////////////////////////////////
    /// \brief This public member function will
    /// create a dialog window, which is described by
    /// the passed WindowSettings object.
    ///
    /// \param type WindowType
    /// \param settings const WindowSettings&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t WindowManager::createWindow(WindowType type, const WindowSettings& settings)
    {
        Window window(type, this, settings);

        NumeReKernel::getInstance()->showWindow(window);

        return window.getId();
    }


    /////////////////////////////////////////////////
    /// \brief This public member function will
    /// create a custom window, which is described by
    /// the passed layout XML object.
    ///
    /// \param layoutString tinyxml2::XMLDocument*
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t WindowManager::createWindow(tinyxml2::XMLDocument* layoutString)
    {
        Window window(WINDOW_CUSTOM, this, layoutString);

        NumeReKernel::getInstance()->showWindow(window);

        return window.getId();
    }


    /////////////////////////////////////////////////
    /// \brief This public member function will
    /// return the window information stored in the
    /// internal map. If the window was closed by the
    /// user and this was noted in the window
    /// information then the object is deleted from
    /// the internal map.
    ///
    /// \param windowId size_t
    /// \return WindowInformation
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This public member function will
    /// return the window information stored in the
    /// internal map. The access is modal, i.e. it's
    /// checked repeatedly, whether a window has been
    /// closed until the information is returned. The
    /// object is then deleted from the internal map.
    ///
    /// \param windowId size_t
    /// \return WindowInformation
    ///
    /////////////////////////////////////////////////
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


