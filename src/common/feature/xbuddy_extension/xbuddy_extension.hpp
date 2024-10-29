#pragma once

#include "cooling.hpp"

#include <optional>
#include <span>

#include <freertos/mutex.hpp>
#include <leds/color.hpp>

namespace buddy {

/// Thread-safe API, can be read/written to from any thread
class XBuddyExtension {
public: // General things, status
    XBuddyExtension();

    enum class Status {
        disabled,
        not_connected,
        ready,
    };

    Status status() const;

    void step();

public: // Fans
    /// \returns measured RPM of the fan1 (chamber cooling)
    std::optional<uint16_t> fan1_rpm() const;

    /// \returns measured RPM of the fan2 (chamber cooling)
    std::optional<uint16_t> fan2_rpm() const;

    /// \returns shared target PWM for fan1/fan2
    uint8_t fan1_fan2_pwm() const;

    /// Sets PWM for fan 1 and fan 2 (chamber cooling fans; 0-255)
    /// The fans have a single shared PWM line, so they can only be set both at once
    /// Disables \p has_fan1_fan2_auto_control control
    void set_fan1_fan2_pwm(uint8_t pwm);

    /// \returns whether the fan1 & fan2 are in auto control mode (or values set explicitly by set_XX_pwm)
    bool has_fan1_fan2_auto_control() const;

    /// See \p has_fan1_fan2_auto_control
    void set_fan1_fan2_auto_control();

    /// \returns measured RPM of the fan3 (in-chamber filtration)
    std::optional<uint16_t> fan3_rpm() const;

    /// Sets PRM for fan 3 (in-chamber filtration, 0-255)
    void set_fan3_pwm(uint8_t pwm);

public: // LEDs
    /// \returns color set for the bed LED strip
    leds::ColorRGBW bed_leds_color() const;

    /// Sets PWM for the led strip that is under the bed
    void set_bed_leds_color(leds::ColorRGBW set);

    /// \returns PWM for the white led strip thas is in the chamber (0-255)
    uint8_t chamber_leds_pwm();

    /// Sets PWM for the white led strip thas is in the chamber (0-255)
    void set_chamber_leds_pwm(uint8_t set);

public: // Other
    /// \returns chamber temperature measured through the thermistor connected to the board, in degrees Celsius
    std::optional<float> chamber_temperature();

protected:
    /// Updates all relevant target registers to match the current config of the printer
    /// To be called right after the connection is established with the board
    /// TODO: call this function from the right place
    void update_registers_nolock();

private:
    mutable freertos::Mutex mutex_;

    leds::ColorRGBW bed_leds_color_;

    FanCooling chamber_cooling;

    uint8_t fan3_pwm = 0;
};

XBuddyExtension &xbuddy_extension();

} // namespace buddy
