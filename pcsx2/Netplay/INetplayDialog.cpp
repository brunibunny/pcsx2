#include "PrecompiledHeader.h"

#include <memory>

#include "INetplayDialog.h"
#include "gui/NetplayDialog.h"
#include "App.h"
#include "Utilities.h"

enum NetplayConfigurationPhase
{
	None,
	Settings,
	Confirmation,
	Ready
};

class ConcreteNetplayDialog: public INetplayDialog
{
public:
	void SetCloseEventHandler(const event_handler_type& handler)
	{
		Utilities::ExecuteOnMainThread([&]() {
			_close_handler = handler;
		});
	}
	event_handler_type& GetCancelEventHandler()
	{
		return _close_handler;
	}
	void SetSettings(const NetplaySettings& settings)
	{
		Utilities::ExecuteOnMainThread([&]() {
			m_dialog->SetSettings(settings);
		});
	}
	NetplaySettings GetSettings()
	{
		NetplaySettings settings;
		Utilities::ExecuteOnMainThread([&]() {
			settings = m_dialog->GetSettings();
		});
		return settings;
	}
	void Initialize()
	{
		auto ok_hdl = [&]() {
			_operation_success = true;
			if (_phase == Settings)
			{
				m_dialog->GetContent()->Disable();
				_phase = Confirmation;
				if (_settings_ready_handler)
					_settings_ready_handler();
			}
			else
			{
				m_dialog->EnableOnlyChat();
				_cond->notify_one();
			}
		};
		auto close_hdl = [&]() { 
			try
			{
				_operation_success = false;
				_cond->notify_one();
				if(_close_handler)
					_close_handler();
			}
			catch(...){}
		};
		auto sendchattext_hdl = [&](std::string msg) {
			try
			{
				if (_sendchattext_handler)
					_sendchattext_handler(msg);
			}
			catch (...) {}
		};
		Utilities::ExecuteOnMainThread([&]() {
			if(m_dialog)
				m_dialog.reset();
			m_dialog.reset(new NetplayDialog((wxWindow*)GetMainFramePtr()));
			_phase = Settings;
			m_dialog->SetOKHandler(ok_hdl);
			m_dialog->SetSendChatMessageHandler(sendchattext_hdl);
			m_dialog->SetCloseEventHandler(close_hdl);
		});
	}
	void Show()
	{
		_is_closed = false;
		Utilities::ExecuteOnMainThread([&]() {
			_cond.reset(new std::condition_variable());
			m_dialog->Show();
		});
	}
	bool IsShown()
	{
		return !!m_dialog;
	}
	void Close()
	{
		{
			std::lock_guard<std::mutex> lock(_close_mutex);
			if(!_is_closed)
				_is_closed = true;
			else
				return;
		}
		_operation_success = false;
		_cond->notify_one();
		Utilities::ExecuteOnMainThread([&]() {
			if(m_dialog) m_dialog.reset();
		});
	}
	void SetConnectionSettingsHandler(const event_handler_type& handler)
	{
		Utilities::ExecuteOnMainThread([&]() {
			_settings_ready_handler = handler;
		});
	}
	int WaitForConfirmation()
	{
		if(!m_dialog)
			return -1;
		if(_phase != Confirmation)
			throw std::exception("invalid state");
		std::unique_lock<std::mutex> lock(_cond_mutex);
		_cond->wait(lock);
		_phase = _operation_success ? Ready : None;
		if(m_dialog && _operation_success)
			return m_dialog->GetLobbyPanel().GetInputDelay();
		else
			return -1;
	}
	void OnConnectionEstablished(int input_delay)
	{
		if(_phase != Confirmation)
			throw std::exception("invalid state");
		Utilities::ExecuteOnMainThread([&]() {
			if(!m_dialog)
				return;
			NetplayLobbyPanel& p = m_dialog->GetLobbyPanel();
			p.SetInputDelay(input_delay);
			p.UpdateHostModeUI(GetSettings().Mode == HostMode);

			m_dialog->SetContent(&p);
		});
	}
	int GetInputDelay()
	{
		if(m_dialog)
			return m_dialog->GetLobbyPanel().GetInputDelay();
		else
			return -1;
	}
	void SetInputDelay(int input_delay)
	{
		Utilities::ExecuteOnMainThread([&]() {
			if(m_dialog)
				m_dialog->GetLobbyPanel().SetInputDelay(input_delay);
		});
	}
	void SetStatus(const wxString& status)
	{
		Utilities::ExecuteOnMainThread([&]() {
			if(m_dialog) m_dialog->SetStatus(status);
		});
	}
    void SetUserlist(const std::vector<userinfo> &usernames, int num_players)
	{
		Utilities::ExecuteOnMainThread([&]() {
			if(m_dialog)
				m_dialog->GetLobbyPanel().SetUserlist(usernames, num_players);
		});
	}
	void AddChatMessage(const std::string &username, const std::string &message)
	{
		Utilities::ExecuteOnMainThread([&]() {
			if (m_dialog)
				m_dialog->GetLobbyPanel().AddChatMessage(username, message);
		});
	}
	void SetSendChatMessageHandler(const std::function<void(std::string message)>& handler)
	{
		Utilities::ExecuteOnMainThread([&]() {
			_sendchattext_handler = handler;
		});
	}
protected:
	bool _is_closed;
	std::shared_ptr<NetplayDialog> m_dialog;
	std::shared_ptr<std::condition_variable> _cond;
	std::mutex _cond_mutex;
	std::mutex _close_mutex;
	bool _operation_success;
	NetplayConfigurationPhase _phase;
	event_handler_type _close_handler;
	event_handler_type _settings_ready_handler;
	std::function<void(std::string message)> _sendchattext_handler;
};


INetplayDialog* INetplayDialog::GetInstance()
{
	if(!instance)
		instance = new ConcreteNetplayDialog();
	return instance;
}

INetplayDialog* INetplayDialog::instance = 0;
