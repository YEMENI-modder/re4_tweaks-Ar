#pragma once
// Arabic text support for ImGui (which has no shaping / RTL support of its own).
// Strings in the source stay in normal *logical* order; at draw time they are converted to
// joined presentation forms (U+FE70..FEFF) in *visual* order, so the font only needs those glyphs.
#include <string>

namespace ArabicText
{
	// true if the UTF-8 string contains any right-to-left (Arabic/Hebrew) character
	bool HasRTL(const char* utf8);

	// Shape one line (no '\n' handling): joining + bidi reordering. Returns input unchanged if no Arabic.
	std::string ShapeLine(const std::string& logical);

	// Shape multi-line text ('\n' separated), each line independently.
	std::string Shape(const std::string& logical);

	// Same as Shape, but leaves an ImGui "##id" / "###id" suffix untouched (for widget labels).
	std::string ShapeLabel(const std::string& label);
}
