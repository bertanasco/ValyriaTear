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
*** \file    textbox.h
*** \author  Raj Sharma, roos@allacrost.org
*** \author  Yohann Ferreira, yohann ferreira orange fr
*** \brief   Header file for TextBox class
***
*** The TextBox class is a GUI control which lets you define a rectangular area
*** of the screen to display text in.
*** ***************************************************************************/


#ifndef __TEXTBOX_HEADER__
#define __TEXTBOX_HEADER__

#include "gui.h"
#include "engine/system.h"
#include "engine/video/text.h"
#include "engine/video/screen_rect.h"

namespace vt_gui
{

//! \brief The default speed at which the text will appear, for non-instant text, in character/second.
const float DEFAULT_MESSAGE_SPEED = 45.0f;

namespace private_gui
{

//! \brief Assume this many characters per line of text when calculating display speed for textboxes
const uint32 CHARS_PER_LINE = 30;

} // namespace private_gui

/** ****************************************************************************
*** \brief These text display modes control how the text is rendered.
*** - VIDEO_TEXT_INSTANT: render the text instantly
*** - VIDEO_TEXT_CHAR: render the text one character at a time
*** - VIDEO_TEXT_FADELINE: fade each line in one at a time
*** - VIDEO_TEXT_FADECHAR: fades in each character at a time
*** - VIDEO_TEXT_REVEAL: goes left to right and reveals the text one pixel column at a time
*** - VIDEO_TEXT_FADEREVEAL: like REVEAL, except as text gets revealed it fades in
*** ***************************************************************************/
enum TEXT_DISPLAY_MODE {
    VIDEO_TEXT_INSTANT    = 0,
    VIDEO_TEXT_CHAR       = 1,
    VIDEO_TEXT_FADELINE   = 2,
    VIDEO_TEXT_FADECHAR   = 3,
    VIDEO_TEXT_REVEAL     = 4,
    VIDEO_TEXT_TOTAL      = 5
};


/** ****************************************************************************
*** \brief Class for representing an invisible box for rendering text to.
*** This class provides word wrapping, and "gradual display",
*** such as drawing one character at a time or fading each line
*** of text in individually.
***
*** \note The alignment flags affect the textbox as a whole, but not the actual text
*** drawn in the textbox.
*** \note This class is based on UNICODE text. If you try to use it for regular
*** strings, it will automatically convert it and store it internally as wide strings.
*** ***************************************************************************/
class TextBox : public private_gui::GUIControl
{
public:
    TextBox();

    TextBox(float x, float y, float width, float height, const TEXT_DISPLAY_MODE &mode = VIDEO_TEXT_INSTANT);

    ~TextBox()
    {}

    //! \brief Removes all text from the text box
    void ClearText();

    /** \brief Updates the amount of text that has been rendered.
    *** \param frame_time The amount of milliseconds that have transpired since the last frame.
    *** This must be called every frame in order to update the gradual display of text.
    **/
    void Update(uint32 frame_time = vt_system::SystemManager->GetUpdateTime());

    /** \brief Renders the textbox to the screen back buffer.
    *** Note that the rendering is not affected by any draw flags or coordinate system settings,.
    *** This function will use whatever has been set for it using the Set*() calls
    **/
    void Draw();

    /** \brief If text is in the middle of gradual rendering, this forces it to complete.
    *** This is useful if a player gets impatient while text is scrolling to the screen.
    **/
    void ForceFinish() {
        if(_text.empty() == true) return;
        _finished = true;
    }

    /** \brief Sets the width and height of the text box
    *** \param w The width to set for the text box (for a 1024x768 coordinate system).
    *** \param h The height to set for the text box (for a 1024x768 coordinate system).
    *** If the width or height are negative, or if they are larger than 1024 or 768 respectively,
    *** then the function will not change the properties of the textbox.
    **/
    void SetDimensions(float w, float h);

    /** \brief Set the x and y alignments for the text.
    *** \param xalign The x alignment, e.g. VIDEO_X_LEFT
    *** \param yalign The y alignment, e.g. VIDEO_Y_TOP
    **/
    void SetTextAlignment(int32 xalign, int32 yalign);

    /** \brief Sets the text style to use for this textbox.
    *** \param style The style intended \see #TextStyle
    **/
    void SetTextStyle(const vt_video::TextStyle &style);

    /** \brief Sets the current text display mode (e.g. fading lines of text, etc.)
    *** \param mode The display mode to use for the text.
    **/
    void SetDisplayMode(const TEXT_DISPLAY_MODE &mode);

    /** \brief Sets the text display speed (in characters per second).
    *** \param display_speed The display speed, in terms of characters per second.
    *** If the current display mode is one line at a time, then the display speed is based
    *** on VIDEO_CHARS_PER_LINE characters per line, so for example, a display speed of 10
    *** would mean 3 seconds per line if VIDEO_CHARS_PER_LINE is 30.
    *** \note This has no effect for textboxes using the VIDEO_TEXT_INSTANT display mode.
    **/
    void SetDisplaySpeed(float display_speed);

    /** \brief Sets the text for this box to the string passed in.
    *** \param text The text to display for the textbox
    *** \note If you use a gradual text display mode like VIDEO_TEXT_CHAR, then the text
    *** will be displayed gradually and when it's done displaying, IsFinished() will return true.
    *** \note This function checks the text passed in if it's too big for the textbox and inserts
    *** new lines where appropriate. If the text is so big that it can't fit even with word
    *** wrapping, an error is printed to the console if debugging is enabled.
    **/
    void SetDisplayText(const vt_utils::ustring &text);

    /** \brief A non-unicode version of SetDisplayText().
    *** \param text The text to be set in the box (a standard non-unicode string).
    *** See the unicode version of SetDisplayText for more details.
    **/
    void SetDisplayText(const std::string &text);

    /** \brief Retrieve the current x and y alignments for the text
    *** \param xalign The member to hold the x alignment (e.g. VIDEO_X_LEFT).
    *** \param yalign The member to hold the y alignment (e.g. VIDEO_Y_TOP).
    **/
    void GetTextAlignment(int32 &xalign, int32 &yalign) {
        xalign = _text_xalign;
        yalign = _text_yalign;
    }

    /** \brief  Gets the current text style for this textbox
    *** \return The current text style.
    **/
    const vt_video::TextStyle &GetTextStyle() {
        return _text_style;
    }

    //! \brief Return the current text display mode that is set for this textbox.
    TEXT_DISPLAY_MODE GetDisplayMode() const {
        return _mode;
    }

    //! \brief Return the current text display speed, in characters per second.
    float GetDisplaySpeed() const {
        return _display_speed;
    }

    //! \brief Returns the text currently being displayed by the textbox.
    vt_utils::ustring GetText() const {
        return (_mode == VIDEO_TEXT_INSTANT) ? _text_image.GetString() : _text_save;
    }

    /** \brief Returns true if this textbox is finished with its gradual display of text
    *** \note If you create a textbox but don't draw any text on it, the finished property
    *** will be false. Only after the text is drawn to it will this method return true.
    **/
    bool IsFinished() const {
        return _finished;
    }

    //! \brief Returns true if this text box contains no text empty.
    bool IsEmpty() const {
        return (_mode == VIDEO_TEXT_INSTANT) ? _text_image.GetString().empty() : _text.empty();
    }

private:
    //! \brief The display speed of the text, in characters per second.
    float _display_speed;

    //! \brief Alignment flags for the textbox.
    int32 _text_xalign, _text_yalign;

    //! \brief Hold the number of characters in the entire text.
    uint32 _num_chars;

    //! \brief True if the text being drawn by SetDisplayText() is done (in the case of gradual rendering).
    bool _finished;

    //! \brief The number of milliseconds that have passed since SetDisplayText() was called.
    uint32 _current_time;

    //! \brief The number of milliseconds remaining until the gradual text display will be complete.
    uint32 _end_time;

    //! \brief The rendered image of the text stored in the text box
    vt_video::TextImage _text_image;

    //! \brief The text style for this textbox
    vt_video::TextStyle _text_style;

    //! \brief The display mode for the text (one character at a time, fading in, instant, etc.).
    TEXT_DISPLAY_MODE _mode;

    //! \brief An array of wide strings, one for each line of text.
    std::vector<vt_utils::ustring> _text;

    //! \brief The unedited text for reformatting
    vt_utils::ustring _text_save;

    //! \brief Cache data for textbox drawing
    //! Recomputed in ReformatText()
    // The textBox text scissoring screen rectangle.
    vt_video::ScreenRect _scissor_rect;
    // Holds the height of the text to be drawn
    float _text_height;
    // Holds the actual x and y position where the text should be drawn
    float _text_xpos;
    float _text_ypos;

    /** \brief Draws the textbox text, taking the display mode into account.
    *** \param text_x The x value to use, depending on the alignment.
    *** \param text_y The y value to use, depending on the alignment.
    *** \param scissor_rect The scissor rectangle used for this textbox.
    **/
    void _DrawTextLines(float text_x, float text_y, vt_video::ScreenRect scissor_rect);

    /** \brief Reformats text for size/font.
    **/
    void _ReformatText();

    /** \brief Returns the height of the text when it's rendered with the current font
    *** \return The height of text rendered in current font
    *** \note This is a low-level function so it doesn't check if the current font is valid or not
    **/
    float _CalculateTextHeight();

    /** \brief Draws an outline of the element boundaries
    *** \note This function also draws an outline for each line of text in addition to the textbox
    *** as a whole.
    **/
    void _DEBUG_DrawOutline();

}; // class TextBox : public GUIControl

} // namespace vt_gui

#endif  // __TEXTBOX_HEADER__
