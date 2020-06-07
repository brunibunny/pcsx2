#pragma once
#include <functional>
#include "NetplaySettings.h"

typedef struct
{
    std::string name;
    std::string ping;
    int side;
} userinfo;

class INetplayDialog
{
protected:
	static INetplayDialog* instance;
public:
	static INetplayDialog* GetInstance();

	typedef std::function<void()> event_handler_type;

	virtual void SetCloseEventHandler(const event_handler_type& handler) = 0;
	virtual event_handler_type& GetCancelEventHandler() = 0;

	virtual void SetSettings(const NetplaySettings& settings) = 0;
	virtual NetplaySettings GetSettings() = 0;

	virtual void Initialize() = 0;
	virtual void Show() = 0;
	virtual bool IsShown() = 0;
	virtual void Close() = 0;

	virtual void SetConnectionSettingsHandler(const event_handler_type& handler) = 0;
	virtual int WaitForConfirmation() = 0;

	virtual void OnConnectionEstablished(int input_delay) = 0;
	virtual int GetInputDelay() = 0;
	virtual void SetInputDelay(int input_delay) = 0;
	virtual void SetStatus(const wxString& status) = 0;

	virtual void SetUserlist(const std::vector<userinfo> &usernames, int num_players) = 0;

	virtual void SetSendChatMessageHandler(const std::function<void(std::string message)>& handler) = 0;
	virtual void AddChatMessage(const std::string &username, const std::string &message) = 0;
};
