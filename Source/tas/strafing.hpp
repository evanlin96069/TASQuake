#pragma once

#include "cpp_quakedef.hpp"

extern cvar_t tas_strafe_lgagst;
extern cvar_t tas_strafe;
extern cvar_t tas_strafe_yaw;
extern cvar_t tas_strafe_yaw_offset;


void Strafe(usercmd_t* cmd);
void Strafe_Jump_Check();
void IN_TAS_Jump_Down(void);
void IN_TAS_Jump_Up(void);