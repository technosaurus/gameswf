// gameswf_xml.h	-- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain. Do whatever
// you want with it.

#ifndef __TEXTFORMAT_H__
#define __TEXTFORMAT_H__

#include <string>

#include "gameswf_log.h"
#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"

namespace gameswf
{  

class text_format
{
public:
  // new text_format([font, [size, [color, [bold, [italic, [underline, [url, [target, [align,
  //                [leftMargin, [rightMargin, [indent, [leading]]]]]]]]]]]]])
  
  text_format();
  text_format(tu_string font);
  text_format(tu_string font, int size);
  text_format(tu_string font, int size, int color);
  ~text_format();
  
  bool underlined()  { return _underline; }
  bool italiced()    { return _italic; }
  bool bold()        { return _bold; }
  bool bullet()      { return _bullet; }
  bool color()       { return _color; }
  bool indent()      { return _indent; }
  bool align()       { return _align; }
  bool blockIndent() { return _block_indent; }
  bool leading()     { return _leading; }
  bool leftMargin()  { return _left_margin; }
  bool RightMargin() { return _right_margin; }
  bool size()        { return _point_size; }

  // In a paragraph, change the format of a range of characters.
  void setTextFormat (text_format &format);
  void setTextFormat (int index, text_format &format);
  void setTextFormat (int start, int end, text_format &format);

  int getTextExtant();

 private:
  bool	_underline;		// A Boolean value that indicates whether the text is underlined.
  bool	_bold;			// A Boolean value that indicates whether the text is boldface.
  bool	_italic;		// A Boolean value that indicates whether the text is italicized.
  bool	_bullet;		// 
  
  tu_string	 _align;	// The alignment of the paragraph, represented as a string.
                                // If "left", the paragraph is left-aligned. If "center", the
                                // paragraph is centered. If "right", the paragraph is
                                // right-aligned.
  int		_block_indent;	// 
  int		_color;		// The color of text using this text format. A number
                                // containing three 8-bit RGB components; for example,
                                // 0xFF0000 is red, 0x00FF00 is green.
  tu_string _font;		// The name of a font for text as a string.
  int		_indent;	// An integer that indicates the indentation from the left
                                // margin to the first character in the paragraph
  int		_leading;	// A number that indicates the amount of leading vertical
                                // space between lines.
  int		_left_margin;	// Indicates the left margin of the paragraph, in points.
  int		_right_margin;	// Indicates the right margin of the paragraph, in points.
  int		_point_size;	// An integer that indicates the point size.
  int		_tab_stops;	// 
  int		_target;	// The target window where the hyperlink is displayed. If the
                                // target window is an empty string, the text is displayed in
                                // the default target window _self. If the url parameter is
                                // set to an empty string or to the value null, you can get
                                // or set this property, but the property will have no effect.
  tu_string	 _url;		// The URL to which the text in this text format hyperlinks.
                                // If url is an empty string, the text does not have a hyperlink
};
 
struct textformat_as_object : public gameswf::as_object
{
	text_format text_obj;
};

void
textformat_new(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

void
textformat_underline(gameswf::as_value* result, gameswf::as_object_interface* this_ptr, gameswf::as_environment* env, int nargs, int first_arg);

} // end of gameswf namespace

#endif	// __TEXTFORMAT_H__
