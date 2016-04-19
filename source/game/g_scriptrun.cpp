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
static int iargs[6];

static float angles[2];
static int moves[3];
static int buttons;
static int strafe;
static float strafeOffset;

static void G_ScriptRun_SetAngle( int n, float angle )
{
	while( angle > 180 )
		angle -= 360.0f;
	while( angle < -180 )
		angle += 360.0f;
	ucmd.angles[n] = ANGLE2SHORT( angle ) - ent->r.client->ps.pmove.delta_angles[n];
}

static float G_ScriptRun_HSpeed()
{
	vec3_t hvel;
	VectorCopy( ent->r.client->ps.pmove.velocity, hvel );
	hvel[2] = 0;
	return VectorLength( hvel );
}


static float G_ScriptRun_MoveAngle()
{
	vec3_t hvel;
	vec3_t an;
	VectorCopy( ent->r.client->ps.pmove.velocity, hvel );
	hvel[2] = 0;
	VecToAngles( hvel, an );
	return an[YAW];
}

static float G_ScriptRun_BestAccelAngle( float offset )
{
	float result = acos( ( 320.0f - 0.320f * ucmd.msec ) / G_ScriptRun_HSpeed() ) * 180.0f / M_PI - offset;
	if( result > 0 )
		return result;
	return 0;
}

static float G_ScriptRun_StrafeAngle()
{
	return G_ScriptRun_BestAccelAngle( 45 );
}

static float G_ScriptRun_BunnyAngle()
{
	return G_ScriptRun_BestAccelAngle( 90 );
}

static void G_ScriptRun_Terminate_f()
{
	trap_Cvar_ForceSet( "g_scriptRun", "0" );
}

static void G_ScriptRun_Wait_f()
{
	if( iargs[0] )
		iargs[0]--;
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
	moves[0] = iargs[0];
	command = 0;
}

static void G_ScriptRun_Up_f()
{
	moves[1] = iargs[0];
	command = 0;
}

static void G_ScriptRun_Side_f()
{
	moves[2] = iargs[0];
	command = 0;
}

static void G_ScriptRun_Button_f()
{
	buttons |= iargs[0];
	command = 0;
}

static void G_ScriptRun_UnButton_f()
{
	buttons &= ~iargs[0];
	command = 0;
}

static void G_ScriptRun_Weapon_f()
{
	ent->r.client->ps.stats[STAT_PENDING_WEAPON] = iargs[0];
	command = 0;
}

static void G_ScriptRun_Strafe_f()
{
	strafe = iargs[0];
	strafeOffset = args[1];
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
	G_ScriptRun_UnButton_f,
	G_ScriptRun_Weapon_f,
	G_ScriptRun_Strafe_f
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
	"weapon",
	"strafe",
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
		strafe = 0;
		strafeOffset = 0;

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
	memset( iargs, 0, sizeof( iargs ) );
	for( int i = 0; i < 6; i++ )
	{
		char *q = COM_Parse( &p );
		if( q )
		{
			args[i] = atof( q );
			iargs[i] = atoi( q );
		}
		else
		{
			break;
		}
	}
	bi = next;

	return command;
}

bool G_ScriptRun_Strafe( void )
{
	if( strafe && moves[2] && G_ScriptRun_HSpeed() >= 320 )
		G_ScriptRun_SetAngle( YAW, G_ScriptRun_MoveAngle()
				+ ( ( moves[0] == 0 ? G_ScriptRun_BunnyAngle() : G_ScriptRun_StrafeAngle() ) + strafeOffset )
					* ( ( moves[2] < 0 ) ^ ( moves[0] < 0 ) ? 1 : -1 )
				+ ( moves[0] < 0 ? 180 : 0 ) );
	else
		return false;
	return true;
}

void G_ScriptRun_Apply( void )
{
	G_ScriptRun_SetAngle( 0, angles[0] );
	if( !G_ScriptRun_Strafe() )
		G_ScriptRun_SetAngle( 1, angles[1] );
	ucmd.buttons = buttons;
	ucmd.forwardmove = moves[0];
	ucmd.upmove = moves[1];
	ucmd.sidemove = moves[2];
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

	G_ScriptRun_Apply();

	ClientThink( ent, &ucmd, 0 );
}
