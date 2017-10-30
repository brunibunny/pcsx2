/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "MainFrame.h"
#include "GSFrame.h"

// General Notes:
//  * It's very important that we re-discover menu items by ID every time we change them,
//	because the modern era of configurable GUIs means that we can't be assured the IDs
//	exist anymore.


// This is necessary because this stupid wxWidgets thing has implicit debug errors
// in the FindItem call that asserts if the menu options are missing.  This is bad
// mojo for configurable/dynamic menus. >_<
void MainEmuFrame::EnableMenuItem(int id, bool enable)
{
	if (wxMenuItem *item = m_menubar.FindItem(id))
		item->Enable(enable);
}

void MainEmuFrame::SetMenuItemLabel(int id, wxString str)
{
	if (wxMenuItem *item = m_menubar.FindItem(id))
		item->SetItemLabel(str);
}

static void _SaveLoadStuff(bool enabled)
{
	sMainFrame.EnableMenuItem(MenuId_Sys_LoadStates, enabled);
	sMainFrame.EnableMenuItem(MenuId_Sys_SaveStates, enabled);

#ifdef USE_NEW_SAVESLOTS_UI
	// Run though all the slots.Update if they need updating or the crc changed.
	for (int i = 0; i < 10; i++)
	{
		int load_menu_item = MenuId_State_Load01 + i + 1;
		int save_menu_item = MenuId_State_Save01 + i + 1;
		
		// We need to reload the file information if the crc or serial # changed.
		if ((saveslot_cache[i].crc != ElfCRC)|| (saveslot_cache[i].serialName != DiscSerial)) saveslot_cache[i].invalid_cache = true;

		// Either the cache needs updating, or the menu items do, or both.
		if (saveslot_cache[i].menu_update || saveslot_cache[i].invalid_cache)
		{
			#ifdef SAVESLOT_LOGS
			Console.WriteLn("Updating slot %i.", i);
			if (saveslot_cache[i].menu_update) Console.WriteLn("Menu update needed.");
			if (saveslot_cache[i].invalid_cache) Console.WriteLn("Invalid cache. (CRC different or just initialized.)");
			#endif

			if (saveslot_cache[i].invalid_cache)
			{
				// Pull everything from disk.
				saveslot_cache[i].UpdateCache();

				#ifdef SAVESLOT_LOGS
				saveslot_cache[i].ConsoleDump();
				#endif
			}

			// Update from the cached information.
			saveslot_cache[i].menu_update = false;
			saveslot_cache[i].crc = ElfCRC;

			sMainFrame.EnableMenuItem(load_menu_item, !saveslot_cache[i].empty);
			sMainFrame.SetMenuItemLabel(load_menu_item, saveslot_cache[i].SlotName());
			sMainFrame.SetMenuItemLabel(save_menu_item, saveslot_cache[i].SlotName());
		}

	}
	Sstates_updateLoadBackupMenuItem(false);
#endif
}

// Updates the enable/disable status of all System related controls: menus, toolbars,
// etc.  Typically called by SysEvtHandler whenever the message pump becomes idle.
void UI_UpdateSysControls()
{
	#ifdef SAVESLOT_LOGS
	Console.WriteLn("In the routine for updating the UI.");
	#endif

	if (wxGetApp().Rpc_TryInvokeAsync(&UI_UpdateSysControls))
		return;

	sApp.PostAction(CoreThreadStatusEvent(CoreThread_Indeterminate));

	_SaveLoadStuff(SysHasValidState());
}

void UI_DisableSysShutdown()
{
	if (wxGetApp().Rpc_TryInvokeAsync(&UI_DisableSysShutdown))
		return;

	sMainFrame.EnableMenuItem(MenuId_Sys_Shutdown, false);
	sMainFrame.EnableMenuItem(MenuId_IsoBrowse, !g_Conf->AskOnBoot);
	wxGetApp().GetRecentIsoManager().EnableItems(!g_Conf->AskOnBoot);
}

void UI_EnableSysShutdown()
{
	if (wxGetApp().Rpc_TryInvokeAsync(&UI_EnableSysShutdown))
		return;

	sMainFrame.EnableMenuItem(MenuId_Sys_Shutdown, true);
}


void UI_DisableSysActions()
{
	if (wxGetApp().Rpc_TryInvokeAsync(&UI_DisableSysActions))
		return;

	sMainFrame.EnableMenuItem(MenuId_Sys_Shutdown, false);

	_SaveLoadStuff(false);
}

void UI_EnableSysActions()
{
	if (wxGetApp().Rpc_TryInvokeAsync(&UI_EnableSysActions))
		return;

	sMainFrame.EnableMenuItem(MenuId_Sys_Shutdown, true);
	sMainFrame.EnableMenuItem(MenuId_IsoBrowse, true);
	wxGetApp().GetRecentIsoManager().EnableItems(true);

	_SaveLoadStuff(true);
}

void UI_DisableStateActions()
{
	if (wxGetApp().Rpc_TryInvokeAsync(&UI_DisableStateActions))
		return;

	_SaveLoadStuff(false);
}

void UI_EnableStateActions()
{
	if( wxGetApp().Rpc_TryInvokeAsync( &UI_EnableStateActions ) ) return;
	_SaveLoadStuff( true );
}

// Disanble MenuItems that can disrupt netplay
void UI_DisableEverything()
{
	sMainFrame.EnableMenuItem( MenuId_Config_SysSettings, false );
	sMainFrame.EnableMenuItem( MenuId_Config_McdSettings, false );
	sMainFrame.EnableMenuItem( MenuId_Config_AppSettings, false );
	sMainFrame.EnableMenuItem( MenuId_Config_GameDatabase, false );
	sMainFrame.EnableMenuItem( MenuId_Config_BIOS, 	false );
	sMainFrame.EnableMenuItem( MenuId_Config_Language, false );
	sMainFrame.EnableMenuItem( MenuId_Config_ResetAll, false );
	sMainFrame.EnableMenuItem( MenuId_Config_Multitap0Toggle, false );
	sMainFrame.EnableMenuItem( MenuId_Config_Multitap1Toggle, false );
	sMainFrame.EnableMenuItem( MenuId_Video_WindowSettings, false );
	sMainFrame.EnableMenuItem( MenuId_Video_CoreSettings, 	false );
	for( int i = 0; i < PluginId_Count; ++i )
	{
		sMainFrame.EnableMenuItem( MenuId_Config_GS+i, false );
		sMainFrame.EnableMenuItem( MenuId_PluginBase_Settings + (i*PluginMenuId_Interval), false );
	}
	for( int i = 0; i < 3; ++i )
		sMainFrame.EnableMenuItem(MenuId_Src_Iso+i, false );

	sMainFrame.EnableMenuItem( MenuId_Boot_CDVD, false );
	sMainFrame.EnableMenuItem( MenuId_Boot_CDVD2, false );
	sMainFrame.EnableMenuItem( MenuId_Boot_Net, false );
	sMainFrame.EnableMenuItem( MenuId_Boot_Replay, false );
	sMainFrame.EnableMenuItem( MenuId_Boot_ELF, false );
	sMainFrame.EnableMenuItem( MenuId_IsoBrowse, false );
	sMainFrame.EnableMenuItem( MenuId_EnableBackupStates, false );
	sMainFrame.EnableMenuItem( MenuId_EnablePatches, false );
	sMainFrame.EnableMenuItem( MenuId_EnableCheats, false );
	sMainFrame.EnableMenuItem( MenuId_EnableHostFs, false );
	sMainFrame.EnableMenuItem( MenuId_Sys_SuspendResume, false );

	for( int i = 0; i < 10; ++i )
		sMainFrame.EnableMenuItem( MenuId_State_Save01 + i + 1, false );
	for( int i = 0; i < 10; ++i )
		sMainFrame.EnableMenuItem( MenuId_State_Load01 + i + 1, false );
	sMainFrame.EnableMenuItem( MenuId_State_LoadBackup, false );
}

void UI_EnableEverything()
{
	sMainFrame.EnableMenuItem( MenuId_Config_SysSettings, true );
	sMainFrame.EnableMenuItem( MenuId_Config_McdSettings, true );
	sMainFrame.EnableMenuItem( MenuId_Config_AppSettings, true );
	sMainFrame.EnableMenuItem( MenuId_Config_GameDatabase, true );
	sMainFrame.EnableMenuItem( MenuId_Config_BIOS, 	true );
	sMainFrame.EnableMenuItem( MenuId_Config_Language, true );
	sMainFrame.EnableMenuItem( MenuId_Config_ResetAll, true );
	sMainFrame.EnableMenuItem( MenuId_Config_Multitap0Toggle, true );
	sMainFrame.EnableMenuItem( MenuId_Config_Multitap1Toggle, true );
	sMainFrame.EnableMenuItem( MenuId_Video_WindowSettings, true );
	sMainFrame.EnableMenuItem( MenuId_Video_CoreSettings, 	true );
	for( int i = 0; i < PluginId_Count; ++i )
	{
		sMainFrame.EnableMenuItem( MenuId_Config_GS+i, true );
		sMainFrame.EnableMenuItem( MenuId_PluginBase_Settings + (i*PluginMenuId_Interval), true );
	}
	for( int i = 0; i < 3; ++i )
		sMainFrame.EnableMenuItem(MenuId_Src_Iso+i, true );

	sMainFrame.EnableMenuItem( MenuId_Boot_CDVD, true );
	sMainFrame.EnableMenuItem( MenuId_Boot_CDVD2, true );
	sMainFrame.EnableMenuItem( MenuId_Boot_Net, true );
	sMainFrame.EnableMenuItem( MenuId_Boot_Replay, true );
	sMainFrame.EnableMenuItem( MenuId_Boot_ELF, true );
	sMainFrame.EnableMenuItem( MenuId_IsoBrowse, true );
	sMainFrame.EnableMenuItem( MenuId_EnableBackupStates, true );
	sMainFrame.EnableMenuItem( MenuId_EnablePatches, true );
	sMainFrame.EnableMenuItem( MenuId_EnableCheats, true );
	sMainFrame.EnableMenuItem( MenuId_EnableHostFs, true );
	sMainFrame.EnableMenuItem( MenuId_Sys_SuspendResume, true );

	for( int i = 0; i < 10; ++i )
		sMainFrame.EnableMenuItem( MenuId_State_Save01 + i + 1, true );
	for( int i = 0; i < 10; ++i )
		sMainFrame.EnableMenuItem( MenuId_State_Load01 + i + 1, true );
	sMainFrame.EnableMenuItem( MenuId_State_LoadBackup, true );
}
