// screen_menu_filament.c

#include "gui.h"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "filament.h"
#include "filament_sensor.h"
#include "marlin_client.h"
#include "menu_vars.h"
#include "window_dlg_load_unload.h"
#include "screens.h"
#include "dbg.h"
#include "DialogHandler.hpp"

/*****************************************************************************/
//parent
class MI_event_dispatcher : public WI_LABEL_t {
protected:
    virtual void click(Iwindow_menu_t &window_menu) {
        //no way to change header on this level, have to dispatch event
        screen_dispatch_event(NULL, WINDOW_EVENT_CLICK, (void *)this);
    }

public:
    MI_event_dispatcher(const char *label)
        : WI_LABEL_t(label, 0, true, false) {}

    virtual const char *GetHeaderAlterLable() = 0;
    virtual void Do() = 0;
};

/*****************************************************************************/
//MI_LOAD
class MI_LOAD : public MI_event_dispatcher {
    constexpr static const char *const label = "Load Filament";
    constexpr static const char *const header_label = "LOAD FILAMENT";

public:
    MI_LOAD()
        : MI_event_dispatcher(label) {}
    virtual const char *GetHeaderAlterLable() {
        return header_label;
    }
    virtual void Do() {
        gui_dlg_load();
    }
};

/*****************************************************************************/
//MI_UNLOAD
class MI_UNLOAD : public MI_event_dispatcher {
    constexpr static const char *const label = "Unload Filament";
    constexpr static const char *const header_label = "UNLOAD FILAMENT";

public:
    MI_UNLOAD()
        : MI_event_dispatcher(label) {}
    virtual const char *GetHeaderAlterLable() {
        return header_label;
    }
    virtual void Do() {
        gui_dlg_unload();
    }
};

/*****************************************************************************/
//MI_CHANGE
class MI_CHANGE : public MI_event_dispatcher {
    constexpr static const char *const label = "Change Filament";
    constexpr static const char *const header_label = "CHANGE FILAMENT";

public:
    MI_CHANGE()
        : MI_event_dispatcher(label) {}
    virtual const char *GetHeaderAlterLable() {
        return header_label;
    }
    virtual void Do() {
        gui_dlg_unload();

        //opens unload dialog if it is not already openned
        DialogHandler::WaitUntilClosed(ClientFSM::Load_unload, uint8_t(LoadUnloadMode::Unload));

        gui_dlg_load();
    }
};

/*****************************************************************************/
//MI_LOAD
class MI_PURGE : public MI_event_dispatcher {
    constexpr static const char *const label = "Purge Filament";
    constexpr static const char *const header_label = "PURGE FILAMENT";

public:
    MI_PURGE()
        : MI_event_dispatcher(label) {}
    virtual const char *GetHeaderAlterLable() {
        return header_label;
    }
    virtual void Do() {
        gui_dlg_purge();
    }
};

using parent = screen_menu_data_t<EHeader::Off, EFooter::On, EHelp::Off, MI_RETURN, MI_LOAD, MI_UNLOAD, MI_CHANGE, MI_PURGE>;

#pragma pack(push, 1)
class ScreenMenuFilament : public parent {
    enum {
        F_EEPROM = 0x01, // filament is known
        F_SENSED = 0x02  // filament is not in sensor
    };

public:
    constexpr static const char *label = "FILAMENT";
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);

private:
    void deactivate_item();

    //todo use std::get<MI_LOAD> on tuple - have to remove pContainer and use template
    typedef enum { LOAD = 1,
        CHANGE = 3,
        PURGE = 4 } Type; // todo remove this enum and use MI_...
    template <Type T>
    void dis() {
        this->menu.GetItem(uint8_t(T))->Disable();
        menu.win.f_invalid = 1;
    }
    template <Type T>
    void ena() {
        this->menu.GetItem(uint8_t(T))->Enable();
        menu.win.f_invalid = 1;
    }
};
#pragma pack(pop)

/*****************************************************************************/
//static method definition
void ScreenMenuFilament::Init(screen_t *screen) {
    Create(screen, label);
    reinterpret_cast<ScreenMenuFilament *>(screen->pdata)->deactivate_item();
}

int ScreenMenuFilament::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuFilament *const ths = reinterpret_cast<ScreenMenuFilament *>(screen->pdata);
    ths->deactivate_item();
    if (event == WINDOW_EVENT_CLICK) {
        MI_event_dispatcher *const item = reinterpret_cast<MI_event_dispatcher *>(param);
        if (item->IsEnabled()) {
            p_window_header_set_text(&ths->header, item->GetHeaderAlterLable()); //set new label
            item->Do();                                                          //do action (load filament ...)
            p_window_header_set_text(&ths->header, label);                       //restore label
        }
    } else {
        return ths->Event(window, event, param);
    }
    return 0;
}

/*****************************************************************************/
//non-static method definition

/*
 * +---------+--------+------+--------+--------+-------+--------------------------------------------------------+
 * | FSENSOR | EEPROM | load | unload | change | purge | comment                                                |
 * +---------+--------+------+--------+--------+-------+--------------------------------------------------------+
 * |       0 |      0 |  YES |    YES |     NO |    NO | filament not loaded                                    |
 * |       0 |      1 |   NO |    YES |    YES |    NO | filament loaded but just runout                        |
 * |       1 |      0 |  YES |    YES |     NO |    NO | user pushed filament into sensor, but it is not loaded |
 * |       1 |      1 |   NO |    YES |    YES |   YES | filament loaded                                        |
 * +---------+--------+------+--------+--------+-------+--------------------------------------------------------+
 */
void ScreenMenuFilament::deactivate_item() {

    uint8_t filament = 0;
    filament |= get_filament() != FILAMENT_NONE ? F_EEPROM : 0;
    filament |= fs_get_state() == FS_NO_FILAMENT ? 0 : F_SENSED;
    switch (filament) {
    case 0: //filament not loaded
        ena<LOAD>();
        dis<CHANGE>();
        dis<PURGE>();
        break;
    case F_EEPROM: //filament loaded but just runout
        dis<LOAD>();
        ena<CHANGE>();
        dis<PURGE>();
        break;
    case F_SENSED: //user pushed filament into sensor, but it is not loaded
        ena<LOAD>();
        dis<CHANGE>();
        dis<PURGE>();
        break;
    case F_SENSED | F_EEPROM: //filament loaded
    default:
        dis<LOAD>();
        ena<CHANGE>();
        ena<PURGE>();
        break;
    }
    gui_invalidate();
}

screen_t screen_menu_filament = {
    0,
    0,
    ScreenMenuFilament::Init,
    ScreenMenuFilament::CDone,
    ScreenMenuFilament::CDraw,
    ScreenMenuFilament::CEvent,
    sizeof(ScreenMenuFilament), //data_size
    0,                          //pdata
};

extern "C" screen_t *const get_scr_menu_filament() { return &screen_menu_filament; }
