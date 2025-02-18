/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// view.h

extern	cvar_t	v_gamma;
extern	cvar_t	v_contrast;
extern	cvar_t	crosshair;

#ifdef GLQUAKE
extern	cvar_t	gl_hwblend;
extern	float	v_blend[4];
void V_AddLightBlend (float r, float g, float b, float a2);
void V_AddWaterfog (int contents);		
#endif

extern	byte	gammatable[256];	// palette is sent through this
extern	byte	current_pal[768];

#ifndef GLQUAKE
extern	cvar_t	lcd_x, lcd_yaw;
#endif

void V_Init (void);
void V_RenderView (void);

void V_CalcBlend (void);
char *LocalTime (char *format);
void SCR_DrawClock (void);
void SCR_DrawSpeed (void);
void SCR_Draw_TAS_Hud (void);
void SCR_DrawFPS (void);
void SCR_DrawStats (void);
void SCR_DrawVolume (void);
void SCR_DrawPlaybackStats (void);

float V_CalcRoll (vec3_t angles, vec3_t velocity);
void V_UpdatePalette (void);
