#include "PrecompiledHeader.h"

#include "AppConfig.h"
#include <wx/stdpaths.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Netplay/NetplayPlugin.h"
#include "Netplay/INetplayDialog.h"

#include "shoryu/session.h"
#include "Message.h"
#include "Replay.h"
#include "NetplaySettings.h"
#include "Utilities.h"


//#define CONNECTION_TEST

class NetplayPlugin : public INetplayPlugin
{
	typedef shoryu::session<Message, EmulatorSyncState> session_type;
	std::shared_ptr<session_type> _session;
	std::shared_ptr<std::thread> _connect_thread;

public:
	NetplayPlugin()
		: _is_initialized(false), _is_stopped(false), _dialog(nullptr)
	{
	}

	void HandleUsernames(const std::vector<std::string> &usernames)
	{
		if (_dialog)
			_dialog->SetUserlist(usernames);
	}

	void HandleChatMessage(const std::string &username, const std::string &message)
	{
		if (_dialog)
			_dialog->AddChatMessage(username, message);
	}

	void Open()
	{
		_dialog = INetplayDialog::GetInstance();
		_is_stopped = false;
		NetplaySettings& settings = g_Conf->Netplay;

		// FIXME: change to assert, UI shouldn't allow this
		if( settings.HostPort <= 0 || settings.HostPort > 65535 )
		{
			Stop();
			ConsoleErrorMT(wxString::Format(wxT("NETPLAY: Invalid host port: %u."), settings.HostPort));
			return;
		}

		// FIXME: change to assert, UI shouldn't allow this
		if (settings.ListenPort <= 0 || settings.ListenPort > 65535)
		{
			Stop();
			ConsoleErrorMT(wxString::Format(wxT("NETPLAY: Invalid listen port: %u."), settings.ListenPort));
			return;
		}

		// FIXME: change to assert, UI shouldn't allow this
		if( settings.Mode == ConnectMode && settings.HostAddress.Len() == 0 )
		{
			Stop();
			ConsoleErrorMT(wxT("NETPLAY: Invalid hostname."));
			return;
		}

		recursive_lock lock(_mutex);
		if(!_dialog->IsShown())
		{
			lock.unlock();
			Stop();
			return;
		}

		shoryu::prepare_io_service();
		_session.reset(new session_type());
		_session->userlist_handler([&](const std::vector<std::string> &usernames) {HandleUsernames(usernames); });
		_session->set_chatmessage_handler([&](const std::string &username, const std::string &message) {HandleChatMessage(username, message); });
#ifdef CONNECTION_TEST
		_session->send_delay_min(40);
		_session->send_delay_max(80);
		_session->packet_loss(25);
#endif

		// 0 picks a random port
		int localPort = (settings.Mode == HostMode) ? settings.ListenPort : 0;

		if(_session->bind(localPort))
		{
			_state = SSNone;
			_session->username(std::string((const char*)settings.Username.mb_str(wxConvUTF8)));

			if(g_Conf->Netplay.SaveReplay)
			{
				_replay.reset(new Replay());
				_replay->Mode(Recording);
			}

			_game_name.clear();
			std::function<bool()> connection_func;

			if(settings.Mode == ConnectMode || settings.Mode == ObserveMode)
				connection_func = [this, settings]() { return Join(settings.HostAddress, settings.HostPort, 0); };
			else
				connection_func = [this]() { return Host(); };

			// FIXME: This name clashes badly with session::set_chatmessage_handler
			// specify which is incoming and which is outgoing
			_dialog->SetSendChatMessageHandler([this](const std::string &msg) {
				_session->send_chatmessage(msg);
			});

			// spawn thread with connection handler
			// connection_func returns when session is started or cancelled
			_connect_thread.reset(new std::thread([this, connection_func]() {
				_state = connection_func() ? SSReady : SSCancelled;
			}));
		}
		else
		{
			lock.unlock();
			Stop();
			ConsoleErrorMT(wxString::Format(wxT("NETPLAY: Unable to bind port %u."), localPort));
		}
	}

	bool IsInit()
	{
		return _is_initialized;
	}

	void Init()
	{
		_is_initialized = true;
		_is_stopped = false;
		Utilities::SaveSettings();
		Utilities::ResetSettingsToSafeDefaults();
	}

	void Close()
	{
		_is_initialized = false;
		EndSession();
		Utilities::RestoreSettings();

		if(_mcd_backup.size())
		{
			Utilities::WriteMCD(0,0,_mcd_backup);
			_mcd_backup.clear();
		}

		if(_replay)
		{
			if(_state == SSRunning)
			{
				try
				{
					wxDirName dir = (wxDirName)wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
					dir = dir.Combine(wxDirName("replays"));
					wxString replayName = _game_name + wxT(".rep");
					replayName.Replace(wxT("<"),wxT("-"));
					replayName.Replace(wxT(">"),wxT("-"));
					replayName.Replace(wxT(":"),wxT("-"));
					replayName.Replace(wxT("\""),wxT("-"));
					replayName.Replace(wxT("/"),wxT("-"));
					replayName.Replace(wxT("\\"),wxT("-"));
					replayName.Replace(wxT("|"),wxT("-"));
					replayName.Replace(wxT("?"),wxT("-"));
					replayName.Replace(wxT("*"),wxT("-"));
					wxString file = ( dir + replayName ).GetFullPath();
					ConsoleInfoMT(wxT("Saving replay to ") + file);
					_replay->SaveToFile(file);
				}
				catch(std::exception& e)
				{
					Stop();
					ConsoleErrorMT(wxT("REPLAY: ") + wxString(e.what(), wxConvLocal));
				}
			}
			_replay.reset();
		}

		Utilities::ExecuteOnMainThread([&]() {
			UI_EnableEverything();
		});
	}

	void ConsoleInfoMT(const wxString& message)
	{
		Utilities::ExecuteOnMainThread([&]() {
			Console.WriteLn(Color_StrongGreen, message);
		});
	}

	void ConsoleErrorMT(const wxString& message)
	{
		Utilities::ExecuteOnMainThread([&]() {
			Console.Error(message);
		});
	}

	void ConsoleWarningMT(const wxString& message)
	{
		Utilities::ExecuteOnMainThread([&]() {
			Console.Warning(message);
		});
	}

	bool mcd_sync(const wxString &caller)
    {
        if (caller == "host") {

            _dialog->SetStatus(wxT("Waiting for memory card synchronization..."));

            auto uncompressed_mcd = Utilities::ReadMCD(0, 0);
            if (_replay)
                _replay->Data(uncompressed_mcd);
            size_t mcd_size = Utilities::GetMCDSize(0, 0);
            Utilities::block_type compressed_mcd(mcd_size);
            if (g_Conf->Netplay.ReadonlyMemcard)
                _mcd_backup = uncompressed_mcd;
            if (!Utilities::Compress(uncompressed_mcd, compressed_mcd)) {
                ConsoleErrorMT(wxT("NETPLAY: Unable to compress MCD buffer."));
                return false;
            }

            {
                recursive_lock lock(_mutex);
                if (!_session || _session->state() != shoryu::MessageType::Ready)
                    return false;
                size_t blockSize = 128;
                for (size_t i = 0; i < compressed_mcd.size(); i += blockSize) {
                    shoryu::message_data data;
                    data.data_length = compressed_mcd.size() - i;
                    if (data.data_length > blockSize)
                        data.data_length = blockSize;
                    data.p.reset(new char[data.data_length]);
                    std::copy(compressed_mcd.data() + i, compressed_mcd.data() + i + data.data_length, data.p.get());
                    _session->queue_data(data);
                }
                shoryu::message_data data;
                data.data_length = 9;
                data.p.reset(new char[data.data_length]);
                char *blockEndMsg = "BLOCK_END";
                std::copy(blockEndMsg, blockEndMsg + data.data_length, data.p.get());
                _session->queue_data(data);
            }
        } else {
            _dialog->SetStatus(wxT("Synchronizing memory card with the host..."));

            _mcd_backup = Utilities::ReadMCD(0, 0);

            shoryu::msec timeout_timestamp = shoryu::time_ms() + 480000;
            size_t mcd_size = Utilities::GetMCDSize(0, 0);
            Utilities::block_type compressed_mcd(mcd_size);
            size_t pos = 0;
            while (true) {
                bool ready;
                shoryu::message_data data;
                {
                    recursive_lock lock(_mutex);
                    if (!_session || _session->end_session_request())
                        return false;
                    _session->send();
                    ready = _session->get_data(0, data, 50);
                }
                if (!ready) {
                    if (timeout_timestamp < shoryu::time_ms()) {
                        ConsoleErrorMT(wxT("NETPLAY: Timeout while synchronizing memory cards."));
                        return false;
                    }
                } else {
                    if (data.data_length != 9 || memcmp(data.p.get(), "BLOCK_END", 9) != 0) {
                        std::copy(data.p.get(), data.p.get() + data.data_length, compressed_mcd.data() + pos);
                        pos += data.data_length;
                    } else {
                        compressed_mcd.resize(pos);
                        Utilities::block_type uncompressed_mcd(mcd_size);

                        if (!Utilities::Uncompress(compressed_mcd, uncompressed_mcd)) {
                            ConsoleErrorMT(wxT("NETPLAY: Unable to decompress MCD buffer."));
                            return false;
                        }
                        if (uncompressed_mcd.size() != mcd_size) {
                            ConsoleErrorMT(wxT("NETPLAY: Invalid MCD received from host."));
                            return false;
                        }
                        Utilities::WriteMCD(0, 0, uncompressed_mcd);
                        if (_replay)
                            _replay->Data(uncompressed_mcd);
                        break;
                    }
                }
            }
        }
        return true;
    }

	bool Join(const wxString& ip, unsigned short port, int timeout)
	{
		std::unique_lock<std::mutex> connection_lock(_connection_mutex);

		// wait for HandleIO(), when game is ready for inputs
		// or in EndSession(), in case session is cancelled
		_ready_to_connect_cond.wait(connection_lock);

		if (auto state = Utilities::GetSyncState())
		{
			zed_net_address_t ep;
			zed_net_get_address(&ep, ip.ToAscii(), port);

			if(_replay)
				_replay->SyncState(*state);

			if(!_session || !_session->join(ep, *state,
				[&](const EmulatorSyncState& s1, const EmulatorSyncState& s2) -> bool
				{return CheckSyncStates(s1, s2);}, timeout))
				return false;

			_game_name = wxDateTime::Now().Format(wxT("[%Y.%m.%d %H-%M] "))  + wxT("[") + Utilities::GetCurrentDiscName() + wxT("]");

			{
				recursive_lock lock(_mutex);
				if(!_session || _session->state() != shoryu::MessageType::Ready)
					return false;

				// show chat window
				_dialog->OnConnectionEstablished(_session->delay());
                _dialog->SetStatus(wxT("Connected."));
			}

            // Wait for the mcd sync signal
            int delay = _session->wait_for_mcd();
            if (delay <= 0)
                return false;

            // Start the mcd sync
			mcd_sync("client");

            _dialog->SetStatus(wxT("Synchronized. Waiting for host..."));

            // Wait for the delay signal
            _session->wait_for_start(ep);

            _dialog->SetStatus(wxT("Game started."));
            return true;
		}
		return false;
	}

	bool Host()
	{
		std::unique_lock<std::mutex> connection_lock(_connection_mutex);

		// wait for HandleIO(), when game is ready for inputs
		// or in EndSession(), in case session is cancelled
		_ready_to_connect_cond.wait(connection_lock);

		if (auto state = Utilities::GetSyncState())
		{
            zed_net_address_t ep;
			if(_replay)
				_replay->SyncState(*state);

			// show chat window with default delay of 1
			_dialog->OnConnectionEstablished(1);

			if(!_session || !_session->create(g_Conf->Netplay.NumPlayers, *state,
				[&](const EmulatorSyncState& s1, const EmulatorSyncState& s2) -> bool
				{return CheckSyncStates(s1, s2);}))
				return false;

			_game_name = wxDateTime::Now().Format(wxT("[%Y.%m.%d %H-%M] "))  + wxT("[") + Utilities::GetCurrentDiscName() + wxT("]");

			{
				recursive_lock lock(_mutex);
				if(!_session || _session->state() != shoryu::MessageType::Ready)
					return false;
			}

			// Wait for start button to be pressed
			int delay = _dialog->WaitForConfirmation();
			if(delay <= 0)
				return false;

            // Send mcd sync signal
            _session->announce_mcd();

            // Start the mcd sync
            mcd_sync("host");

			// Start the mcd send loop
			mcd_send_thread.reset(new std::thread(&NetplayPlugin::mcd_send_loop, this));

			// Wait for the ready signal from all clients
            _session->wait_for_start(ep);

			_dialog->SetStatus(wxT("Game started."));

			{
				recursive_lock lock(_mutex);

				if(!_session || _session->state() != shoryu::MessageType::Ready)
					return false;

				if(delay != _session->delay())
					_session->delay(delay);

				// send delay to all clients
				_session->reannounce_delay();

			}
            return true;
		}
		return false;
	}

	void mcd_send_loop()
    {
        shoryu::msec timeout_timestamp = shoryu::time_ms() + 480000;
        while (true) {
            {
                recursive_lock lock(_mutex);
                if (!_session || _session->state() != shoryu::MessageType::Ready)
                    return;
                if (!_session->send())
                    break;
                if (_session->end_session_request())
                    return;
                if (_session->first_received_frame() != -1) {
                    _session->clear_queue();
                    break;
                }
            }
            if (timeout_timestamp < shoryu::time_ms()) {
                ConsoleErrorMT(wxT("NETPLAY: Timeout while synchonizing memory cards."));
                return;
            }
        }
    }

	void EndSession()
	{
		recursive_lock lock(_mutex);
		INetplayDialog* dialog = INetplayDialog::GetInstance();

		if(dialog->IsShown())
		{
			dialog->Close();
			_dialog = nullptr;
		}

		{
			if(_session)
			{
				if(_session->state() == shoryu::MessageType::Ready)
				{
					_session->send_end_session_request();
					int try_count = _session->delay() * 4;
					while(_session->send())
					{
						shoryu::sleep(17);
						if(try_count-- == 0)
							break;
					}
				}
				_session->shutdown();
				_session->unbind();
			}
		}

		// if we're connecting, notify and join the connecting thread
		// the thread should realize the session is dead and return
		if(_connect_thread)
		{
			_ready_to_connect_cond.notify_all();
			_connect_thread->join();
			_connect_thread.reset();
		}

		_session.reset();
	}

	void Stop()
	{
		_is_stopped = true;
		EndSession();
		Utilities::ExecuteOnMainThread([&]() {
			CoreThread.Reset();
		});
	}

	void NextFrame()
	{
		if(_is_stopped || !_session) return;

		_my_frame = Message();
		_session->next_frame();

		/*if(_session->last_error().length())
		{
			ConsoleErrorMT(wxT("NETPLAY: ") + wxString(_session->last_error().c_str(), wxConvLocal));
			_session->last_error("");
		}*/

		if(_state == SSReady)
		{
			// if there's still an errant connecting thread, kill it
			if(_connect_thread)
			{
				_connect_thread->detach();
				_connect_thread.reset();
			}
			_state = SSRunning;
		}
	}

	// called when IOPHook has a frame ready to send
	void AcceptInput(int side)
	{
		if(_is_stopped || !_session) return;

		try
		{
			_session->set(_my_frame);
		}
		catch(std::exception& e)
		{
			Stop();
			ConsoleErrorMT(wxT("NETPLAY: ") + wxString(e.what(), wxConvLocal) + wxT(". Interrupting session."));
		}

		if(_replay)
		{
			Message f;
			_session->get(side, f, 0);
			_replay->Write(side, f);
		}
	}

	int RemapVibrate(int pad)
	{
		if (_is_stopped || !_session) return pad;

		// FIXME: use pad input, right now this only remaps pad 0
		if (pad == 0)
			return _session->side();
		else
			return -1;
	}

	// called by IOPHook when a pad needs IO
	u8 HandleIO(int side, int index, u8 value)
	{
		if(_is_stopped || !_session) return value;

		// wait for session to start or be cancelled
		{
			int delay = _session->delay();

			if(_state == SSNone)
				_ready_to_connect_cond.notify_one();

			while(_state == SSNone)
			{
				{
					recursive_lock lock(_mutex);
					if(!_session || _session->end_session_request())
					{
						lock.unlock();
						Stop();
						break;
					}

					if(delay != _session->delay())
					{
						delay = _session->delay();
						_dialog->SetInputDelay(delay);
					}
				}

				// FIXME: this delays connection by up to 150ms
				// use a signal or something instead
				// fixing this will require fixing the early frame before everyone sends ready though
				shoryu::sleep(150);
			}
		}

		if( _state == SSCancelled && !_is_stopped )
		{
			Stop();
		}

		if( _session && _session->end_session_request() && !_is_stopped )
		{
			auto frame = _session->frame();
			Stop();
			ConsoleWarningMT(wxString::Format(wxT("NETPLAY: Session ended on frame %d."), (int)frame));
		}

		if(_is_stopped || !_session) return value;

		Message frame;

		// ignore unassigned pads
		if (side >= _session->num_players())
			return frame.input[index];

		// record local player inputs
		if(side == 0)
			_my_frame.input[index] = value;

		// wait up to 10 seconds for input
		// this is probably overkill, but you never know
		auto timeout = shoryu::time_ms() + 10000;
		try
		{
			while(true)
			{
				auto until_timeout = timeout - shoryu::time_ms();
				if (until_timeout < 1)
					until_timeout = 1;

				if (_session->get(side, frame, until_timeout))
					break;

				_session->send();
				if(_session->end_session_request())
					break;
				if(timeout <= shoryu::time_ms())
				{
					auto frame = _session->frame();
					Stop();
					ConsoleErrorMT(wxString::Format(wxT("NETPLAY: Timeout on frame %d."), (int)frame));
					break;
				}
#ifdef CONNECTION_TEST
				shoryu::sleep(500);
#endif
			}
		}
		catch(std::exception& e)
		{
			Stop();
			ConsoleErrorMT(wxT("NETPLAY: ") + wxString(e.what(), wxConvLocal));
		}

		value = frame.input[index];
		return value;
	}

	void SendChatText(const std::string &message)
	{
		_session->send_chatmessage(message);
	}

protected:
	bool CheckSyncStates(const EmulatorSyncState& s1, const EmulatorSyncState& s2)
	{
		if(memcmp(s1.biosVersion, s2.biosVersion, sizeof(s1.biosVersion)))
		{
			ConsoleErrorMT(wxT("NETPLAY: Bios version mismatch."));
			return false;
		}

		if(memcmp(s1.discId, s2.discId, sizeof(s1.discId)))
		{
			size_t s1discIdLen = sizeof(s1.discId);
			size_t s2discIdLen = sizeof(s2.discId);

			for(size_t i = 0; i < s1discIdLen; i++)
			{
				if(s1.discId[i] == 0)
				{
					s1discIdLen = i;
					break;
				}
			}

			for(size_t i = 0; i < s2discIdLen; i++)
			{
				if(s2.discId[i] == 0)
				{
					s2discIdLen = i;
					break;
				}
			}

			wxString s1discId(s1.discId, wxConvUTF8, s1discIdLen);
			wxString s2discId(s2.discId, wxConvUTF8, s2discIdLen);

			ConsoleErrorMT(wxT("NETPLAY: You are trying to boot different games: ") + 
				Utilities::GetDiscNameById(s1discId) + wxT(" and ") + 
				Utilities::GetDiscNameById(s2discId));
			return false;
		}

		if(s1.skipMpeg != s2.skipMpeg)
		{
			ConsoleErrorMT(wxT("NETPLAY: SkipMpegHack settings mismatch."));
			return false;
		}

		return true;
	}
	
	enum SessionState
	{
		SSNone,
		SSCancelled,
		SSReady,
		SSRunning
	} _state;

	bool _is_initialized;
	bool _is_stopped;
	std::condition_variable _ready_to_connect_cond;
    std::unique_ptr<std::thread> mcd_send_thread;
	std::mutex _connection_mutex;
	wxString _game_name;
	Message _my_frame;
	Utilities::block_type _mcd_backup;
	std::shared_ptr<Replay> _replay;
	INetplayDialog* _dialog;
	std::recursive_mutex _mutex;
	typedef std::unique_lock<std::recursive_mutex> recursive_lock;
};

INetplayPlugin* INetplayPlugin::instance = nullptr;

INetplayPlugin& INetplayPlugin::GetInstance()
{
	if(!instance)
		instance = new NetplayPlugin();
	return *instance;
}
