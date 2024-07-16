#include "filament_gui.hpp"

static constexpr IWindowMenuItem::ColorScheme hidden_filament_color_scheme {
    .text = {
        .focused = COLOR_GRAY,
        .unfocused = COLOR_GRAY,
    },
};

void FilamentTypeGUI::setup_menu_item([[maybe_unused]] FilamentType ft, const FilamentTypeParameters &params, IWindowMenuItem &item) {
    item.SetLabel(string_view_utf8::MakeRAM(params.name));
    item.set_color_scheme(ft.is_visible() ? nullptr : &hidden_filament_color_scheme);
}
