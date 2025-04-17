/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#include <wx/stdpaths.h>

#include "reportissuedialog.hpp"
#include "../globals.hpp"
#include "../../kernel/core/io/logger.hpp"
#include "../../kernel/core/utils/stringtools.hpp"
#include "../../network/githubapi.hpp"
#include "../../kernel/versioninformation.hpp"

#ifdef RELEASE
// This file securely encodes the github api token. It cannot be found in the
// repository for this reason and is only compiled within the release version.
#include "../../secureauth.hpp"
#endif // RELEASE

BEGIN_EVENT_TABLE(ReportIssueDialog, wxDialog)
    EVT_BUTTON(-1, ReportIssueDialog::OnButtonClick)
    EVT_CHOICE(-1, ReportIssueDialog::OnDropDown)
END_EVENT_TABLE()

#define BUG "[Bug] I'm experiencing a problem"
#define FEATURE "[Feature] I'm missing something"
#define CHANGE "[Change] I want something to be changed"

// Testing repository:
//static const std::string sApiUrl = "https://api.github.com/repos/numeredev/ghfeatures/issues";
static const std::string sApiUrl = "https://api.github.com/repos/numere-org/NumeRe/issues";
static const std::string sRepoUrl = "https://github.com/numere-org/NumeRe/issues";

/////////////////////////////////////////////////
/// \brief Simple helper function to obtain the
/// app root path.
///
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getAppFilePath()
{
    wxStandardPaths systemPaths = wxStandardPaths::Get();
    wxString appPath = systemPaths.GetExecutablePath();
    wxFileName fullProgramPath(appPath);
    return fullProgramPath.GetPath().ToStdString();
}


/////////////////////////////////////////////////
/// \brief Temporary helper function to load the
/// auth token from a file and not to have it
/// hardcoded in code.
///
/// \return std::string
///
/////////////////////////////////////////////////
static std::string loadAuthToken()
{
    std::string sAuthFile = getAppFilePath() + "/github.auth";

    if (!wxFileName::Exists(sAuthFile))
    {
#ifdef RELEASE
        // This function decodes the encrypted github auth token and is
        // not part of this repository for this reason. If you don't trust
        // this code section: create a "github.auth" file within the
        // NumeRe root directory and put your github auth token as single
        // content into it. If the code finds this file, the following line
        // is never executed.
        return "ghp_" + getAuthToken();
#endif // RELEASE
        return "";
    }

    std::ifstream auth(sAuthFile);

    if (!auth.good())
        return "";

    std::getline(auth, sAuthFile);
    return sAuthFile;
}


/////////////////////////////////////////////////
/// \brief Construct a issue reporting dialog.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param errLoc ErrorLocation
///
/////////////////////////////////////////////////
ReportIssueDialog::ReportIssueDialog(wxWindow* parent, wxWindowID id, ErrorLocation errLoc) : wxDialog(parent, id, "Report an issue", wxDefaultPosition, wxSize(600*g_pixelScale, 730*g_pixelScale), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_isCriticalIssue(false)
{
    // Prepare the options for the dropdown and the radio group
    wxArrayString types;
    types.Add(FEATURE);
    types.Add(BUG);
    types.Add(CHANGE);
    wxArrayString logExcerpt;
    logExcerpt.Add("None");
    logExcerpt.Add("Last session");
    logExcerpt.Add("Current session");

    wxBoxSizer* vsizer = new wxBoxSizer(wxVERTICAL);
    GroupPanel* panel = new GroupPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_STATIC);

    panel->AddStaticText(panel, panel->getMainSizer(), "Thank you for reporting this issue.\nPlease fill out this form with all relevant information for understanding your issue in English. Consider also\nto give us some contact details to allow us getting in touch with you in case of questions.");

    panel->AddStaticText(panel, panel->getMainSizer(), "Select the type of your request");
    m_issueType = panel->CreateChoices(panel, panel->getMainSizer(), types);
    m_issueTitle = panel->CreateTextInput(panel, panel->getMainSizer(), "Give a very short summary *");
    m_issueDescription = panel->CreateTextInput(panel, panel->getMainSizer(),
                                                "Describe, what you observe or want to be changed (Markdown enabled) *", wxEmptyString,
                                                wxTE_MULTILINE | wxTE_RICH2 | wxTE_BESTWRAP, wxID_ANY, wxSize(-1, 80*g_pixelScale));
    wxBoxSizer* bugSizer = panel->createGroup(wxHORIZONTAL);
    wxBoxSizer* reproSizer = panel->createGroup(wxVERTICAL, bugSizer, 2);
    m_issueReproduction = panel->CreateTextInput(panel, reproSizer,
                                                 "How to reproduce the problem? (Markdown enabled) *", wxEmptyString,
                                                 wxTE_MULTILINE | wxTE_RICH2 | wxTE_BESTWRAP, wxID_ANY,
                                                 wxSize(-1, 80*g_pixelScale), wxALIGN_CENTER_VERTICAL, 2);

    m_logSelection = panel->CreateRadioBox(panel, bugSizer, "Attach excerpt from the log",
                                           logExcerpt, wxVERTICAL);
    m_expectedBehavior = panel->CreateTextInput(panel, panel->getMainSizer(),
                                                "What do you expect to happen? (Markdown enabled) *", wxEmptyString,
                                                wxTE_MULTILINE | wxTE_RICH2 | wxTE_BESTWRAP, wxID_ANY, wxSize(-1, 80*g_pixelScale));


    m_contact = panel->CreateTextInput(panel, panel->getMainSizer(), "Mailaddress or GitHub handle to get in touch with you (opt.)");

    wxStaticBoxSizer* disclaimerSizer = panel->createGroup("Privacy disclaimer", wxVERTICAL);
    wxStaticText* legalDisclaimer = panel->AddStaticText(disclaimerSizer->GetStaticBox(), disclaimerSizer, "Any information you enter here will be added to a new publicly available issue on GitHub. Please review\nthe contents carefully for any personal or confidential information before continuing.\nThis is especially important, if you attach an excerpt of your log file.");
    legalDisclaimer->SetForegroundColour(*wxRED);

    wxBoxSizer* buttonSizer = panel->createGroup(wxHORIZONTAL);

    m_okButton = panel->CreateButton(panel, buttonSizer, "Send issue", wxID_OK);
    panel->CreateButton(panel, buttonSizer, "Cancel", wxID_CANCEL);

    vsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer(vsizer);
    Centre();

    // It makes sense that markdown-enabled fields use a monospaced font
    wxFont markdownFont;
    markdownFont.SetNativeFontInfoUserDesc("Consolas");

    // We want to disable all tools until the user selects an issue type.
    // We also add some explanatory tooltips.
    m_issueTitle->Disable();
    m_issueTitle->SetToolTip("We will use this summary to create the title of your issue. It should point out your intent as precisely as possible in very few words. \"Bug\" or \"New feature\" does not fulfill this requirement.\n\nEx. \"Create a \"Report an issue\" feature\"");
    m_issueDescription->Disable();
    m_issueDescription->SetFont(markdownFont);
    m_issueDescription->SetToolTip("A clear and concise description of what the intent of this request is.\n\nEx. \"It would improve usability when [...]\"");
    m_issueReproduction->Hide();
    m_issueReproduction->SetFont(markdownFont);
    m_issueReproduction->SetToolTip("Give us the necessary steps leading to the problem, even if the problem is not reproducable on your end. It is good practice to use an enumerated list, but not strictly required.\n\nExample:\n1. load file XYZ\n2. Use method ABC\n3. ...");
    m_expectedBehavior->Disable();
    m_expectedBehavior->SetFont(markdownFont);
    m_expectedBehavior->SetToolTip("Tell us what you expect that shall happen (in case of changes or new features) or should have happened (in case of a bug) according to your opinion. Feel free to propose alternatives.\n\nEx. \"I would expect that using feature XYZ would do ABC.\"");
    m_logSelection->Hide();
    m_logSelection->SetSelection(0);
    m_contact->Disable();
    m_contact->SetToolTip("It is common that we have further questions about your request, so it is necessary that we can get in touch with you. Although optional, giving us this information is greatly appreciated.\n\nYou can also give us other contact possibilities. Mastodon handles or Discord handles are also fine, if you add the contact type, e.g. \"Mastodon: @numeredevs@fosstodon.org\"");
    m_okButton->Disable();

    // Is this an automatic crash report?
    if (errLoc == ERR_CRASH)
        fillCrashData();
    else if (errLoc == ERR_STARTUP)
        fillStartupData();
}


/////////////////////////////////////////////////
/// \brief Event handler for a button click.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void ReportIssueDialog::OnButtonClick(wxCommandEvent& event)
{
    if (event.GetId() == wxID_OK)
    {
        bool isBug = m_issueType->GetString(m_issueType->GetSelection()) == BUG;
        bool isFeature = m_issueType->GetString(m_issueType->GetSelection()) == FEATURE;

        // Validate inputs
        if (m_issueTitle->IsEmpty()
            || m_issueDescription->IsEmpty()
            || m_expectedBehavior->IsEmpty()
            || (m_issueReproduction->IsEmpty() && isBug))
        {
            wxMessageBox("Please fill out all fields marked with an asterisk.", "Information missing", wxCENTER | wxICON_ERROR | wxOK, this);
            return;
        }

#ifdef RELEASE
        // Is the user sure that he does not want to share any contact details
        if (m_contact->IsEmpty()
            && wxMessageBox("Are you sure that you do not want to share any contact details? Social network handles are also acceptable, if you add the network type.\n\nWithout the possibility to ask you any questions, resolving your issue might be impossible.", "Share contact details", wxCENTER | wxICON_QUESTION | wxYES_NO, this) == wxNO)
            return;
#endif // RELEASE

        // Create the issue
        GitHub::Issue issue;
        issue.title = m_issueTitle->GetValue().mb_str(wxConvUTF8);
        issue.assignees.push_back("numeredev");
        issue.body = "### DESCRIPTION\n**Describe, what you observe or want to be changed:**\n" + std::string(m_issueDescription->GetValue().mb_str(wxConvUTF8));

        if (isBug)
            issue.body += "\n\n**How to reproduce the problem:**\n" + std::string(m_issueReproduction->GetValue().mb_str(wxConvUTF8));

        issue.body += "\n\n**Describe, what you expect to happen:**\n" + std::string(m_expectedBehavior->GetValue().mb_str(wxConvUTF8));

        if (isBug)
            issue.body += "\n\n**System information:**\n- " + g_logger.get_system_information() + "\n- Version: v" + getVersion() + " (var. " + getSubVersion() + ")";

        if (m_contact->GetValue().length())
            issue.body += "\n\n**How we can contact you:**\n" + std::string(m_contact->GetValue().mb_str(wxConvUTF8));

        if (isBug && m_logSelection->GetSelection() != 0)
        {
            issue.body += "\n\n**Excerpt from the log:**\n```\n";

            if (m_logSelection->GetSelection() == 1)
                issue.body += ansiToUtf8(g_logger.get_session_log(1)) + "```";
            else
                issue.body += ansiToUtf8(g_logger.get_session_log()) + "```";
        }

        // Add generic text to the body
        issue.body += "\n\n-----------------------------------------------------\n## DEVS' SECTION\n### ANALYSIS\n(*Describe, what's the issue and which changes have to be made*)\n\n### IMPLEMENTATION STEPS\n(*see also our [Wiki for implementation guidelines](https://github.com/numeredev/NumeRe/wiki/HowTo:-Contribute-Code)*)\n- Implement the necessary changes in a new branch created here on GitHub\n- Test your implementation\n\n### DOCUMENTATION STEPS\n(*see also our [Wiki for further information](https://github.com/numeredev/NumeRe/wiki/HowTo:-Create-Language-Strings)*)\n- Update the changes log\n- Add comments to your implementation\n- Add Doxygen documentation comments- Create or update the documentation articles (`*.NHLP` and `*.NDB` files, if needed)\n- Update the language strings (`*.NLNG` files, if needed)\n\n### PULL REQUEST\n- Create a pull request for your changes\n- Fill out the template\n- Assign @numere-org/maintainers as reviewers";

        if (isFeature)
        {
            issue.type = "Feature request";
            issue.labels.push_back("feature");
        }
        else if (isBug)
        {
            issue.type = "Bug report";
            issue.labels.push_back("bug");

            if (m_isCriticalIssue)
                issue.labels.push_back("critical");
        }
        else
        {
            issue.type = "Change request";
            issue.labels.push_back("change");
        }


        static std::string sAuthToken = loadAuthToken();

        // Transmit via CURL
        std::string sIssueUrl = GitHub::create(issue,
                                               sApiUrl,
                                               sAuthToken,
                                               getAppFilePath());

        if (sIssueUrl.length())
        {
            int ret = wxMessageBox("The issue was created successfully at\n\n" + sIssueUrl + "\n\nDo you want to view it in your browser?",
                                   "Issue created successfully", wxCENTER | wxYES | wxNO | wxICON_QUESTION, this);

            if (ret == wxYES)
                wxLaunchDefaultBrowser(sIssueUrl);

            EndModal(wxID_OK);
        }
        else
        {
            int ret = wxMessageBox("We could not create the issue successfully.\n\nDo you want to create it manually via your browser? You will need a GitHub account, though.\n(We'll keep this dialog open for you. Diagnostic information can be found in 'github.log'.)",
                                   "Error in issue creation", wxCENTER | wxYES | wxNO | wxICON_ERROR, this);

            if (ret == wxYES)
            {
                wxLaunchDefaultBrowser(sRepoUrl);
                return;
            }

            EndModal(wxID_CANCEL);
        }
    }
    else
        // Just continue regularily
        event.Skip();
}


/////////////////////////////////////////////////
/// \brief If the user switches the issue type,
/// some UI elements are hidden or shown with
/// this event handler.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
void ReportIssueDialog::OnDropDown(wxCommandEvent& event)
{
    if (m_issueType->GetString(event.GetInt()) == BUG)
    {
        m_issueReproduction->Show(true);
        m_logSelection->Show(true);
    }
    else
    {
        m_issueReproduction->Hide();
        m_logSelection->Hide();
    }

    m_issueTitle->Enable(true);
    m_issueDescription->Enable(true);
    m_expectedBehavior->Enable(true);
    m_contact->Enable(true);
    m_okButton->Enable(true);
}


/////////////////////////////////////////////////
/// \brief If we are opening this dialog after a
/// crash, then we automatically fill in the
/// relevant crash data.
///
/// \return void
///
/////////////////////////////////////////////////
void ReportIssueDialog::fillCrashData()
{
    m_issueType->SetSelection(1);
    m_issueTitle->Enable(true);
    m_issueDescription->Enable(true);
    m_issueReproduction->Show(true);
    m_logSelection->Show(true);
    m_expectedBehavior->Enable(true);
    m_contact->Enable(true);
    m_okButton->Enable(true);

    m_issueTitle->SetValue("Automated: Crash report");
    m_issueDescription->SetValue("While interacting with the application, I experienced a crash-to-desktop. System data and logs are appended.");
    m_logSelection->SetSelection(1);
    m_expectedBehavior->SetValue("Resolve the problem for this crash.");
    m_isCriticalIssue = true;
}


/////////////////////////////////////////////////
/// \brief If we are opening this dialog after a
/// startup, then we automatically fill in the
/// relevant startup problem data.
///
/// \return void
///
/////////////////////////////////////////////////
void ReportIssueDialog::fillStartupData()
{
    m_issueType->SetSelection(1);
    m_issueTitle->Enable(true);
    m_issueDescription->Enable(true);
    m_issueReproduction->Show(true);
    m_logSelection->Show(true);
    m_expectedBehavior->Enable(true);
    m_contact->Enable(true);
    m_okButton->Enable(true);

    m_issueTitle->SetValue("Automated: Startup problem");
    m_issueDescription->SetValue("While starting the application, some problems occured. System data and logs are appended.");
    m_issueReproduction->SetValue("See attached logfile for relevant error locations.");
    m_logSelection->SetSelection(2);
    m_expectedBehavior->SetValue("Resolve the problem occured during this startup.");
}

