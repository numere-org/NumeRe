/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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


#include <wx/artprov.h>
#include <wx/dir.h>
#include <wx/clipbrd.h>
#include "imagepanel.hpp"
#include "../../kernel/core/ui/language.hpp"

extern Language _guilang;


BEGIN_EVENT_TABLE(ImagePanel, wxPanel)
// some useful events
/*
 EVT_MOTION(wxImagePanel::mouseMoved)
 EVT_LEFT_DOWN(wxImagePanel::mouseDown)
 EVT_LEFT_UP(wxImagePanel::mouseReleased)
 EVT_RIGHT_DOWN(wxImagePanel::rightClick)
 EVT_LEAVE_WINDOW(wxImagePanel::mouseLeftWindow)
 EVT_KEY_UP(wxImagePanel::keyReleased)
 EVT_MOUSEWHEEL(wxImagePanel::mouseWheelMoved)
 */
    EVT_ENTER_WINDOW    (ImagePanel::OnEnter)
    EVT_SET_FOCUS       (ImagePanel::OnFocus)
    EVT_KILL_FOCUS      (ImagePanel::OnLoseFocus)
    EVT_KEY_DOWN        (ImagePanel::keyPressed)
    EVT_NAVIGATION_KEY  (ImagePanel::OnNavigationKey)
    // catch paint events
    EVT_PAINT           (ImagePanel::paintEvent)
    //Size event
    EVT_SIZE            (ImagePanel::OnSize)

    EVT_MENU            (ID_SAVEIMAGE, ImagePanel::OnSaveAs)
    EVT_MENU            (ID_COPYIMAGE, ImagePanel::OnCopy)
    EVT_MENU            (ID_NEXTIMAGE, ImagePanel::OnNextImage)
    EVT_MENU            (ID_PREVIOUSIMAGE, ImagePanel::OnPreviousImage)
END_EVENT_TABLE()



/////////////////////////////////////////////////
/// \brief ImagePanel constructor
///
/// \param parent wxFrame*
/// \param file wxString
/// \param format wxBitmapType
///
/////////////////////////////////////////////////
ImagePanel::ImagePanel(wxFrame* parent, wxString file, wxBitmapType format) : wxPanel(parent)
{
    // load the file... ideally add a check to see if loading was successful
    wxInitAllImageHandlers();
    parentFrame = parent;

    this->LoadImage(file, format, false);

    toptoolbar = parentFrame->CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT);
    toptoolbar->AddTool(ID_SAVEIMAGE, _guilang.get("GUI_TB_SAVE"), wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), _guilang.get("GUI_TB_SAVE_TTP"));
    toptoolbar->AddTool(ID_COPYIMAGE, _guilang.get("GUI_TB_COPY"), wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), _guilang.get("GUI_TB_COPY"));
    toptoolbar->AddSeparator();
    toptoolbar->AddTool(ID_PREVIOUSIMAGE, _guilang.get("GUI_TB_PREVIOUSIMAGE"), wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR), _guilang.get("GUI_TB_PREVIOUSIMAGE"));
    toptoolbar->AddTool(ID_NEXTIMAGE, _guilang.get("GUI_TB_NEXTIMAGE"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR), _guilang.get("GUI_TB_NEXTIMAGE"));
    toptoolbar->Realize();

    parentFrame->Bind(wxEVT_MENU, &ImagePanel::OnSaveAs, this, ID_SAVEIMAGE);
    parentFrame->Bind(wxEVT_MENU, &ImagePanel::OnCopy, this, ID_COPYIMAGE);
    parentFrame->Bind(wxEVT_MENU, &ImagePanel::OnNextImage, this, ID_NEXTIMAGE);
    parentFrame->Bind(wxEVT_MENU, &ImagePanel::OnPreviousImage, this, ID_PREVIOUSIMAGE);
}


/////////////////////////////////////////////////
/// \brief This method loads the specified file,
/// updates the parent frame's size (if desired)
/// and displays the image as a bitmap.
///
/// \param filename const wxString&
/// \param format wxBitmapType
/// \param doUpdateFrame bool
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::LoadImage(const wxString& filename, wxBitmapType format, bool doUpdateFrame)
{
    image.LoadFile(filename, format);
    w = image.GetWidth();
    h = image.GetHeight();
    sized_w = -1;
    sized_h = -1;
    currentFile = filename;

    if (doUpdateFrame)
    {
        this->SetSize(this->getRelation()*600,600);
        parentFrame->SetClientSize(this->GetSize());
        parentFrame->SetTitle("NumeRe-ImageViewer: " + wxFileName(filename).GetName());
        this->paintNow();
    }
}


/////////////////////////////////////////////////
/// \brief This method tries to load the desired
/// image file and returns, whether it was
/// successful or not.
///
/// \param filename const wxFileName&
/// \return bool
///
/////////////////////////////////////////////////
bool ImagePanel::LoadNextImage(const wxFileName& filename)
{
    if (filename.GetExt() == "png" || filename.GetExt() == "bmp" || filename.GetExt() == "jpg"  || filename.GetExt() == "jpeg" || filename.GetExt() == "gif" || filename.GetExt() == "tif" || filename.GetExt() == "tiff")
    {
        wxBitmapType format;

        if (filename.GetExt() == "png")
            format = wxBITMAP_TYPE_PNG;
        if (filename.GetExt() == "bmp")
            format = wxBITMAP_TYPE_BMP;
        if (filename.GetExt() == "jpg" || filename.GetExt() == "jpeg")
            format = wxBITMAP_TYPE_JPEG;
        if (filename.GetExt() == "gif")
            format = wxBITMAP_TYPE_GIF;
        if (filename.GetExt() == "tif" || filename.GetExt() == "tiff")
            format = wxBITMAP_TYPE_TIF;

        //wxFileName current(currentFile);
        LoadImage(filename.GetFullPath(), format);
        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This method returns a list of all
/// files in the current folder (used for
/// iterating over all images).
///
/// \param dirname const wxString&
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString ImagePanel::getFileList(const wxString& dirname)
{
    wxArrayString filelist;
    if (wxDir::GetAllFiles(dirname, &filelist, "*.*", wxDIR_FILES))
        return filelist;
    return wxArrayString();
}


/////////////////////////////////////////////////
/// \brief Called by the system of by wxWidgets
/// when the panel needs to be redrawn. You can
/// also trigger this call by calling
/// Refresh()/Update().
///
/// \param evt wxPaintEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::paintEvent(wxPaintEvent & evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    render(dc);
}


/////////////////////////////////////////////////
/// \brief Alternatively, you can use a clientDC
/// to paint on the panel at any time. Using this
/// generally does not free you from catching
/// paint events, since it is possible that e.g.
/// the window manager throws away your drawing
/// when the window comes to the background, and
/// expects you will redraw it when the window
/// comes back (by sending a paint event).
///
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    wxClientDC dc(this);
    render(dc);
}


/////////////////////////////////////////////////
/// \brief Here we do the actual rendering. I put
/// it in a separate method so that it can work
/// no matter what type of DC (e.g. wxPaintDC or
/// wxClientDC) is used.
///
/// \param dc wxDC&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::render(wxDC&  dc)
{
    int neww, newh;
    dc.GetSize(&neww, &newh);

    if (neww != sized_w || newh != sized_h)
    {
        if (neww/(double)newh > w/(double)h)
        {
            sized_w = w/(double)h*newh;
            sized_h = newh;
        }
        else
        {
            sized_w = neww;
            sized_h = h/(double)w*neww;
        }
        // BICUBIC may be slow, but in this case it doesn't have to be fast. However, in most cases the images will be mase smaller and the default quality is bad in this case
        resized = wxBitmap(image.Scale(sized_w, sized_h, wxIMAGE_QUALITY_BICUBIC));
        dc.DrawBitmap( resized, 0, 0, false );
    }
    else
    {
        dc.DrawBitmap( resized, 0, 0, false );
    }
}


/////////////////////////////////////////////////
/// \brief Here we call refresh to tell the panel
/// to draw itself again. So when the user
/// resizes the image panel the image should be
/// resized too.
///
/// \param event wxSizeEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnSize(wxSizeEvent& event)
{
    Refresh();
    //skip the event.
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This function closes the parent frame,
/// if the user presses the ESC key.
///
/// \param event wxKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::keyPressed(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the image
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();
}


/////////////////////////////////////////////////
/// \brief This member function chooses the next
/// or previous image in the file list, depending
/// on the arrow key pressed by the user.
///
/// \param event wxNavigationKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnNavigationKey(wxNavigationKeyEvent& event)
{
    // connect the navigation keys with next and previous image
    wxCommandEvent commandevent;
    if (event.GetDirection())
        OnNextImage(commandevent);
    else
        OnPreviousImage(commandevent);
}


/////////////////////////////////////////////////
/// \brief On enter event handler focuses this
/// panel.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief On focus event handler.
///
/// \param event wxFocusEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnFocus(wxFocusEvent& event)
{
    //m_parent->SetTransparent(wxIMAGE_ALPHA_OPAQUE);
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief On lose focus event handler.
///
/// \param event wxFocusEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnLoseFocus(wxFocusEvent& event)
{
    //m_parent->SetTransparent(80);
    event.Skip();
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// "save as" toolbar option of the parent frame.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnSaveAs(wxCommandEvent& event)
{
    wxString title = _guilang.get("GUI_DLG_SAVEAS");
    wxString filterString = "PNG (*.png)|*.png|Bitmap (*.bmp)|*.bmp|JPEG (*.jpg)|*.jpg";
    wxFileName fileName = currentFile;
    wxFileDialog dlg(this, title, fileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), fileName.GetName(), filterString, wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);

    // ie, user clicked cancel
    if(dlg.ShowModal() != wxID_OK)
    {
        return;
    }

    fileName = wxFileName(dlg.GetPath());

    image.SaveFile(fileName.GetFullName());
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// "copy" toolbar option of the parent frame.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnCopy(wxCommandEvent& event)
{
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxBitmapDataObject(wxBitmap(image)));
        wxTheClipboard->Close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function displays the next
/// image in the current folder.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnNextImage(wxCommandEvent& event)
{
    wxFileName filename(currentFile);
    wxArrayString filelist = getFileList(filename.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
    if (!filelist.size())
        return;
    int nIndex = filelist.Index(filename.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + filename.GetFullName());
    if (nIndex == wxNOT_FOUND)
        return;

    for (size_t i = 1; i < filelist.size(); i++)
    {
        if (nIndex+i == filelist.size())
            nIndex = -(int)i;

        if (LoadNextImage(filelist[i+nIndex]))
            return;
    }
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// previous image in the current folder.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void ImagePanel::OnPreviousImage(wxCommandEvent& event)
{
    wxFileName filename(currentFile);
    wxArrayString filelist = getFileList(filename.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
    if (!filelist.size())
        return;
    int nIndex = filelist.Index(filename.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + filename.GetFullName());
    if (nIndex == wxNOT_FOUND)
        return;

    for (int i = -1; i > -(int)filelist.size(); i--)
    {
        if (nIndex+i < 0)
            nIndex = filelist.size()-i-1;

        if (LoadNextImage(filelist[i+nIndex]))
            return;
    }
}


