/*
Copyright (C) 2016 Gerco van Heerdt

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

#include "g_local.h"

cvar_t *g_scriptRun;

static usercmd_t ucmd;
static bool running = false;
static char buf[524288];
static int bi;
static int command;
static float args[6];

static void ( *scriptFunctions[] )( void ) = {
	NULL
};

void G_ScriptRun_LoadCommand( void )
{
	if( command && g_scriptRun->integer > 0 )
		return;

	if( !running || g_scriptRun->integer < 0 )
	{
		int filenum;
		int length = trap_FS_FOpenFile( va( "scriptruns/%s", level.mapname ), &filenum, FS_READ );
		if( length == -1 )
			return;
		trap_FS_Read( buf, length, filenum );
		trap_FS_FCloseFile( filenum );
		bi = 0;
		if( g_scriptRun->integer < 0 )
			trap_Cvar_ForceSet( "g_scriptRun", "1" );
	}

	if( !buf[bi] )
		return;

	int next = bi;
	while( buf[next] && buf[next] != '\n' && buf[next] != '\r' )
		next++;
	buf[next++] = 0;
	while( buf[next] == '\n' || buf[next] == '\r' )
		next++;

	const char *p = &buf[bi];
	command = atoi( COM_Parse( &p ) );
	for( int i = 0; i < 6; i++ )
	{
		char *q = COM_Parse( &p );
		if( q )
		{
			args[i] = atof( q );
		}
		else
		{
			args[i] = 0;
			break;
		}
	}
	bi = next;
}

void G_ScriptRun( edict_t *ent )
{
	memset( &ucmd, 0, sizeof( ucmd ) );

	bool repeat = true;
	while( repeat )
	{
		if( !command )
			repeat = false;
		G_ScriptRun_LoadCommand();
		if( command )
			scriptFunctions[command]();
		repeat &= command == 0;
	}

	ucmd.msec = game.frametime;
	ucmd.serverTimeStamp = game.serverTime;
	ClientThink( ent, &ucmd, 0 );
}
