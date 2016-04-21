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

typedef struct {
	int id;
	float args[6];
	int iargs[6];
} scriptcmd_t;

static edict_t *ent;
static usercmd_t ucmd;

static bool running = false;
static char buf[524288];
static int bi;
static scriptcmd_t command;
static int record;
static int replay;
static scriptcmd_t recordings[4096];

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
	float result = acos( ent->r.client->ps.pmove.stats[PM_STAT_MAXSPEED] * ( 1 - 0.001f * ucmd.msec ) / G_ScriptRun_HSpeed() ) * 180.0f / M_PI - offset;
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

static void G_ScriptRun_Nop_f()
{
	command.id = 0;
}

static void G_ScriptRun_Wait_f()
{
	if( command.iargs[0] )
		command.iargs[0]--;
	else
		command.id = 0;
}

static void G_ScriptRun_WaitFrame_f()
{
	if( command.iargs[0] == ucmd.msec )
		command.id = 0;
	else
		Com_Printf( "frametime %u\n", ucmd.msec );
}

static void G_ScriptRun_WaitAttr_f()
{
	float ref = 0;
	switch( command.iargs[0] )
	{
		case 0:
		case 1:
		case 2:
			ref = ent->r.client->ps.pmove.velocity[command.iargs[0]];
			break;
		case 3:
		case 4:
		case 5:
			ref = ent->r.client->ps.pmove.origin[command.iargs[0] - 3];
			break;
		case 6:
		case 7:
		case 8:
			vec3_t end;
			VectorCopy( ent->r.client->ps.pmove.velocity, end );
			if( command.iargs[0] == 6 )
			{
				end[2] = 0;
			}
			else if( command.iargs[0] == 7 )
			{
				end[0] = end[1] = 0;
			}
			float l = VectorNormalize( end );
			if( l != 0 )
			{
				VectorMA( ent->r.client->ps.pmove.origin, command.args[2] + 1, end, end );
				trace_t trace;
				G_Trace( &trace, ent->r.client->ps.pmove.origin, ent->r.mins, ent->r.maxs, end, ent, MASK_PLAYERSOLID );
				if( !trace.allsolid )
					ref = trace.fraction * ( command.args[2] + 1 );
			}
			break;
	}
	if( ( command.iargs[1] < 0 && ref < command.args[2] ) || ( command.iargs[1] > 0 && ref > command.args[2] ) )
		command.id = 0;
}

static void G_ScriptRun_Print_f()
{
	Com_Printf( "origin %f %f %f\n", ent->r.client->ps.pmove.origin[0], ent->r.client->ps.pmove.origin[1], ent->r.client->ps.pmove.origin[2] );
	Com_Printf( "angles %f %f\n", ent->r.client->ps.viewangles[0], ent->r.client->ps.viewangles[1] );
	Com_Printf( "velocity %f %f %f\n", ent->r.client->ps.pmove.velocity[0], ent->r.client->ps.pmove.velocity[1], ent->r.client->ps.pmove.velocity[2] );
	command.id = 0;
}

static void G_ScriptRun_Record_f()
{
	record = command.iargs[0] + 1;
	command.id = 0;
}

static void G_ScriptRun_Stop_f()
{
	replay = 0;
	command.id = 0;
}

static void G_ScriptRun_Replay_f()
{
	replay = command.iargs[0] + 1;
	command.id = 0;
}

static void G_ScriptRun_Angles_f()
{
	if( command.iargs[2] <= 1 )
	{
		angles[0] = command.args[0];
		angles[1] = command.args[1];
		command.id = 0;
	}
	else
	{
		angles[0] = command.args[0] / command.iargs[2] + ( command.iargs[2] - 1 ) * angles[0] / command.iargs[2];
		angles[1] = command.args[1] / command.iargs[2] + ( command.iargs[2] - 1 ) * angles[1] / command.iargs[2];
		command.iargs[2]--;
	}
}

static void G_ScriptRun_Move_f( int n )
{
	moves[n] = command.iargs[0];
	if( moves[n] < -1 )
		moves[n] = -1;
	else if( moves[n] > 1 )
		moves[n] = 1;
	command.id = 0;
}

static void G_ScriptRun_Forward_f()
{
	G_ScriptRun_Move_f( 0 );
}

static void G_ScriptRun_Up_f()
{
	G_ScriptRun_Move_f( 1 );
}

static void G_ScriptRun_Side_f()
{
	G_ScriptRun_Move_f( 2 );
}

static void G_ScriptRun_Button_f()
{
	buttons |= command.iargs[0];
	command.id = 0;
}

static void G_ScriptRun_UnButton_f()
{
	buttons &= ~command.iargs[0];
	command.id = 0;
}

static void G_ScriptRun_Use_f()
{
	gsitem_t *it;

	it = GS_Cmd_UseItem( &ent->r.client->ps, va( "%d", command.iargs[0] ), 0 );
	if( it )
		G_UseItem( ent, it );

	command.id = 0;
}

static void G_ScriptRun_Strafe_f()
{
	strafe = command.iargs[0];
	strafeOffset = command.args[1];
	command.id = 0;
}

static struct scriptfunction_s {
	const char *name;
	void ( *f )( void );
} scriptFunctions[] = {
	// keep these first two where they are
	{ "terminate", G_ScriptRun_Terminate_f },
	{ "nop", G_ScriptRun_Nop_f },
	{ "wait", G_ScriptRun_Wait_f },
	{ "waitframe", G_ScriptRun_WaitFrame_f },
	{ "waitattr", G_ScriptRun_WaitAttr_f },
	{ "print", G_ScriptRun_Print_f },
	{ "record", G_ScriptRun_Record_f },
	{ "stop", G_ScriptRun_Stop_f },
	{ "replay", G_ScriptRun_Replay_f },
	{ "angles", G_ScriptRun_Angles_f },
	{ "forward", G_ScriptRun_Forward_f },
	{ "up", G_ScriptRun_Up_f },
	{ "side", G_ScriptRun_Side_f },
	{ "button", G_ScriptRun_Button_f },
	{ "unbutton", G_ScriptRun_UnButton_f },
	{ "use", G_ScriptRun_Use_f },
	{ "strafe", G_ScriptRun_Strafe_f },
	{ NULL, NULL }
};

static int G_ScriptRun_FindCommand( const char *name )
{
	for( int i = 0; scriptFunctions[i].name; i++ )
	{
		if( !strcmp( scriptFunctions[i].name, name ) )
			return i;
	}
	return 0;
}

static int G_ScriptRun_LoadCommand( void )
{
	if( command.id && g_scriptRun->integer > 0 )
		return command.id;

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
		record = 0;
		replay = 0;
		memset( recordings, 0, sizeof( recordings ) );
	}

	if( replay )
	{
		memcpy( &command, &recordings[replay++ - 1], sizeof( command ) );
		return command.id;
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
	memset( &command, 0, sizeof( command ) );
	command.id = atoi( s );
	if( !command.id && strcmp( s, "0" ) && !( command.id = G_ScriptRun_FindCommand( s ) ) )
	{
		Com_Printf( "Unknown command: %s\n", s );
		command.id = 1;
	}
	for( int i = 0; i < 6; i++ )
	{
		char *q = COM_Parse( &p );
		if( q )
		{
			command.args[i] = atof( q );
			command.iargs[i] = atoi( q );
		}
		else
		{
			break;
		}
	}
	bi = next;

	return command.id;
}

static bool G_ScriptRun_Strafe( void )
{
	if( strafe && moves[2] && G_ScriptRun_HSpeed() >= ent->r.client->ps.pmove.stats[PM_STAT_MAXSPEED] )
		G_ScriptRun_SetAngle( YAW, G_ScriptRun_MoveAngle()
				+ ( ( moves[0] == 0 ? G_ScriptRun_BunnyAngle() : G_ScriptRun_StrafeAngle() ) + strafeOffset )
					* ( ( moves[2] < 0 ) ^ ( moves[0] < 0 ) ? 1 : -1 )
				+ ( moves[0] < 0 ? 180 : 0 ) );
	else
		return false;
	return true;
}

static void G_ScriptRun_Apply( void )
{
	G_ScriptRun_SetAngle( 0, angles[0] );
	if( !G_ScriptRun_Strafe() )
		G_ScriptRun_SetAngle( 1, angles[1] );
	ucmd.buttons = buttons;
	ucmd.forwardmove = moves[0];
	ucmd.upmove = moves[1];
	ucmd.sidemove = moves[2];
}

static void G_ScriptRun_Record( void )
{
	memcpy( &recordings[record++ - 1], &command, sizeof( command ) );
	if( command.id == G_ScriptRun_FindCommand( "stop" ) )
		record = 0;
	command.id = 0;
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
		command.id = G_ScriptRun_LoadCommand();
		running = command.id != 0;
		repeat = running;
		if( record )
			G_ScriptRun_Record();
		else
			scriptFunctions[command.id].f();
		repeat &= command.id == 0;
	}
	while( repeat );

	G_ScriptRun_Apply();

	ClientThink( ent, &ucmd, 0 );
}
