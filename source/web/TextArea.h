/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2015-2017
 *
 *  @file  TextArea.h
 *  @brief Specs for the TextArea widget.
 *
 *
 * @todo Callback does a lot of string-copies at the moment; should be streamlined.
 */

#ifndef EMP_WEB_TEXT_AREA_H
#define EMP_WEB_TEXT_AREA_H

#include "Widget.h"

namespace emp {
namespace web {

  /// An input field for text data.  A function provided at creation time will be called
  /// each time the contents of the TextWidget are changed.  The current text contents
  /// can also always be accessed with the GetText() member function.

  class TextArea : public internal::WidgetFacet<TextArea> {
    friend class TextAreaInfo;
  protected:

    // TextAreas associated with the same DOM element share a single TextAreaInfo object.
    class TextAreaInfo : public internal::WidgetInfo {
      friend TextArea;
    protected:
      int cols;                 ///< How many columns of text in the area?
      int rows;                 ///< How many rows of text in the area?
      int max_length;           ///< Maximum number of total characters allowed.

      std::string cur_text;     ///< Text that should currently be in the box.

      bool autofocus;
      bool disabled;

      std::function<void(const std::string &)> callback;
      uint32_t callback_id;

      TextAreaInfo(const std::string & in_id="") : internal::WidgetInfo(in_id) { ; }
      TextAreaInfo(const TextAreaInfo &) = delete;               // No copies of INFO allowed
      TextAreaInfo & operator=(const TextAreaInfo &) = delete;   // No copies of INFO allowed
      virtual ~TextAreaInfo() {
        if (callback_id) emp::JSDelete(callback_id);             // Delete callback wrapper.
      }

      std::string TypeName() const override { return "TextAreaInfo"; }
      virtual bool IsTextAreaInfo() const override { return true; }

      void DoCallback(std::string in_text) {
        cur_text = in_text;
        if (callback) callback(cur_text);
        UpdateDependants();
      }

      virtual void GetHTML(std::stringstream & HTML) override {
        HTML.str("");                                           // Clear the current text.
        HTML << "<textarea ";                                   // Start the textarea tag.
        if (disabled) { HTML << " disabled=true"; }             // Check if should be disabled
        HTML << " id=\"" << id << "\"";                         // Indicate ID.
        HTML << " onkeyup=\"emp.Callback(" << callback_id << ", $(this).val())\"";
        HTML << " rows=\"" << rows << "\""
             << " cols=\"" << cols << "\"";
        if (max_length >= 0) { HTML << " maxlength=\"" << max_length << "\""; }
        HTML << ">" << cur_text << "</textarea>";              // Close and label the textarea
      }

      void UpdateAutofocus(bool in_af) {
        autofocus = in_af;
        if (state == Widget::ACTIVE) ReplaceHTML();     // If node is active, immediately redraw!
      }

      void UpdateCallback(const std::function<void(const std::string &)> & in_cb) {
        callback = in_cb;
      }

      void UpdateDisabled(bool in_dis) {
        disabled = in_dis;
        if (state == Widget::ACTIVE) ReplaceHTML();     // If node is active, immediately redraw!
      }

      void UpdateText(const std::string & in_string) {
        EM_ASM_ARGS({
            var id = Pointer_stringify($0);
            var text = Pointer_stringify($1);
            $('#' + id).val(text);
          }, id.c_str(), in_string.c_str());
      }

    public:
      virtual std::string GetType() override { return "web::TextAreaInfo"; }
    }; // End of TextAreaInfo definition


    // Get a properly cast version of indo.
    TextAreaInfo * Info() { return (TextAreaInfo *) info; }
    const TextAreaInfo * Info() const { return (TextAreaInfo *) info; }

    TextArea(TextAreaInfo * in_info) : WidgetFacet(in_info) { ; }

  public:
    /// Build a text area with a specified HTML identifier.
    TextArea(const std::string & in_id="")
      : WidgetFacet(in_id)
    {
      info = new TextAreaInfo(in_id);

      Info()->cols = 20;
      Info()->rows = 1;
      Info()->max_length = -1;
      Info()->cur_text = "";
      Info()->autofocus = false;
      Info()->disabled = false;

      Info()->callback_id = 0;
    }

    /// Build a text area with a specified function to call with every change.
    TextArea(std::function<void(const std::string &)> in_cb, const std::string & in_id="")
      : TextArea(in_id)
    {
      Info()->callback = in_cb;
      TextAreaInfo * ta_info = Info();
      Info()->callback_id = JSWrap( std::function<void(std::string)>(
        [ta_info](std::string in_str){ ta_info->DoCallback(in_str); }
      ));
    }

    /// Connect to an existing TextArea
    TextArea(const TextArea & in) : WidgetFacet(in) { ; }
    TextArea(const Widget & in) : WidgetFacet(in) { emp_assert(info->IsTextAreaInfo()); }
    virtual ~TextArea() { ; }

    using INFO_TYPE = TextAreaInfo;

    bool GetDisabled() const { return Info()->disabled; }

    /// Get the current text in this TextArea.
    const std::string & GetText() const { return Info()->cur_text; }

    TextArea & SetAutofocus(bool in_af) { Info()->UpdateAutofocus(in_af); return *this; }

    /// Change the callback function for this TextArea.
    TextArea & SetCallback(const std::function<void(const std::string &)> & in_cb) {
      Info()->UpdateCallback(in_cb);
      return *this;
    }
    TextArea & SetDisabled(bool in_dis) { Info()->UpdateDisabled(in_dis); return *this; }

    /// Set the text contained in the text area.
    TextArea & SetText(const std::string & in_text) {
      Info()->cur_text = in_text;
      Info()->UpdateText(in_text);
      return *this;
    }

    bool HasAutofocus() const { return Info()->autofocus; }
    bool IsDisabled() const { return Info()->disabled; }
  };


}
}

#endif