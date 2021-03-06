#include "NetplayDialogBase.h"
#include <functional>
#include "Netplay/INetplayDialog.h"

class NetplayLobbyPanel : public NetplayLobbyPanelBase
{
public:
    NetplayLobbyPanel(wxWindow *parent);

    typedef std::function<void()> event_handler_type;
    void SetStartHandler(const event_handler_type &handler);
    void SetSendTextHandler(const std::function<void(std::string message)> &handler);

    void SetInputDelay(int value);
    int GetInputDelay();
    void UpdateHostModeUI(bool host = true);
    void SetUserlist(const std::vector<userinfo> &usernames, int num_players);
    void AddChatMessage(const std::string &username, const std::string &message);
    void EnableOnlyChat();

protected:
    virtual void OnStart(wxCommandEvent &event);
    virtual void OnSendText(wxCommandEvent &event);
    event_handler_type m_start_handler;
    std::function<void(std::string message)> m_sendtext_handler;
};