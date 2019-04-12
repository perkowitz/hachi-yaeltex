/*
    Yaeltex Kilomux V2 firmware
    2019 - Buenos Aires - Argentina - 
    Author: Franco Grassano

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "../../database/blocks/Buttons.h"

typedef enum
{
    sysExSection_buttons_type,
    sysExSection_buttons_midiMessage,
    sysExSection_buttons_midiID,
    sysExSection_buttons_velocity,
    sysExSection_buttons_midiChannel,
    SYSEX_SECTIONS_BUTTONS
} sysExSection_buttons_t;

//map sysex sections to sections in db
const uint8_t sysEx2DB_buttons[SYSEX_SECTIONS_BUTTONS] =
{
    dbSection_buttons_type,
    dbSection_buttons_midiMessage,
    dbSection_buttons_midiID,
    dbSection_buttons_velocity,
    dbSection_buttons_midiChannel
};