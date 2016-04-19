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

static edict_t *ent;
static usercmd_t ucmd;
static bool running = false;
static char buf[524288];
static int bi;
static int command;
static float args[6];

static float angles[2];
static int moves[3];
static int buttons;

static void G_ScriptRun_SetAngle( int n, float angle )
{
	ucmd.angles[n] = ANGLE2SHORT( angle ) - ent->r.client->ps.pmove.delta_angles[n];
}

static void G_ScriptRun_Terminate_f()
{
	trap_Cvar_ForceSet( "g_scriptRun", "0" );
}

static void G_ScriptRun_Wait_f()
{
	if( args[0] )
		args[0]--;
	else
		command = 0;
}

static void G_ScriptRun_Angles_f()
{
	angles[0] = args[0];
	angles[1] = args[1];
	command = 0;
}

static void G_ScriptRun_Forward_f()
{
	moves[0] = ( int )( args[0] );
	command = 0;
}

static void G_ScriptRun_Up_f()
{
	moves[1] = ( int )( args[0] );
	command = 0;
}

static void G_ScriptRun_Side_f()
{
	moves[2] = ( int )( args[0] );
	command = 0;
}

static void G_ScriptRun_Button_f()
{
	buttons |= ( int )( args[0] );
	command = 0;
}

static void G_ScriptRun_UnButton_f()
{
	buttons &= ~( int )( args[0] );
	command = 0;
}

static void ( *scriptFunctions[] )( void ) = {
	G_ScriptRun_Terminate_f,
	G_ScriptRun_Wait_f,
	G_ScriptRun_Angles_f,
	G_ScriptRun_Forward_f,
	G_ScriptRun_Up_f,
	G_ScriptRun_Side_f,
	G_ScriptRun_Button_f,
	G_ScriptRun_UnButton_f
};

static const char *names[] = {
	"terminate",
	"wait",
	"angles",
	"forward",
	"up",
	"side",
	"button",
	"unbutton",
	NULL
};

int G_ScriptRun_LoadCommand( void )
{
	if( command && g_scriptRun->integer > 0 )
		return command;

	if( !running || g_scriptRun->integer < 0 )
	{
		memset( angles, 0, sizeof( angles ) );
		memset( moves, 0, sizeof( moves ) );
		buttons = 0;

		int filenum;
		int length = trap_FS_FOpenFile( va( "scriptruns/%s", level.mapname ), &filenum, FS_READ );
		if( length == -1 )
			return 0;
		trap_FS_Read( buf, length, filenum );
		trap_FS_FCloseFile( filenum );
		bi = 0;
		if( g_scriptRun->integer < 0 )
			trap_Cvar_ForceSet( "g_scriptRun", "1" );
		running = true;
	}

	if( !buf[bi] )
		return 0;

	int next = bi;
	while( buf[next] && buf[next] != '\n' && buf[next] != '\r' )
		next++;
	buf[next++] = 0;
	while( buf[next] == '\n' || buf[next] == '\r' )
		next++;

	const char *p = &buf[bi];
	const char *s = COM_Parse( &p );
	command = atoi( s );
	if( !command )
	{
		for( int i = 0; names[i]; i++ )
		{
			if( !strcmp( names[i], s ) )
			{
				command = i;
				break;
			}
		}
	}
	memset( args, 0, sizeof( args ) );
	for( int i = 0; i < 6; i++ )
	{
		char *q = COM_Parse( &p );
		if( q )
			args[i] = atof( q );
		else
			break;
	}
	bi = next;

	return command;
}

void G_ScriptRun( edict_t *new_ent )
{
	ent = new_ent;
	memset( &ucmd, 0, sizeof( ucmd ) );
	ucmd.msec = game.frametime;
	ucmd.serverTimeStamp = game.serverTime;

	bool repeat;
	do
	{
		command = G_ScriptRun_LoadCommand();
		running = command != 0;
		repeat = running;
		scriptFunctions[command]();
		repeat &= command == 0;
	}
	while( repeat );

	G_ScriptRun_SetAngle( 0, angles[0] );
	G_ScriptRun_SetAngle( 1, angles[1] );
	ucmd.buttons = buttons;
	ucmd.forwardmove = moves[0];
	ucmd.upmove = moves[1];
	ucmd.sidemove = moves[2];

	ClientThink( ent, &ucmd, 0 );
}
