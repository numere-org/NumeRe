
#include "wx/wx.h"
#include "wx/dir.h"
#include <string>
#include <vector>
#include "IconManager.h"
#include "../common/datastructures.h"

using namespace std;
class wxTreeItemData;

class FileNameTreeData : public wxTreeItemData
{
    public:
        wxString filename;
};

class DirTraverser : public wxDirTraverser
{
    private:
        wxTreeCtrl* rootNode;
        IconManager* iconManager;
        wxTreeItemId id;
        FileFilterType fileSpec;
        wxString path;
        vector<wxTreeItemId> vcurrentnodes;
        unsigned int ncurrentdepth;
    public:
        DirTraverser(wxTreeCtrl* therootNode, IconManager* theiconmanager, wxTreeItemId theid, const wxString& thepath, FileFilterType thefilespec);

        virtual wxDirTraverseResult OnFile(const wxString& filename);
        virtual wxDirTraverseResult OnDir(const wxString& dirname);
};

