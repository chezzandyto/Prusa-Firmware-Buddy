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
#include "../../module/motion.h"
#include "../../module/stepper.h"

#if ENABLED(I2C_POSITION_ENCODERS)
  #include "../../feature/I2CPositionEncoder.h"
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G90: Set to Absolute Positioning <a href="https://reprap.org/wiki/G-code#G90:_Set_to_Absolute_Positioning">G90: Set to Absolute Positioning</a>
 *
 *#### Usage
 *
 *    G90
 */

/**
 *### G91: Set to Relative Positioning <a href="https://reprap.org/wiki/G-code#G91:_Set_to_Relative_Positioning">G91: Set to Relative Positioning</a>
 *
 *#### Usage
 *
 *    G91
 */

/**
 *### G92: Get/Set current position <a href="https://reprap.org/wiki/G-code#G92:_Set_Position">G92: Set Position</a>
 *
 * Allows programming of absolute zero point, by reseting the current position to the values specified.
 *
 *#### Usage
 *
 *    G92 [ X | Y | Z | E ]
 *
 *#### Parameters
 *
 * - `X` - Set current position on X axis
 * - `Y` - Set current position on Y axis
 * - `Z` - Set current position on Z axis
 * - `E` - Set current position on E axis
 *
 *#### Examples
 *
 *    G92         ; without any parameters or without any values behind the parameters reports current xyze position
 *    G92 X10 E90 ; set the machine's X coordinate to 10, and the extrude coordinate to 90. No physical motion will occur.
 *
 */
void GcodeSuite::G92() {

  bool didE = false;
  #if IS_SCARA || !HAS_POSITION_SHIFT
    bool didXYZ = false;
  #else
    constexpr bool didXYZ = false;
  #endif

  #if USE_GCODE_SUBCODES
    const uint8_t subcode_G92 = parser.subcode;
  #else
    constexpr uint8_t subcode_G92 = 0;
  #endif

  switch (subcode_G92) {
    default: break;
    #if ENABLED(CNC_COORDINATE_SYSTEMS)
      case 1: {
        // Zero the G92 values and restore current position
        #if !IS_SCARA
          LOOP_XYZ(i) if (position_shift[i]) {
            position_shift[i] = 0;
            update_workspace_offset((AxisEnum)i);
          }
        #endif // Not SCARA
      } return;
    #endif
    #if ENABLED(POWER_LOSS_RECOVERY)
      case 9: {
        LOOP_XYZE(i) {
          if (parser.seenval(axis_codes[i])) {
            current_position[i] = parser.value_axis_units((AxisEnum)i);
            #if IS_SCARA || !HAS_POSITION_SHIFT
              if (i == E_AXIS) didE = true; else didXYZ = true;
            #elif HAS_POSITION_SHIFT
              if (i == E_AXIS) didE = true;
            #endif
          }
        }
      } break;
    #endif
    case 0: {
      LOOP_XYZE(i) {
        if (parser.seenval(axis_codes[i])) {
          const float l = parser.value_axis_units((AxisEnum)i),
                      v = i == E_AXIS ? l : LOGICAL_TO_NATIVE(l, i),
                      d = v - current_position[i];
          if (!NEAR_ZERO(d)) {
            #if IS_SCARA || !HAS_POSITION_SHIFT
              if (i == E_AXIS) didE = true; else didXYZ = true;
              current_position[i] = v;        // Without workspaces revert to Marlin 1.0 behavior
            #elif HAS_POSITION_SHIFT
              if (i == E_AXIS) {
                didE = true;
                current_position.e = v; // When using coordinate spaces, only E is set directly
              }
              else {
                position_shift[i] += d;       // Other axes simply offset the coordinate space
                update_workspace_offset((AxisEnum)i);
              }
            #endif
          }
        }
      }
    } break;
  }

  #if ENABLED(CNC_COORDINATE_SYSTEMS)
    // Apply workspace offset to the active coordinate system
    if (WITHIN(active_coordinate_system, 0, MAX_COORDINATE_SYSTEMS - 1))
      coordinate_system[active_coordinate_system] = position_shift;
  #endif

  if    (didXYZ) sync_plan_position();
  else if (didE) sync_plan_position_e();

  report_current_position();
}

/** @}*/
