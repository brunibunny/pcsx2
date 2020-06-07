#include "PrecompiledHeader.h"
#include "NetplayLobbyPanel.h"

NetplayLobbyPanel::NetplayLobbyPanel(wxWindow *parent)
    : NetplayLobbyPanelBase(parent)
{
    m_playersBox->InsertColumn(0, _("#"), 0, 25);
    m_playersBox->InsertColumn(1, _("Name"), 0, 75);
    m_playersBox->InsertColumn(2, _("Ping"), 0, 50);
}

void NetplayLobbyPanel::SetInputDelay(int value)
{
    this->m_inputDelaySpinner->SetValue(value);
}
int NetplayLobbyPanel::GetInputDelay()
{
    return this->m_inputDelaySpinner->GetValue();
}

void NetplayLobbyPanel::UpdateHostModeUI(bool host)
{
    this->m_inputDelayLabel->Enable(host);
    this->m_inputDelaySpinner->Enable(host);
    this->m_moveUpButton->Enable(host);
    this->m_moveDownButton->Enable(host);
    this->m_startButton->Enable(host);
}
void NetplayLobbyPanel::SetStartHandler(const event_handler_type &handler)
{
    m_start_handler = handler;
}
void NetplayLobbyPanel::SetUserlist(const std::vector<userinfo> &usernames, int num_players)
{
	if (m_playersBox->GetItemCount() < num_players)
        m_playersBox->InsertItem(0, "itm");

	if (m_playersBox->GetItemCount() > num_players)
        m_playersBox->DeleteItem(0);

    for (auto &str : usernames) {
        m_playersBox->SetItem(str.side, 0, std::to_string(str.side + 1));
        m_playersBox->SetItem(str.side, 1, str.name);
        m_playersBox->SetItem(str.side, 2, str.ping);
    }
}

void NetplayLobbyPanel::AddChatMessage(const std::string &username, const std::string &message)
{
    //m_NetplayChatTextCtrl->SetDefaultStyle(wxTextAttr(*wxBLUE));
    m_NetplayChatTextCtrl->AppendText(username);
    //m_NetplayChatTextCtrl->SetDefaultStyle(wxTextAttr(*wxGREEN));
    m_NetplayChatTextCtrl->AppendText(": ");
    //m_NetplayChatTextCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK));
    m_NetplayChatTextCtrl->AppendText(message);
    m_NetplayChatTextCtrl->AppendText("\r\n");
}

void NetplayLobbyPanel::OnStart(wxCommandEvent &event)
{
    if (m_start_handler)
        m_start_handler();
}

void NetplayLobbyPanel::SetSendTextHandler(const std::function<void(std::string message)> &handler)
{
    m_sendtext_handler = handler;
}

void NetplayLobbyPanel::OnSendText(wxCommandEvent &event)
{
    std::string msg = m_NetplayConsoleEntryTextCtrl->GetValue().mb_str();

    AddChatMessage("<me>", msg);

    if (m_sendtext_handler) {
        m_sendtext_handler(msg);
    }

    m_NetplayConsoleEntryTextCtrl->Clear();
}
void NetplayLobbyPanel::EnableOnlyChat()
{
    m_moveUpButton->Disable();
    m_moveDownButton->Disable();
    m_startButton->Disable();
    m_inputDelayLabel->Disable();
    m_inputDelaySpinner->Disable();
}