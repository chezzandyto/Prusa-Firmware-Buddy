/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../../inc/MarlinConfig.h"

#if HAS_HOTEND_OFFSET

#include "../gcode.h"
#include "../../module/motion.h"

#if ENABLED(DELTA)
  #include "../../module/planner.h"
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M218: Get/Set hotend offset <a href="https://reprap.org/wiki/G-code#M218:_Set_Hotend_Offset">M218: Set Hotend Offset</a>
 *
 *#### Usage
 *
 *    M218 [ X | Y | Z ]
 *
 *#### Parameters
 *
 *
 * - `T` - Tool
 * - `X` - X hotend offset
 * - `Y` - Y hotend offset
 * - `Z` - Z hotend offset
 *
 * Without parameters prints the current hotend offset(s)
 */
void GcodeSuite::M218() {

  const int8_t target_extruder = get_target_extruder_from_command();
  if (target_extruder < 0) return;

  if (parser.seenval('X')) hotend_offset[target_extruder].x = parser.value_linear_units();
  if (parser.seenval('Y')) hotend_offset[target_extruder].y = parser.value_linear_units();
  if (parser.seenval('Z')) hotend_offset[target_extruder].z = parser.value_linear_units();

  if (!parser.seen("XYZ")) {
    SERIAL_ECHO_START();
    SERIAL_ECHOPGM(MSG_HOTEND_OFFSET);
    HOTEND_LOOP() {
      SERIAL_CHAR(' ');
      SERIAL_ECHO(hotend_offset[e].x);
      SERIAL_CHAR(',');
      SERIAL_ECHO(hotend_offset[e].y);
      SERIAL_CHAR(',');
      SERIAL_ECHO_F(hotend_offset[e].z, 3);
    }
    SERIAL_EOL();
  }

  #if ENABLED(DELTA)
    if (target_extruder == active_extruder)
      do_blocking_move_to_xy(current_position, planner.settings.max_feedrate_mm_s[X_AXIS]);
  #endif
}

/** @}*/

#endif // HAS_HOTEND_OFFSET
