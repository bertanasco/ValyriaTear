///////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2004-2011 by The Allacrost Project
//            Copyright (C) 2012-2014 by Bertram (Valyria Tear)
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See http://www.gnu.org/copyleft/gpl.html for details.
///////////////////////////////////////////////////////////////////////////////

/** ****************************************************************************
*** \file    options_handler.cpp
*** \author  Viljami Korhonen, mindflayer@allacrost.org
*** \author  Yohann Ferreira, yohann ferreira orange fr
*** \brief   Source file for the options menus
*** ***************************************************************************/

#include "utils/utils_pch.h"
#include "options_handler.h"

#include "engine/system.h"
#include "engine/input.h"
#include "engine/audio/audio.h"
#include "engine/script/script_write.h"

#include "engine/mode_manager.h"
#include "modes/mode_help_window.h"

#include "utils/utils_files.h"

#include "common/global/global.h"

using namespace vt_utils;
using namespace vt_video;
using namespace vt_input;
using namespace vt_global;
using namespace vt_system;
using namespace vt_script;
using namespace vt_audio;

namespace vt_gui
{

namespace private_gui
{

void OptionMenu::AddOption(const ustring &text, GameOptionsMenuHandler* handler,
                         void (GameOptionsMenuHandler::*confirm_function)(),
                         void (GameOptionsMenuHandler::*up_function)(), void (GameOptionsMenuHandler::*down_function)(),
                         void (GameOptionsMenuHandler::*left_function)(), void (GameOptionsMenuHandler::*right_function)())
{
    OptionBox::AddOption(text);

    _handler = handler;

    if (_handler == NULL) {
        PRINT_WARNING << "Invalid option menu handler given: No option menu functions will be working..." << std::endl;
        _confirm_handlers.push_back(NULL);
        _up_handlers.push_back(NULL);
        _down_handlers.push_back(NULL);
        _left_handlers.push_back(NULL);
        _right_handlers.push_back(NULL);
        return;
    }

    _confirm_handlers.push_back(confirm_function);
    _up_handlers.push_back(up_function);
    _down_handlers.push_back(down_function);
    _left_handlers.push_back(left_function);
    _right_handlers.push_back(right_function);
}

void OptionMenu::InputConfirm()
{
    OptionBox::InputConfirm();

    int32 selection = OptionBox::GetSelection();
    if((selection != -1) && (_confirm_handlers.empty() == false)) {
        void (GameOptionsMenuHandler::*confirm_function)() = _confirm_handlers.at(selection);
        if(confirm_function != NULL)
            (_handler->*confirm_function)();
    }
}

void OptionMenu::InputUp()
{
    OptionBox::InputUp();

    int32 selection = OptionBox::GetSelection();
    if((selection != -1) && (_up_handlers.empty() == false)) {
        void (GameOptionsMenuHandler::*up_function)() = _up_handlers.at(selection);
        if(up_function != NULL)
            (_handler->*up_function)();
    }
}

void OptionMenu::InputDown()
{
    OptionBox::InputDown();

    int32 selection = OptionBox::GetSelection();
    if((selection != -1) && (_down_handlers.empty() == false)) {
        void (GameOptionsMenuHandler::*down_function)() = _down_handlers.at(selection);
        if(down_function != NULL)
            (_handler->*down_function)();
    }
}

void OptionMenu::InputLeft()
{
    OptionBox::InputLeft();

    int32 selection = OptionBox::GetSelection();
    if((selection != -1) && (_left_handlers.empty() == false)) {
        void (GameOptionsMenuHandler::*left_function)() = _left_handlers.at(selection);
        if(left_function != NULL)
            (_handler->*left_function)();
    }
}

void OptionMenu::InputRight()
{
    OptionBox::InputRight();

    int32 selection = OptionBox::GetSelection();
    if((selection != -1) && (_right_handlers.empty() == false)) {
        void (GameOptionsMenuHandler::*right_function)() = _right_handlers.at(selection);
        if(right_function != NULL)
            (_handler->*right_function)();
    }
}

const std::string _LANGUAGE_FILE = "dat/config/languages.lua";

GameOptionsMenuHandler::GameOptionsMenuHandler(vt_mode_manager::GameMode* parent_mode):
    _first_run(false),
    _has_modified_settings(false),
    _active_menu(NULL),
    _key_setting_function(NULL),
    _joy_setting_function(NULL),
    _joy_axis_setting_function(NULL),
    _message_window(ustring(), 310.0f, 233.0f),
    _parent_mode(parent_mode)
{
    // Create the option window used as background
    _options_window.Create(300.0f, 550.0f);
    _options_window.SetPosition(360.0f, 188.0f);
    _options_window.Hide();

    // Setup all menu options and properties
    _SetupOptionsMenu();
    _SetupVideoOptionsMenu();
    _SetupAudioOptionsMenu();
    _SetupLanguageOptionsMenu();
    _SetupKeySettingsMenu();
    _SetupJoySettingsMenu();
    _SetupResolutionMenu();

    // make sure message window is not visible
    _message_window.Hide();
}

GameOptionsMenuHandler::~GameOptionsMenuHandler()
{
    _options_window.Destroy();
    _SaveSettingsFile();

    _key_setting_function = NULL;
    _joy_setting_function = NULL;
    _joy_axis_setting_function = NULL;
}

void GameOptionsMenuHandler::Activate()
{
    _active_menu = &_options_menu;
    _options_window.Show();
}

void GameOptionsMenuHandler::ShowFirstRunLanguageSelection()
{
    _first_run = true;
    _options_window.Show();
    _active_menu = &_language_options_menu;
}

void GameOptionsMenuHandler::Update()
{
    _options_window.Update(vt_system::SystemManager->GetUpdateTime());

    // On first app run, show the language menu and apply language on any key press.
    if (_first_run && _active_menu == &_language_options_menu) {
        _active_menu->Update();
        if (InputManager->UpPress()) {
            _active_menu->InputUp();
        }
        else if (InputManager->DownPress()) {
            _active_menu->InputDown();
        }
        else if (InputManager->LeftPress() || InputManager->RightPress()) {
            // Do nothing in this case
        }
        else if (InputManager->AnyKeyboardKeyPress()
                || InputManager->AnyJoystickKeyPress()
                || InputManager->ConfirmPress()) {
            // Set the language
            _active_menu->InputConfirm();
            // Go directly back to the main menu when first selecting the language.
            _options_window.Hide();
            _active_menu = NULL;
            // And show the help window
            vt_mode_manager::ModeManager->GetHelpWindow()->Show();
            // save the settings (automatically changes the first_start variable to 0)
            _has_modified_settings = true;
            _SaveSettingsFile();
            _first_run = false;
        }
        return;
    }

    // Updates the current menu or do nothing
    if (_active_menu)
        _active_menu->Update();
    else
        return;

    // Check for waiting keypresses or joystick button presses
    if(_joy_setting_function != NULL) {
        if(InputManager->AnyJoystickKeyPress()) {
            (this->*_joy_setting_function)(InputManager->GetMostRecentEvent().jbutton.button);
            _joy_setting_function = NULL;
            _has_modified_settings = true;
            _RefreshJoySettings();
            _message_window.Hide();
        }
        if(InputManager->CancelPress()) {
            _joy_setting_function = NULL;
            _message_window.Hide();
        }
        return;
    }

    if(_joy_axis_setting_function != NULL) {
        int8 x = InputManager->GetLastAxisMoved();
        if(x != -1) {
            (this->*_joy_axis_setting_function)(x);
            _joy_axis_setting_function = NULL;
            _has_modified_settings = true;
            _RefreshJoySettings();
            _message_window.Hide();
        }
        if(InputManager->CancelPress()) {
            _joy_axis_setting_function = NULL;
            _message_window.Hide();
        }
        return;
    }

    if(_key_setting_function != NULL) {
        if(InputManager->AnyKeyboardKeyPress()) {
            (this->*_key_setting_function)(InputManager->GetMostRecentEvent().key.keysym.sym);
            _key_setting_function = NULL;
            _has_modified_settings = true;
            _RefreshKeySettings();
            _message_window.Hide();
        }
        if(InputManager->CancelPress()) {
            _key_setting_function = NULL;
            _message_window.Hide();
        }
        return;
    }

    if(InputManager->ConfirmPress()) {
        // Play 'confirm sound' if the selection isn't grayed out and it has a confirm handler
        if(_active_menu->IsOptionEnabled(_active_menu->GetSelection())) {
            // Don't play the sound on New Games as they have their own sound
            if(_active_menu->GetSelection() != -1)
                GlobalManager->Media().PlaySound("confirm");
        } else {
            // Otherwise play a different sound
            GlobalManager->Media().PlaySound("bump");
        }

        _active_menu->InputConfirm();

    } else if(InputManager->LeftPress()) {
        GlobalManager->Media().PlaySound("bump");
        _active_menu->InputLeft();
    } else if(InputManager->RightPress()) {
        GlobalManager->Media().PlaySound("bump");
        _active_menu->InputRight();
    } else if(InputManager->UpPress()) {
        GlobalManager->Media().PlaySound("bump");
        _active_menu->InputUp();
    } else if(InputManager->DownPress()) {
        GlobalManager->Media().PlaySound("bump");
        _active_menu->InputDown();
    } else if(InputManager->CancelPress() || InputManager->QuitPress()) {
        if(_active_menu == &_options_menu) {
            _options_window.Hide();
            _active_menu = NULL;
        } else if(_active_menu == &_video_options_menu) {
            _active_menu = &_options_menu;
        } else if(_active_menu == &_audio_options_menu) {
            _active_menu = &_options_menu;
        } else if(_active_menu == &_language_options_menu) {
            _active_menu = &_options_menu;
        } else if(_active_menu == &_key_settings_menu) {
            _active_menu = &_options_menu;
        } else if(_active_menu == &_joy_settings_menu) {
            _active_menu = &_options_menu;
        } else if(_active_menu == &_resolution_menu) {
            _active_menu = &_video_options_menu;
        }

        // Play cancel sound
        GlobalManager->Media().PlaySound("cancel");
    }
}

void GameOptionsMenuHandler::Draw()
{
    VideoManager->PushState();
    VideoManager->SetDrawFlags(VIDEO_X_LEFT, VIDEO_Y_TOP, VIDEO_BLEND, 0);
    VideoManager->SetStandardCoordSys();

    _options_window.Draw();

    if(_active_menu)
        _active_menu->Draw();

    VideoManager->SetDrawFlags(VIDEO_X_RIGHT, VIDEO_Y_BOTTOM, 0);
    VideoManager->Move(0.0f, 0.0f);
    _message_window.Draw();

    VideoManager->PopState();
}

void GameOptionsMenuHandler::ReloadTranslatableMenus()
{
    _SetupOptionsMenu();
    _SetupVideoOptionsMenu();
    _SetupAudioOptionsMenu();
    _SetupKeySettingsMenu();
    _SetupJoySettingsMenu();
    _SetupResolutionMenu();

    // Make the parent game mode reload its translated text
    if (_parent_mode)
        _parent_mode->ReloadTranslatedTexts();
}

void GameOptionsMenuHandler::_SetupOptionsMenu()
{
    _options_menu.ClearOptions();
    _options_menu.SetPosition(512.0f, 468.0f);
    _options_menu.SetDimensions(300.0f, 600.0f, 1, 5, 1, 5);
    _options_menu.SetTextStyle(TextStyle("title22"));
    _options_menu.SetAlignment(VIDEO_X_CENTER, VIDEO_Y_CENTER);
    _options_menu.SetOptionAlignment(VIDEO_X_CENTER, VIDEO_Y_CENTER);
    _options_menu.SetSelectMode(VIDEO_SELECT_SINGLE);
    _options_menu.SetVerticalWrapMode(VIDEO_WRAP_MODE_STRAIGHT);
    _options_menu.SetCursorOffset(-50.0f, -28.0f);
    _options_menu.SetSkipDisabled(true);

    _options_menu.AddOption(UTranslate("Video"), this, &GameOptionsMenuHandler::_OnVideoOptions);
    _options_menu.AddOption(UTranslate("Audio"), this, &GameOptionsMenuHandler::_OnAudioOptions);
    _options_menu.AddOption(UTranslate("Language"), this, &GameOptionsMenuHandler::_OnLanguageOptions);
    _options_menu.AddOption(UTranslate("Key Settings"), this, &GameOptionsMenuHandler::_OnKeySettings);
    _options_menu.AddOption(UTranslate("Joystick Settings"), this, &GameOptionsMenuHandler::_OnJoySettings);

    _options_menu.SetSelection(0);

    // Disable the language menu when not in the boot menu.
    // Otherwise, the game language changes aren't handled correctly.
    if (_parent_mode && _parent_mode->GetGameType() != vt_mode_manager::MODE_MANAGER_BOOT_MODE)
        _options_menu.EnableOption(2, false);
}

void GameOptionsMenuHandler::_SetupVideoOptionsMenu()
{
    _video_options_menu.ClearOptions();
    _video_options_menu.SetPosition(512.0f, 468.0f);
    _video_options_menu.SetDimensions(300.0f, 400.0f, 1, 4, 1, 4);
    _video_options_menu.SetTextStyle(TextStyle("title22"));
    _video_options_menu.SetAlignment(VIDEO_X_CENTER, VIDEO_Y_CENTER);
    _video_options_menu.SetOptionAlignment(VIDEO_X_CENTER, VIDEO_Y_CENTER);
    _video_options_menu.SetSelectMode(VIDEO_SELECT_SINGLE);
    _video_options_menu.SetVerticalWrapMode(VIDEO_WRAP_MODE_STRAIGHT);
    _video_options_menu.SetCursorOffset(-50.0f, -28.0f);
    _video_options_menu.SetSkipDisabled(true);

    _video_options_menu.AddOption(UTranslate("Resolution: "), this, &GameOptionsMenuHandler::_OnResolution);
    // Left & right will change window mode as well as confirm
    _video_options_menu.AddOption(UTranslate("Window mode: "), this, &GameOptionsMenuHandler::_OnToggleFullscreen, NULL, NULL,
                                  &GameOptionsMenuHandler::_OnToggleFullscreen, &GameOptionsMenuHandler::_OnToggleFullscreen);
    _video_options_menu.AddOption(UTranslate("Brightness: "), this, NULL, NULL, NULL, &GameOptionsMenuHandler::_OnBrightnessLeft,
                                  &GameOptionsMenuHandler::_OnBrightnessRight);
    _video_options_menu.AddOption(UTranslate("UI Theme: "), this, &GameOptionsMenuHandler::_OnUIThemeRight, NULL, NULL,
                                  &GameOptionsMenuHandler::_OnUIThemeLeft, &GameOptionsMenuHandler::_OnUIThemeRight);

    _video_options_menu.SetSelection(0);
}

void GameOptionsMenuHandler::_SetupAudioOptionsMenu()
{
    _audio_options_menu.ClearOptions();
    _audio_options_menu.SetPosition(512.0f, 468.0f);
    _audio_options_menu.SetDimensions(300.0f, 200.0f, 1, 2, 1, 2);
    _audio_options_menu.SetTextStyle(TextStyle("title22"));
    _audio_options_menu.SetAlignment(VIDEO_X_CENTER, VIDEO_Y_CENTER);
    _audio_options_menu.SetOptionAlignment(VIDEO_X_CENTER, VIDEO_Y_CENTER);
    _audio_options_menu.SetSelectMode(VIDEO_SELECT_SINGLE);
    _audio_options_menu.SetVerticalWrapMode(VIDEO_WRAP_MODE_STRAIGHT);
    _audio_options_menu.SetCursorOffset(-50.0f, -28.0f);
    _audio_options_menu.SetSkipDisabled(true);

    _audio_options_menu.AddOption(UTranslate("Sound Volume: "), this, NULL, NULL, NULL,
                                  &GameOptionsMenuHandler::_OnSoundLeft,
                                  &GameOptionsMenuHandler::_OnSoundRight);
    _audio_options_menu.AddOption(UTranslate("Music Volume: "), this, NULL, NULL, NULL,
                                  &GameOptionsMenuHandler::_OnMusicLeft,
                                  &GameOptionsMenuHandler::_OnMusicRight);

    _audio_options_menu.SetSelection(0);
}


void GameOptionsMenuHandler::_SetupLanguageOptionsMenu()
{
    _language_options_menu.SetPosition(402.0f, 468.0f);
    _language_options_menu.SetTextStyle(TextStyle("title22"));
    _language_options_menu.SetAlignment(VIDEO_X_LEFT, VIDEO_Y_CENTER);
    _language_options_menu.SetOptionAlignment(VIDEO_X_LEFT, VIDEO_Y_CENTER);
    _language_options_menu.SetSelectMode(VIDEO_SELECT_SINGLE);
    _language_options_menu.SetVerticalWrapMode(VIDEO_WRAP_MODE_STRAIGHT);
    _language_options_menu.SetCursorOffset(-50.0f, -28.0f);
    _language_options_menu.SetSkipDisabled(true);

    _RefreshLanguageOptions();
}

void GameOptionsMenuHandler::_SetupKeySettingsMenu()
{
    _key_settings_menu.ClearOptions();
    _key_settings_menu.SetPosition(512.0f, 468.0f);
    _key_settings_menu.SetDimensions(250.0f, 500.0f, 1, 10, 1, 10);
    _key_settings_menu.SetTextStyle(TextStyle("title20"));
    _key_settings_menu.SetAlignment(VIDEO_X_CENTER, VIDEO_Y_CENTER);
    _key_settings_menu.SetOptionAlignment(VIDEO_X_LEFT, VIDEO_Y_CENTER);
    _key_settings_menu.SetSelectMode(VIDEO_SELECT_SINGLE);
    _key_settings_menu.SetVerticalWrapMode(VIDEO_WRAP_MODE_STRAIGHT);
    _key_settings_menu.SetCursorOffset(-50.0f, -28.0f);
    _key_settings_menu.SetSkipDisabled(true);

    _key_settings_menu.AddOption(UTranslate("Up: "), this, &GameOptionsMenuHandler::_RedefineUpKey);
    _key_settings_menu.AddOption(UTranslate("Down: "), this, &GameOptionsMenuHandler::_RedefineDownKey);
    _key_settings_menu.AddOption(UTranslate("Left: "), this, &GameOptionsMenuHandler::_RedefineLeftKey);
    _key_settings_menu.AddOption(UTranslate("Right: "), this, &GameOptionsMenuHandler::_RedefineRightKey);
    _key_settings_menu.AddOption(UTranslate("Confirm: "), this, &GameOptionsMenuHandler::_RedefineConfirmKey);
    _key_settings_menu.AddOption(UTranslate("Cancel: "), this, &GameOptionsMenuHandler::_RedefineCancelKey);
    _key_settings_menu.AddOption(UTranslate("Menu: "), this, &GameOptionsMenuHandler::_RedefineMenuKey);
    _key_settings_menu.AddOption(UTranslate("Toggle Map: "), this, &GameOptionsMenuHandler::_RedefineMinimapKey);
    _key_settings_menu.AddOption(UTranslate("Pause: "), this, &GameOptionsMenuHandler::_RedefinePauseKey);
    _key_settings_menu.AddOption(UTranslate("Restore defaults"), this, &GameOptionsMenuHandler::_OnRestoreDefaultKeys);
}

void GameOptionsMenuHandler::_SetupJoySettingsMenu()
{
    _joy_settings_menu.ClearOptions();
    _joy_settings_menu.SetPosition(512.0f, 468.0f);
    _joy_settings_menu.SetDimensions(250.0f, 500.0f, 1, 12, 1, 12);
    _joy_settings_menu.SetTextStyle(TextStyle("title20"));
    _joy_settings_menu.SetTextStyle(TextStyle("title22"));
    _joy_settings_menu.SetAlignment(VIDEO_X_CENTER, VIDEO_Y_CENTER);
    _joy_settings_menu.SetOptionAlignment(VIDEO_X_LEFT, VIDEO_Y_CENTER);
    _joy_settings_menu.SetSelectMode(VIDEO_SELECT_SINGLE);
    _joy_settings_menu.SetVerticalWrapMode(VIDEO_WRAP_MODE_STRAIGHT);
    _joy_settings_menu.SetCursorOffset(-50.0f, -28.0f);
    _joy_settings_menu.SetSkipDisabled(true);

    ustring dummy;
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_OnToggleJoystickEnabled, NULL, NULL,
                                 &GameOptionsMenuHandler::_OnToggleJoystickEnabled,
                                 &GameOptionsMenuHandler::_OnToggleJoystickEnabled);
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefineXAxisJoy);
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefineYAxisJoy);
    _joy_settings_menu.AddOption(dummy, this, NULL, NULL, NULL, &GameOptionsMenuHandler::_OnThresholdJoyLeft,
                                 &GameOptionsMenuHandler::_OnThresholdJoyRight);

    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefineConfirmJoy);
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefineCancelJoy);
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefineMenuJoy);
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefineMinimapJoy);
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefinePauseJoy);
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefineHelpJoy);
    _joy_settings_menu.AddOption(dummy, this, &GameOptionsMenuHandler::_RedefineQuitJoy);

    _joy_settings_menu.AddOption(UTranslate("Restore defaults"), this, &GameOptionsMenuHandler::_OnRestoreDefaultJoyButtons);
}

void GameOptionsMenuHandler::_SetupResolutionMenu()
{
    _resolution_menu.SetPosition(442.0f, 468.0f);
    _resolution_menu.SetDimensions(300.0f, 200.0f, 1, 7, 1, 7);
    _resolution_menu.SetTextStyle(TextStyle("title22"));
    _resolution_menu.SetAlignment(VIDEO_X_LEFT, VIDEO_Y_CENTER);
    _resolution_menu.SetOptionAlignment(VIDEO_X_LEFT, VIDEO_Y_CENTER);
    _resolution_menu.SetSelectMode(VIDEO_SELECT_SINGLE);
    _resolution_menu.SetVerticalWrapMode(VIDEO_WRAP_MODE_STRAIGHT);
    _resolution_menu.SetCursorOffset(-50.0f, -28.0f);
    _resolution_menu.SetSkipDisabled(true);

    uint32 res_index = 0;
    if(VideoManager->GetScreenWidth() == 640)
        res_index = 0;
    else if(VideoManager->GetScreenWidth() == 800)
        res_index = 1;
    else if(VideoManager->GetScreenWidth() == 1024)
        res_index = 2;
    else if(VideoManager->GetScreenWidth() == 1280)
        res_index = 3;
    else if(VideoManager->GetScreenWidth() == 1366)
        res_index = 4;
    else if(VideoManager->GetScreenWidth() == 1440)
        res_index = 5;
    else if(VideoManager->GetScreenWidth() == 1600)
        res_index = 6;

    // NOTE: Once on SDL2, append and sort the supported resolutions as well.
    std::vector<std::string> res_list;
    res_list.push_back("640 x 480");
    res_list.push_back("800 x 600");
    res_list.push_back("1024 x 768");
    res_list.push_back("1280 x 1024");
    res_list.push_back("1366 x 768");
    res_list.push_back("1440 x 900");
    res_list.push_back("1600 x 900");

    _resolution_menu.ClearOptions();

    for (uint32 i = 0; i < res_list.size(); ++i) {
        _resolution_menu.AddOption(ustring(), this, &GameOptionsMenuHandler::_OnResolutionConfirm);
        // Show the current selection
        if (res_index == i) {
            _resolution_menu.AddOptionElementImage(i, "img/menus/star.png");
            _resolution_menu.SetSelection(i);
        }
        _resolution_menu.AddOptionElementPosition(i, 32);
        _resolution_menu.AddOptionElementText(i, MakeUnicodeString(res_list[i]));
    }
}

void GameOptionsMenuHandler::_RefreshVideoOptions()
{
    // Update resolution text
    std::ostringstream resolution("");
    resolution << VideoManager->GetScreenWidth() << " x " << VideoManager->GetScreenHeight();
    _video_options_menu.SetOptionText(0, UTranslate("Resolution: ") + MakeUnicodeString(resolution.str()));

    // Update text on current video mode
    if(VideoManager->IsFullscreen())
        _video_options_menu.SetOptionText(1, UTranslate("Window mode: ") + UTranslate("Fullscreen"));
    else
        _video_options_menu.SetOptionText(1, UTranslate("Window mode: ") + UTranslate("Windowed"));

    // Update brightness
    float curr_brightness = VideoManager->GetBrightness();
    uint32 brightness = IsFloatEqual(curr_brightness * 50.0f, 0.0f) ? 0 : (uint32)(curr_brightness * 50.0f + 0.5f);
    _video_options_menu.SetOptionText(2, UTranslate("Brightness: ") + MakeUnicodeString(NumberToString(brightness) + " %"));

    // Update the UI theme.
    _video_options_menu.SetOptionText(3, UTranslate("UI Theme: ") + GUIManager->GetDefaultMenuSkinName());
}

void GameOptionsMenuHandler::_RefreshLanguageOptions()
{
    // Get the list of languages from the Lua file.
    ReadScriptDescriptor read_data;
    if(!read_data.OpenFile(_LANGUAGE_FILE) || !read_data.OpenTable("languages")) {
        PRINT_ERROR << "Failed to load language file: " << _LANGUAGE_FILE << std::endl
                    << "The language list will be empty." << std::endl;
        read_data.CloseFile();
        return;
    }

    uint32 table_size = read_data.GetTableSize();

    // Set up the dimensions of the window according to how many languages are available.
    _language_options_menu.ClearOptions();
    _language_options_menu.SetDimensions(300.0f, 500.0f, 1, table_size, 1, (table_size > 12 ? 12 : table_size));

    // Used to warn about missing po files, but only once at start.
    static bool warnAboutMissingFiles = true;

    _po_files.clear();
    std::string current_language = vt_system::SystemManager->GetLanguage();
    for(uint32 i = 1; i <= table_size; ++i) {
        read_data.OpenTable(i);
        _po_files.push_back(read_data.ReadString(2));

        std::string lang = _po_files[i - 1];
        _language_options_menu.AddOption(ustring(), this,  &GameOptionsMenuHandler::_OnLanguageSelect);
        if (lang == current_language) {
            _language_options_menu.AddOptionElementImage(i - 1, "img/menus/star.png");
            _language_options_menu.SetSelection(i - 1);
        }
        _language_options_menu.AddOptionElementPosition(i - 1, 32);
        _language_options_menu.AddOptionElementText(i - 1, MakeUnicodeString(read_data.ReadString(1)));

        // Test the current language availability
        if (!vt_system::SystemManager->IsLanguageAvailable(lang)) {
            // NOTE: English is always available.
            if (i > 1)
                _language_options_menu.EnableOption(i - 1, false);
            // We also reset the current selection when the current language is unavailable.
            if (lang == current_language)
                _language_options_menu.SetSelection(0);

            if (warnAboutMissingFiles) {
                std::string mo_filename = lang + "/LC_MESSAGES/"APPSHORTNAME".mo";
                PRINT_WARNING << "Couldn't locate gettext .mo file: '" << mo_filename << "'." << std::endl
                    << "The " << lang << " translation will be disabled." << std::endl;
            }
        }

#ifdef DISABLE_TRANSLATIONS
        // If translations are disabled, only admit the first entry (English)
        if (i > 1)
            _language_options_menu.EnableOption(i - 1, false);
        _language_options_menu.SetSelection(0);
#endif
        read_data.CloseTable();
    }

    // Only warn once about missing language files.
    warnAboutMissingFiles = false;

    read_data.CloseTable();
    if(read_data.IsErrorDetected())
        PRINT_ERROR << "Error occurred while loading language list: " << read_data.GetErrorMessages() << std::endl;
    read_data.CloseFile();
}

void GameOptionsMenuHandler::_RefreshAudioOptions()
{
    _audio_options_menu.SetOptionText(0, UTranslate("Sound Volume: ") + MakeUnicodeString(NumberToString(static_cast<int32>(AudioManager->GetSoundVolume() * 100.0f + 0.5f)) + " %"));
    _audio_options_menu.SetOptionText(1, UTranslate("Music Volume: ") + MakeUnicodeString(NumberToString(static_cast<int32>(AudioManager->GetMusicVolume() * 100.0f + 0.5f)) + " %"));
}

void GameOptionsMenuHandler::_RefreshKeySettings()
{
    // Update key names
    _key_settings_menu.SetOptionText(0, UTranslate("Move Up") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetUpKeyName()));
    _key_settings_menu.SetOptionText(1, UTranslate("Move Down") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetDownKeyName()));
    _key_settings_menu.SetOptionText(2, UTranslate("Move Left") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetLeftKeyName()));
    _key_settings_menu.SetOptionText(3, UTranslate("Move Right") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetRightKeyName()));
    _key_settings_menu.SetOptionText(4, UTranslate("Confirm") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetConfirmKeyName()));
    _key_settings_menu.SetOptionText(5, UTranslate("Cancel") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetCancelKeyName()));
    _key_settings_menu.SetOptionText(6, UTranslate("Menu") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetMenuKeyName()));
    _key_settings_menu.SetOptionText(7, UTranslate("Toggle Map") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetMinimapKeyName()));
    _key_settings_menu.SetOptionText(8, UTranslate("Pause") + MakeUnicodeString("<r>") + UTranslate(InputManager->GetPauseKeyName()));
}

void GameOptionsMenuHandler::_RefreshJoySettings()
{
    int32 i = 0;
    if(InputManager->GetJoysticksEnabled())
        _joy_settings_menu.SetOptionText(i++, UTranslate("Input enabled: ") + MakeUnicodeString("<r>") +  UTranslate("Yes"));
    else
        _joy_settings_menu.SetOptionText(i++, UTranslate("Input enabled: ") + MakeUnicodeString("<r>") +  UTranslate("No"));

    _joy_settings_menu.SetOptionText(i++, UTranslate("X Axis") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetXAxisJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Y Axis") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetYAxisJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Threshold") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetThresholdJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Confirm: Button") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetConfirmJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Cancel: Button") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetCancelJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Menu: Button") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetMenuJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Map: Button") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetMinimapJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Pause: Button") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetPauseJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Help: Button") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetHelpJoy())));
    _joy_settings_menu.SetOptionText(i++, UTranslate("Quit: Button") + MakeUnicodeString("<r>" + NumberToString(InputManager->GetQuitJoy())));
}

void GameOptionsMenuHandler::_OnVideoOptions()
{
    _active_menu = &_video_options_menu;
    _RefreshVideoOptions();
}

void GameOptionsMenuHandler::_OnAudioOptions()
{
    // Switch the current menu
    _active_menu = &_audio_options_menu;
    _RefreshAudioOptions();
}

void GameOptionsMenuHandler::_OnLanguageOptions()
{
    // Switch the current menu
    _active_menu = &_language_options_menu;
    _RefreshLanguageOptions();
}

void GameOptionsMenuHandler::_OnKeySettings()
{
    _active_menu = &_key_settings_menu;
    _RefreshKeySettings();
}

void GameOptionsMenuHandler::_OnJoySettings()
{
    _active_menu = &_joy_settings_menu;
    _RefreshJoySettings();
}

void GameOptionsMenuHandler::_OnToggleFullscreen()
{
    // Toggle fullscreen / windowed
    VideoManager->ToggleFullscreen();
    VideoManager->ApplySettings();
    _RefreshVideoOptions();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnResolution()
{
    _active_menu = &_resolution_menu;
}

void GameOptionsMenuHandler::_OnResolutionConfirm()
{
    switch (_resolution_menu.GetSelection()) {
    case 0:
        _ChangeResolution(640, 480);
        break;
    default:
    case 1:
        _ChangeResolution(800, 600);
        break;
    case 2:
        _ChangeResolution(1024, 768);
        break;
    case 3:
        _ChangeResolution(1280, 1024);
        break;
    case 4:
        _ChangeResolution(1366, 768);
        break;
    case 5:
        _ChangeResolution(1440, 900);
        break;
    case 6:
        _ChangeResolution(1600, 900);
        break;
    }
}

void GameOptionsMenuHandler::_OnBrightnessLeft()
{
    VideoManager->SetBrightness(VideoManager->GetBrightness() - 0.1f);
    _RefreshVideoOptions();
}

void GameOptionsMenuHandler::_OnBrightnessRight()
{
    VideoManager->SetBrightness(VideoManager->GetBrightness() + 0.1f);
    _RefreshVideoOptions();
}

void GameOptionsMenuHandler::_OnUIThemeLeft()
{
    GUIManager->SetPreviousDefaultMenuSkin();
    _ReloadGUIDefaultSkin();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnUIThemeRight()
{
    GUIManager->SetNextDefaultMenuSkin();
    _ReloadGUIDefaultSkin();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnSoundLeft()
{
    AudioManager->SetSoundVolume(AudioManager->GetSoundVolume() - 0.1f);
    _RefreshAudioOptions();
    // Play a sound for user to hear new volume level.
    GlobalManager->Media().PlaySound("volume_test");
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnSoundRight()
{
    AudioManager->SetSoundVolume(AudioManager->GetSoundVolume() + 0.1f);
    _RefreshAudioOptions();
    // Play a sound for user to hear new volume level.
    GlobalManager->Media().PlaySound("volume_test");
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnMusicLeft()
{
    AudioManager->SetMusicVolume(AudioManager->GetMusicVolume() - 0.1f);
    _RefreshAudioOptions();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnMusicRight()
{
    AudioManager->SetMusicVolume(AudioManager->GetMusicVolume() + 0.1f);
    _RefreshAudioOptions();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnLanguageSelect()
{
    std::string language = _po_files[_language_options_menu.GetSelection()];
    // Reset the language in case changing failed.
    if (!SystemManager->SetLanguage(language)) {
        _RefreshLanguageOptions();
        return;
    }

    // Reload the font according to the newly selected language.
    TextManager->LoadFonts(language);

    _has_modified_settings = true;

    // Reloads the theme names before the menus
    GUIManager->ReloadSkinNames("dat/config/themes.lua");

    // Reload all the translatable text in the menus.
    ReloadTranslatableMenus();

    // Reloads the global scripts to update their inner translatable strings
    GlobalManager->ReloadGlobalScripts();

    _RefreshLanguageOptions();
}

void GameOptionsMenuHandler::_OnRestoreDefaultKeys()
{
    InputManager->RestoreDefaultKeys();
    _RefreshKeySettings();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnToggleJoystickEnabled()
{
    InputManager->SetJoysticksEnabled(!InputManager->GetJoysticksEnabled());
    if (InputManager->GetJoysticksEnabled())
        InputManager->InitializeJoysticks();
    else
        InputManager->DeinitializeJoysticks();

    _RefreshJoySettings();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnThresholdJoyLeft()
{
    InputManager->SetThresholdJoy(InputManager->GetThresholdJoy() - 100);
    _RefreshJoySettings();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnThresholdJoyRight()
{
    InputManager->SetThresholdJoy(InputManager->GetThresholdJoy() + 100);
    _RefreshJoySettings();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_OnRestoreDefaultJoyButtons()
{
    InputManager->RestoreDefaultJoyButtons();
    _RefreshJoySettings();
    _has_modified_settings = true;
}

void GameOptionsMenuHandler::_ShowMessageWindow(bool joystick)
{
    if(joystick)
        _ShowMessageWindow(WAIT_JOY_BUTTON);
    else
        _ShowMessageWindow(WAIT_KEY);
}

void GameOptionsMenuHandler::_ShowMessageWindow(WAIT_FOR wait)
{
    if(wait == WAIT_JOY_BUTTON)
        _message_window.SetText(UTranslate("Please press a new joystick button."));
    else if(wait == WAIT_KEY)
        _message_window.SetText(UTranslate("Please press a new key."));
    else if(wait == WAIT_JOY_AXIS)
        _message_window.SetText(UTranslate("Please move an axis."));
    else {
        PRINT_WARNING << "Undefined wait value." << std::endl;
        return;
    }

    _message_window.Show();
}

bool GameOptionsMenuHandler::_ChangeResolution(int32 width, int32 height)
{
    if (VideoManager->GetScreenWidth() == width &&
            VideoManager->GetScreenHeight() == height)
        return false;

    VideoManager->SetResolution(width, height);

    bool ret_value = VideoManager->ApplySettings();
    if (ret_value)
        _has_modified_settings = true;

    _RefreshVideoOptions();
    _SetupResolutionMenu();
    return ret_value;
}

void GameOptionsMenuHandler::_ReloadGUIDefaultSkin()
{
    _options_window.Destroy();
    _options_window.Create(300.0f, 550.0f);
    _options_window.SetPosition(360.0f, 188.0f);
    _options_window.Show();

    // Setup all menu options and properties
    _SetupOptionsMenu();
    _SetupVideoOptionsMenu();
    _SetupAudioOptionsMenu();
    _SetupLanguageOptionsMenu();
    _SetupKeySettingsMenu();
    _SetupJoySettingsMenu();
    _SetupResolutionMenu();

    _active_menu = &_video_options_menu;

    // Currently, the GUI default skin option is 3.
    _video_options_menu.SetSelection(3);
    _RefreshVideoOptions();
}

bool GameOptionsMenuHandler::_SaveSettingsFile(const std::string& filename)
{
    // No need to save the settings if we haven't edited anything!
    if(!_has_modified_settings)
        return false;

    std::string file;
    std::string fileTemp;

    // Load the settings file for reading in the original data
    fileTemp = GetUserConfigPath() + "/settings.lua";

    if(filename.empty())
        file = fileTemp;
    else
        file = GetUserConfigPath() + "/" + filename;

    //copy the default file so we have an already set up lua file and then we can modify its settings
    if(!DoesFileExist(file))
        CopyFile(std::string("dat/config/settings.lua"), file);

    WriteScriptDescriptor settings_lua;
    if(!settings_lua.OpenFile(file)) {
        PRINT_ERROR << "Failed to open settings file: " <<
            file << std::endl;
        return false;
    }

    settings_lua.WriteComment("--General settings--");
    settings_lua.BeginTable("settings");

    // Write the current settings into the .lua file
    settings_lua.WriteComment("Show the first time help window");
    settings_lua.WriteInt("first_start", 0);

    //Save language settings
    settings_lua.WriteComment("The GUI and in game dialogues language used");
    settings_lua.WriteString("language", SystemManager->GetLanguage());

    // video
    settings_lua.InsertNewLine();
    settings_lua.WriteComment("--Video settings--");
    settings_lua.BeginTable("video_settings");
    settings_lua.WriteComment("Screen resolution");
    settings_lua.WriteInt("screen_resx", VideoManager->GetScreenWidth());
    settings_lua.WriteInt("screen_resy", VideoManager->GetScreenHeight());
    settings_lua.WriteComment("Run the screen fullscreen/in a window");
    settings_lua.WriteBool("full_screen", VideoManager->IsFullscreen());
    settings_lua.WriteComment("The UI Theme to load.");
    settings_lua.WriteString("ui_theme", GUIManager->GetDefaultMenuSkinId());
    settings_lua.EndTable(); // video_settings

    // audio
    settings_lua.InsertNewLine();
    settings_lua.WriteComment("--Audio settings--");
    settings_lua.BeginTable("audio_settings");
    settings_lua.WriteComment("Music and sounds volumes: [0.0 - 1.0]");
    settings_lua.WriteFloat("music_vol", AudioManager->GetMusicVolume());
    settings_lua.WriteFloat("sound_vol", AudioManager->GetSoundVolume());
    settings_lua.EndTable(); // audio_settings

    // input
    settings_lua.InsertNewLine();
    settings_lua.WriteComment("--Keyboard settings--");
    settings_lua.BeginTable("key_settings");
    settings_lua.WriteComment("Keyboard key SDL values.");
    settings_lua.WriteInt("up", InputManager->GetUpKey());
    settings_lua.WriteInt("down", InputManager->GetDownKey());
    settings_lua.WriteInt("left", InputManager->GetLeftKey());
    settings_lua.WriteInt("right", InputManager->GetRightKey());
    settings_lua.WriteInt("confirm", InputManager->GetConfirmKey());
    settings_lua.WriteInt("cancel", InputManager->GetCancelKey());
    settings_lua.WriteInt("menu", InputManager->GetMenuKey());
    settings_lua.WriteInt("minimap", InputManager->GetMinimapKey());
    settings_lua.WriteInt("pause", InputManager->GetPauseKey());
    settings_lua.EndTable(); // key_settings

    settings_lua.InsertNewLine();
    settings_lua.WriteComment("--Joystick settings--");
    settings_lua.BeginTable("joystick_settings");
    settings_lua.WriteComment("Tells whether joysticks input should be taken in account");
    settings_lua.WriteBool("input_disabled", (!InputManager->GetJoysticksEnabled()));
    settings_lua.WriteComment("The axis index number to be used as x/y axis");
    settings_lua.WriteInt("x_axis", InputManager->GetXAxisJoy());
    settings_lua.WriteInt("y_axis", InputManager->GetYAxisJoy());
    settings_lua.WriteComment("The joystick x/y axis dead zone [0-N] (Default: 8192)");
    settings_lua.WriteInt("threshold", InputManager->GetThresholdJoy());
    settings_lua.WriteComment("Joystick keys index number [0-N] (0 is the first button)");
    settings_lua.WriteInt("confirm", InputManager->GetConfirmJoy());
    settings_lua.WriteInt("cancel", InputManager->GetCancelJoy());
    settings_lua.WriteInt("menu", InputManager->GetMenuJoy());
    settings_lua.WriteInt("minimap", InputManager->GetMinimapJoy());
    settings_lua.WriteInt("pause", InputManager->GetPauseJoy());
    settings_lua.WriteInt("help", InputManager->GetHelpJoy());
    settings_lua.WriteInt("quit", InputManager->GetQuitJoy());
    settings_lua.EndTable(); // joystick_settings

    // game
    settings_lua.InsertNewLine();
    settings_lua.WriteComment("--Game settings--");
    settings_lua.BeginTable("game_options");
    std::stringstream speed_text("");
    speed_text << "Speed of text displayed in dialogues (in characters per seconds) [1-N] (Default: "
               << vt_gui::DEFAULT_MESSAGE_SPEED << ")";
    settings_lua.WriteComment(speed_text.str());
    settings_lua.WriteInt("message_speed", SystemManager->GetMessageSpeed());
    settings_lua.EndTable(); // game_options

    settings_lua.EndTable(); // settings

    // and save it!
    settings_lua.SaveFile();
    settings_lua.CloseFile();

    _has_modified_settings = false;

    return true;
} // bool GameOptionsMenuHandler::_SaveSettingsFile(const std::string& filename)

// ****************************************************************************
// ***** Handler input configuration methods
// ****************************************************************************

void GameOptionsMenuHandler::_RedefineUpKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetUpKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_RedefineDownKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetDownKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_RedefineLeftKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetLeftKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_RedefineRightKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetRightKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_RedefineConfirmKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetConfirmKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_RedefineCancelKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetCancelKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_RedefineMenuKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetMenuKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_RedefineMinimapKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetMinimapKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_RedefinePauseKey()
{
    _key_setting_function = &GameOptionsMenuHandler::_SetPauseKey;
    _ShowMessageWindow(false);
}

void GameOptionsMenuHandler::_SetUpKey(const SDLKey &key)
{
    InputManager->SetUpKey(key);
}

void GameOptionsMenuHandler::_SetDownKey(const SDLKey &key)
{
    InputManager->SetDownKey(key);
}

void GameOptionsMenuHandler::_SetLeftKey(const SDLKey &key)
{
    InputManager->SetLeftKey(key);
}

void GameOptionsMenuHandler::_SetRightKey(const SDLKey &key)
{
    InputManager->SetRightKey(key);
}

void GameOptionsMenuHandler::_SetConfirmKey(const SDLKey &key)
{
    InputManager->SetConfirmKey(key);
}

void GameOptionsMenuHandler::_SetCancelKey(const SDLKey &key)
{
    InputManager->SetCancelKey(key);
}

void GameOptionsMenuHandler::_SetMenuKey(const SDLKey &key)
{
    InputManager->SetMenuKey(key);
}

void GameOptionsMenuHandler::_SetMinimapKey(const SDLKey &key)
{
    InputManager->SetMinimapKey(key);
}

void GameOptionsMenuHandler::_SetPauseKey(const SDLKey &key)
{
    InputManager->SetPauseKey(key);
}

void GameOptionsMenuHandler::_RedefineXAxisJoy()
{
    _joy_axis_setting_function = &GameOptionsMenuHandler::_SetXAxisJoy;
    _ShowMessageWindow(WAIT_JOY_AXIS);
    InputManager->ResetLastAxisMoved();
}

void GameOptionsMenuHandler::_RedefineYAxisJoy()
{
    _joy_axis_setting_function = &GameOptionsMenuHandler::_SetYAxisJoy;
    _ShowMessageWindow(WAIT_JOY_AXIS);
    InputManager->ResetLastAxisMoved();
}

void GameOptionsMenuHandler::_RedefineConfirmJoy()
{
    _joy_setting_function = &GameOptionsMenuHandler::_SetConfirmJoy;
    _ShowMessageWindow(true);
}

void GameOptionsMenuHandler::_RedefineCancelJoy()
{
    _joy_setting_function = &GameOptionsMenuHandler::_SetCancelJoy;
    _ShowMessageWindow(true);
}

void GameOptionsMenuHandler::_RedefineMenuJoy()
{
    _joy_setting_function = &GameOptionsMenuHandler::_SetMenuJoy;
    _ShowMessageWindow(true);
}

void GameOptionsMenuHandler::_RedefineMinimapJoy()
{
    _joy_setting_function = &GameOptionsMenuHandler::_SetMinimapJoy;
    _ShowMessageWindow(true);
}

void GameOptionsMenuHandler::_RedefinePauseJoy()
{
    _joy_setting_function = &GameOptionsMenuHandler::_SetPauseJoy;
    _ShowMessageWindow(true);
}

void GameOptionsMenuHandler::_RedefineHelpJoy()
{
    _joy_setting_function = &GameOptionsMenuHandler::_SetHelpJoy;
    _ShowMessageWindow(true);
}

void GameOptionsMenuHandler::_RedefineQuitJoy()
{
    _joy_setting_function = &GameOptionsMenuHandler::_SetQuitJoy;
    _ShowMessageWindow(true);
}

void GameOptionsMenuHandler::_SetXAxisJoy(int8 axis)
{
    InputManager->SetXAxisJoy(axis);
}

void GameOptionsMenuHandler::_SetYAxisJoy(int8 axis)
{
    InputManager->SetYAxisJoy(axis);
}

void GameOptionsMenuHandler::_SetConfirmJoy(uint8 button)
{
    InputManager->SetConfirmJoy(button);
}

void GameOptionsMenuHandler::_SetCancelJoy(uint8 button)
{
    InputManager->SetCancelJoy(button);
}

void GameOptionsMenuHandler::_SetMenuJoy(uint8 button)
{
    InputManager->SetMenuJoy(button);
}

void GameOptionsMenuHandler::_SetMinimapJoy(uint8 button)
{
    InputManager->SetMinimapJoy(button);
}

void GameOptionsMenuHandler::_SetPauseJoy(uint8 button)
{
    InputManager->SetPauseJoy(button);
}

void GameOptionsMenuHandler::_SetHelpJoy(uint8 button)
{
    InputManager->SetHelpJoy(button);
}

void GameOptionsMenuHandler::_SetQuitJoy(uint8 button)
{
    InputManager->SetQuitJoy(button);
}

} // namespace private_gui

} // namespace vt_gui
