#include "filament.hpp"
#include "filament_list.hpp"

#include <cassert>
#include <cstring>
#include <utility>

#include "i18n.h"
#include "client_response_texts.hpp"
#include "../../include/printers.h"
#include <Marlin/src/inc/MarlinConfigPre.h>
#include <enum_array.hpp>
#include <option/has_loadcell.h>

// !!! If this value changes, you need to inspect usages and possibly write up some config store migrations
static_assert(max_preset_filament_type_count == 32);

// We're storing the bed temperature in uint8_t, so make sure the bed cannot go higher
static_assert(BED_MAXTEMP <= 255);

static constexpr FilamentTypeParameters none_filament_parameters {
    .name = "---",
    .nozzle_temperature = 0,
    .nozzle_preheat_temperature = 0,
    .heatbed_temperature = 0,
};

// These temperatures correspond to slicer defaults for MBL.
constexpr EnumArray<PresetFilamentType, FilamentTypeParameters, PresetFilamentType::_count> preset_filament_parameters {
    {
        PresetFilamentType::PLA,
        {
            .name = "PLA",
            .nozzle_temperature = 215,
            .heatbed_temperature = 60,
        },
    },
    {
        PresetFilamentType::PETG,
        {
            .name = "PETG",
            .nozzle_temperature = 230,
            .heatbed_temperature = 85,
        },
    },
    {
        PresetFilamentType::ASA,
        {
            .name = "ASA",
            .nozzle_temperature = 260,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PC,
        {
            .name = "PC",
            .nozzle_temperature = 275,
            .nozzle_preheat_temperature = HAS_LOADCELL() ? 170 : 275 - 25,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PVB,
        {
            .name = "PVB",
            .nozzle_temperature = 215,
            .heatbed_temperature = 75,
        },
    },
    {
        PresetFilamentType::ABS,
        {
            .name = "ABS",
            .nozzle_temperature = 255,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::HIPS,
        {
            .name = "HIPS",
            .nozzle_temperature = 220,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PP,
        {
            .name = "PP",
            .nozzle_temperature = 240,
            .heatbed_temperature = 100,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::FLEX,
        {
            .name = "FLEX",
            .nozzle_temperature = 240,
            .nozzle_preheat_temperature = HAS_LOADCELL() ? 170 : 210,
            .heatbed_temperature = 50,
            .requires_filtration = true,
        },
    },
    {
        PresetFilamentType::PA,
        {
            .name = "PA",
            // MINI has slightly lower max nozzle temperature but it is still OK for polyamid
            .nozzle_temperature = PRINTER_IS_PRUSA_MINI ? 280 : 285,
            .heatbed_temperature = 100,
        },
    },
};
constexpr bool temperatures_are_within_spec(const FilamentTypeParameters &filament) {
    return (filament.nozzle_temperature <= HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.nozzle_preheat_temperature <= HEATER_0_MAXTEMP - HEATER_MAXTEMP_SAFETY_MARGIN)
        && (filament.heatbed_temperature <= BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN);
}
static_assert(std::ranges::all_of(preset_filament_parameters, temperatures_are_within_spec));

FilamentType FilamentType::from_name(std::string_view name) {
    if (name.length() >= filament_name_buffer_size) {
        return FilamentType::none;
    }

    for (const FilamentType filament_type : all_filament_types) {
        if (name == filament_type.parameters().name) {
            return filament_type;
        }
    }

    return FilamentType::none;
}

FilamentTypeParameters FilamentType::parameters() const {
    return std::visit([]<typename T>(const T &v) -> FilamentTypeParameters {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return preset_filament_parameters[v];

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            return none_filament_parameters;
        }
    },
        *this);
}

bool FilamentType::is_visible() const {
    return std::visit([]<typename T>(const T &v) -> bool {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            return config_store().visible_preset_filament_types.get().test(static_cast<size_t>(v));

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            return false;
        }
    },
        *this);
}

void FilamentType::set_visible(bool set) const {
    return std::visit([set]<typename T>(const T &v) -> void {
        if constexpr (std::is_same_v<T, PresetFilamentType>) {
            config_store().visible_preset_filament_types.apply([v, set](auto &value) {
                value.set(static_cast<size_t>(v), set);
            });

        } else if constexpr (std::is_same_v<T, NoFilamentType>) {
            // Do nothing

        } else {
            static_assert(0);
        }
    },
        *this);
}
