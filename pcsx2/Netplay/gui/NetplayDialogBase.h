///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>
#include <wx/statbox.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class NetplayDialogBase
///////////////////////////////////////////////////////////////////////////////
class NetplayDialogBase : public wxFrame
{
	private:

	protected:
		wxStaticText* m_statusText;
		wxBoxSizer* m_contentSizer;
		wxStdDialogButtonSizer* m_dialogButtonSizer;
		wxButton* m_dialogButtonSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		NetplayDialogBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Netplay - PCSX2"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 250,-1 ), long style = wxCAPTION|wxCLOSE_BOX|wxSYSTEM_MENU );

		~NetplayDialogBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class NetplaySettingsPanelBase
///////////////////////////////////////////////////////////////////////////////
class NetplaySettingsPanelBase : public wxPanel
{
	private:

	protected:
		wxStaticText* m_usernameLabel;
		wxTextCtrl* m_usernameTextCtrl;
		wxCheckBox* m_saveReplayCheckBox;
		wxNotebook* m_netplayModeNotebook;
		wxPanel* m_connect;
		wxStaticText* m_hostAddressLabel;
		wxTextCtrl* m_hostAddressTextCtrl;
		wxStaticText* m_hostPortLabel;
		wxSpinCtrl* m_hostPortSpinCtrl;
		wxCheckBox* m_observeCheckBox;
		wxButton* m_connectButton;
		wxPanel* m_host;
		wxStaticText* m_hostPortLabel1;
		wxSpinCtrl* m_listenPortSpinCtrl;
		wxStaticText* m_hostNumPlayersLabel;
		wxSpinCtrl* m_hostNumPlayersSpinCtrl;
		wxCheckBox* m_ClientOnlyDelayCheckBox;
		wxCheckBox* m_MCDSyncCheckBox;
		wxCheckBox* m_readOnlyMCDCheckBox;
		wxButton* m_hostButton;

		// Virtual event handlers, overide them in your derived class
		virtual void OnConnect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHost( wxCommandEvent& event ) { event.Skip(); }


	public:

		NetplaySettingsPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 295,299 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~NetplaySettingsPanelBase();

};

///////////////////////////////////////////////////////////////////////////////
/// Class NetplayLobbyPanelBase
///////////////////////////////////////////////////////////////////////////////
class NetplayLobbyPanelBase : public wxPanel
{
	private:

	protected:
		wxTextCtrl* m_NetplayChatTextCtrl;
		wxTextCtrl* m_NetplayConsoleEntryTextCtrl;
		wxButton* m_sendButton;
		wxListCtrl* m_playersBox;
		wxButton* m_moveUpButton;
		wxButton* m_moveDownButton;
		wxButton* m_startButton;
		wxStaticText* m_inputDelayLabel;
		wxSpinCtrl* m_inputDelaySpinner;

		// Virtual event handlers, overide them in your derived class
		virtual void OnSendText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnStart( wxCommandEvent& event ) { event.Skip(); }


	public:

		NetplayLobbyPanelBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 530,336 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~NetplayLobbyPanelBase();

};

