/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "ui_precompiled.h"
#include "kernel/ui_syscalls.h"

namespace WSWUI
{
	ui_import_t UI_IMPORT;

	// if API is different, the dll cannot be used
	int API( void )
	{
		return UI_API_VERSION;
	}

	void Init( int vidWidth, int vidHeight, float pixelRatio,
		int protocol, const char *demoExtension, const char *basePath )
	{
	}

	void Shutdown( void )
	{
	}

	void TouchAllAssets( void )
	{
	}

	void Refresh( unsigned int time, int clientState, int serverState, 
		bool demoPlaying, const char *demoName, bool demoPaused, unsigned int demoTime, 
		bool backGround, bool showCursor )
	{
	}

	void UpdateConnectScreen( const char *serverName, const char *rejectmessage, 
		int downloadType, const char *downloadfilename, float downloadPercent, int downloadSpeed, 
		int connectCount, bool backGround )
	{
	}

	void Keydown( int context, int key )
	{
	}

	void Keyup( int context, int key )
	{
	}

	void CharEvent( int context, wchar_t key )
	{
	}

	void MouseMove( int context, int dx, int dy )
	{
	}

	void MouseSet( int context, int mx, int my, bool showCursor )
	{
	}

	bool TouchEvent( int context, int id, touchevent_t type, int x, int y )
	{
		return false;
	}

	bool IsTouchDown( int context, int id )
	{
		return false;
	}

	void CancelTouches( int context )
	{
	}

	void ForceMenuOff( void )
	{
	}

	void ShowQuickMenu( bool show )
	{
	}

	bool HaveQuickMenu( void )
	{
		return false;
	}

	void AddToServerList( const char *adr, const char *info )
	{
	}
}	// namespace

//=================================

ui_export_t *GetUIAPI( ui_import_t *import )
{
	static ui_export_t globals;

	// Trap::UI_IMPORT = *import;
	WSWUI::UI_IMPORT = *import;

	globals.API = WSWUI::API;

	globals.Init = WSWUI::Init;
	globals.Shutdown = WSWUI::Shutdown;

	globals.TouchAllAssets = WSWUI::TouchAllAssets;

	globals.Refresh = WSWUI::Refresh;
	globals.UpdateConnectScreen = WSWUI::UpdateConnectScreen;

	globals.Keydown = WSWUI::Keydown;
	globals.Keyup = WSWUI::Keyup;
	globals.CharEvent = WSWUI::CharEvent;
	globals.MouseMove = WSWUI::MouseMove;
	globals.MouseSet = WSWUI::MouseSet;
	globals.TouchEvent = WSWUI::TouchEvent;
	globals.IsTouchDown = WSWUI::IsTouchDown;
	globals.CancelTouches = WSWUI::CancelTouches;

	globals.ForceMenuOff = WSWUI::ForceMenuOff;
	globals.ShowQuickMenu = WSWUI::ShowQuickMenu;
	globals.HaveQuickMenu = WSWUI::HaveQuickMenu;

	globals.AddToServerList = WSWUI::AddToServerList;

	return &globals;
}

#ifndef UI_HARD_LINKED
#include <stdarg.h>

// this is only here so the functions in q_shared.c and q_math.c can link
void Sys_Error( const char *format, ... )
{
	va_list	argptr;
	char msg[3072];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap::Error( msg );
}

void Com_Printf( const char *format, ... )
{
	va_list	argptr;
	char msg[3072];

	va_start( argptr, format );
	Q_vsnprintfz( msg, sizeof( msg ), format, argptr );
	va_end( argptr );

	trap::Print( msg );
}
#endif

#if defined(HAVE_DLLMAIN) && !defined(UI_HARD_LINKED)
int WINAPI DLLMain( void *hinstDll, unsigned long dwReason, void *reserved )
{
	return 1;
}
#endif
