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

#include "../gcode.h"
#include "../../Marlin.h" // for stepper_inactive_time, disable_e_steppers
#include "../../lcd/ultralcd.h"
#include "../../module/stepper.h"
#include "gcode/parser.h"

#if BOTH(AUTO_BED_LEVELING_UBL, ULTRA_LCD)
  #include "../../feature/bedlevel/bedlevel.h"
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 * M17: Enable stepper motors
 *
 * ## Parameters
 *
 * - `X` - Enable X axis stepper motor
 * - `Y` - Enable Y axis stepper motor
 * - `Z` - Enable Z axis stepper motor
 * - `E` - Enable E axis stepper motor
 */
void GcodeSuite::M17() {
  if (parser.seen("XYZE")) {
    #if ENABLED(XY_LINKED_ENABLE)
      if (parser.seen('X') || parser.seen('Y')) enable_XY();
    #else
      if (parser.seen('X')) enable_X();
      if (parser.seen('Y')) enable_Y();
    #endif
    if (parser.seen('Z')) enable_Z();
    #if HAS_E_STEPPER_ENABLE
      if (parser.seen('E')) enable_e_steppers();
    #endif
  }
  else {
    LCD_MESSAGEPGM(MSG_NO_MOVE);
    enable_all_steppers();
  }
}

/**
 * M18, M84: Disable stepper motors
 *
 * ## Parameters
 *
 * - `S` - [seconds] stepper inactive time
 * - `X` - Disable X axis stepper motor
 * - `Y` - Disable Y axis stepper motor
 * - `Z` - Disable Z axis stepper motor
 * - `E` - Disable E axis stepper motor
 */
void GcodeSuite::M18_M84() {
  if (parser.seenval('S')) {
    stepper_inactive_time = parser.value_millis_from_seconds();
  }
  else {
    if (parser.seen("XYZE")) {
      planner.synchronize();
      #if ENABLED(XY_LINKED_ENABLE)
        if (parser.seen('X') || parser.seen('Y')) {
          disable_XY();
        }
      #else
        if (parser.seen('X')) disable_X();
        if (parser.seen('Y')) disable_Y();
      #endif
      if (parser.seen('Z')) disable_Z();
      #if HAS_E_STEPPER_ENABLE
        if (parser.seen('E')) disable_e_steppers();
      #endif
    }
    else
      planner.finish_and_disable();

    #if HAS_LCD_MENU && ENABLED(AUTO_BED_LEVELING_UBL)
      if (ubl.lcd_map_control) {
        ubl.lcd_map_control = false;
        ui.defer_status_screen(false);
      }
    #endif
  }
}

/** @}*/
