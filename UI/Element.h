#ifndef EMP_UI_ELEMENT_H
#define EMP_UI_ELEMENT_H

/////////////////////////////////////////////////////////////////////////////////////////
//
//  Base class for a single element on a web page (a paragraph, a button, a table, etc.)
//

#include <emscripten.h>
#include <sstream>
#include <string>
#include <typeinfo>

#include "../tools/assert.h"
#include "../tools/string_utils.h"

#include "events.h"
#include "UI_base.h"

namespace emp {
namespace UI {

  class Element {
  protected:
    std::string name;        // Unique DOM id for this element.
    std::stringstream HTML; // Full HTML contents for this element.

    // Track hiearchy
    Element * parent;
    std::vector<Element *> children;

    // UpdateHTML() makes sure that the HTML stream above id up-to-date.
    virtual void UpdateHTML() { ; }
    virtual void UpdateCSS() { ; }

    // If an Append doesn't work with the currnet class, forward it to the parent!
    template <typename FORWARD_TYPE>
    Element & AppendParent(FORWARD_TYPE && arg) {
      emp_assert(parent != nullptr);
      return parent->Append(std::forward<FORWARD_TYPE>(arg));
    }

  public:
    Element(const std::string & in_name, Element * in_parent)
      : name(in_name), parent(in_parent)
    {
      emp_assert(name.size() > 0);  // Make sure a name was included.
      // Ensure the name consists of just alphanumeric chars (plus '_' & '-'?)
      emp_assert( emp::is_valid(name,
                                { emp::is_alphanumeric,
                                  [](char x){return x=='_' || x=='-';}}) );
      Register(this);
    }
    virtual ~Element() {
      // Recursively delete children.
      for (Element * cur_element : children) { delete cur_element; }
    }

    // Do not allow elements to be copied.
    Element(const Element &) = delete;
    Element & operator=(const Element &) = delete;

    // Functions to access current state
    virtual bool IsText() const { return false; }

    const std::string & GetName() const { return name; }
    Element * GetParent() { return parent; }

    // Functions to access children
    int GetNumChildren() const { return children.size(); }
    Element & GetChild(int id) {
      emp_assert(id >= 0 && id < children.size());
      return *(children[id]);
    }
    const Element & GetChild(int id) const {
      emp_assert(id >= 0 && id < children.size());
      return *(children[id]);
    }


    // Register is used so we can lookup classes by name.
    // Overridden in classes that manage multiple element; below is the default version.
    virtual void Register(Element * new_element) {
      if (parent) parent->Register(new_element);
    }


    // UpdateNow() refreshes the document immediately (and should only be called if that's okay!)
    // By default: call UpdateHTML (which should be overridden) print HTML_string, and UpdateCSS
    virtual void UpdateNow() {
      UpdateHTML();
      EM_ASM_ARGS({
          var elem_name = Pointer_stringify($0);
          var html_str = Pointer_stringify($1);
          $( '#' + elem_name ).html(html_str);
        }, GetName().c_str(), HTML.str().c_str() );
      UpdateCSS();

      // Now that the parent is up-to-day, update all children.
      for (auto * child : children) child->UpdateNow();
    }


    // Update() refreshes the document once it's ready.
    void Update() {
      // OnDocumentReady( [this](){ this->UpdateNow(); } );
      OnDocumentReady( std::function<void(void)>([this](){ this->UpdateNow(); }) );
    }


    // By default, elements should forward unknown inputs to their parents.

    virtual Element & Append(const std::string & text) { return AppendParent(text); }
    virtual Element & Append(const std::function<std::string()> & fun) { return AppendParent(fun); }
    virtual Element & Append(emp::UI::Button info) { return AppendParent(info); }
    virtual Element & Append(emp::UI::Image info) { return AppendParent(info); }
    virtual Element & Append(emp::UI::Table info) { return AppendParent(info); }

    // Convert arbitrary inputs to a string and try again!
    virtual Element & Append(char in_char) { return Append(emp::to_string(in_char)); }
    virtual Element & Append(double in_num) { return Append(emp::to_string(in_num)); }
    virtual Element & Append(int in_num) { return Append(emp::to_string(in_num)); }


    // Setup << operator to redirect to Append.
    template <typename IN_TYPE>
    Element & operator<<(IN_TYPE && in_val) { return Append(std::forward<IN_TYPE>(in_val)); }


    // Print out the contents of this element as HTML.
    virtual void PrintHTML(std::ostream & os) {
      UpdateHTML();
      os << HTML.str();
    }

    void AlertHTML() {
      std::stringstream ss;
      PrintHTML(ss);
      emp::Alert(ss.str());
    }
  };

};
};

#endif
