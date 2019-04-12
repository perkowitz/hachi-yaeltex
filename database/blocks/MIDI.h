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

typedef enum
{
    dbSection_midi_feature,
    dbSection_midi_merge,
    DB_SECTIONS_MIDI
} dbSection_midi_t;


typedef enum
{
    midiFeatureStandardNoteOff,
    midiFeatureRunningStatus,
    midiFeatureMergeEnabled,
    midiFeatureDinEnabled,
    MIDI_FEATURES
} midiFeature_t;

typedef enum
{
    midiMergeToInterface,
    midiMergeUSBchannel,
    midiMergeDINchannel,
    MIDI_MERGE_OPTIONS
} midiMerge_t;

typedef enum
{
    midiMergeToInterfaceUSB,
    midiMergeToInterfaceDIN,
    MIDI_MERGE_TO_INTERFACES
} midiMergeToInterface_t;
