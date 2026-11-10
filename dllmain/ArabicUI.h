#pragma once
// ArabicUI.h - transparent Arabic support for ImGui text widgets.
//
// Include this header LAST in any .cpp that draws translated UI. After it, every `ImGui::Xxx(...)`
// call in that file resolves to `ImGuiAr::Xxx`: text-showing functions shape the Arabic text
// (letter joining + right-to-left order) and then forward to the real ImGui; every other function is
// forwarded untouched through `using namespace ::ImGui`. Strings with no Arabic pass through unchanged.
#include "imgui.h"
#include <string>
#include <utility>
#include "ArabicText.h"

namespace ImGuiAr
{
	using namespace ::ImGui;

	// Shaped copy of a widget label / text (keeps "##id" suffix). Pointer stays valid for the next ~500 calls.
	const char* L(const char* label);

	// Shaped std::string (for APIs outside ImGui::, e.g. ImGuiFileDialog titles)
	inline std::string Shape(const std::string& s) { return ArabicText::Shape(s); }

	// ---- widgets whose first argument is a visible label (also their ImGui ID) ----
	template<typename... A> bool Button(const char* l, A&&... a)        { return ::ImGui::Button(L(l), std::forward<A>(a)...); }
	template<typename... A> bool SmallButton(const char* l, A&&... a)   { return ::ImGui::SmallButton(L(l), std::forward<A>(a)...); }
	template<typename... A> bool Checkbox(const char* l, A&&... a)      { return ::ImGui::Checkbox(L(l), std::forward<A>(a)...); }
	template<typename... A> bool RadioButton(const char* l, A&&... a)   { return ::ImGui::RadioButton(L(l), std::forward<A>(a)...); }
	template<typename... A> bool Selectable(const char* l, A&&... a)    { return ::ImGui::Selectable(L(l), std::forward<A>(a)...); }
	template<typename... A> bool SliderInt(const char* l, A&&... a)     { return ::ImGui::SliderInt(L(l), std::forward<A>(a)...); }
	template<typename... A> bool SliderFloat(const char* l, A&&... a)   { return ::ImGui::SliderFloat(L(l), std::forward<A>(a)...); }
	template<typename... A> bool InputInt(const char* l, A&&... a)      { return ::ImGui::InputInt(L(l), std::forward<A>(a)...); }
	template<typename... A> bool InputFloat(const char* l, A&&... a)    { return ::ImGui::InputFloat(L(l), std::forward<A>(a)...); }
	template<typename... A> bool InputFloat3(const char* l, A&&... a)   { return ::ImGui::InputFloat3(L(l), std::forward<A>(a)...); }
	template<typename... A> bool InputText(const char* l, A&&... a)     { return ::ImGui::InputText(L(l), std::forward<A>(a)...); }
	template<typename... A> bool ColorEdit4(const char* l, A&&... a)    { return ::ImGui::ColorEdit4(L(l), std::forward<A>(a)...); }
	template<typename... A> bool BeginListBox(const char* l, A&&... a)  { return ::ImGui::BeginListBox(L(l), std::forward<A>(a)...); }
	template<typename... A> bool TreeNodeEx(const char* l, A&&... a)    { return ::ImGui::TreeNodeEx(L(l), std::forward<A>(a)...); }
	template<typename... A> void TableSetupColumn(const char* l, A&&... a) { ::ImGui::TableSetupColumn(L(l), std::forward<A>(a)...); }
	inline bool Begin(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) { return ::ImGui::Begin(L(name), p_open, flags); }
	template<typename... A> bool BeginPopup(const char* l, A&&... a)    { return ::ImGui::BeginPopup(L(l), std::forward<A>(a)...); }
	inline bool BeginPopupModal(const char* name, bool* p_open = NULL, ImGuiWindowFlags flags = 0) { return ::ImGui::BeginPopupModal(L(name), p_open, flags); }
	inline void OpenPopup(const char* str_id, ImGuiPopupFlags popup_flags = 0) { ::ImGui::OpenPopup(L(str_id), popup_flags); }
	inline void OpenPopup(ImGuiID id, ImGuiPopupFlags popup_flags = 0)         { ::ImGui::OpenPopup(id, popup_flags); }
	template<typename... A> bool BeginCombo(const char* l, const char* preview, A&&... a)
	{
		std::string lab = L(l); // copy: L() reuses a ring buffer
		return ::ImGui::BeginCombo(lab.c_str(), L(preview), std::forward<A>(a)...);
	}
	template<typename... A> bool InputTextWithHint(const char* l, const char* hint, A&&... a)
	{
		std::string lab = L(l);
		return ::ImGui::InputTextWithHint(lab.c_str(), L(hint), std::forward<A>(a)...);
	}

	// ---- plain text ----
	void Text(const char* fmt, ...) IM_FMTARGS(1);
	void TextDisabled(const char* fmt, ...) IM_FMTARGS(1);
	void TextColored(const ImVec4& col, const char* fmt, ...) IM_FMTARGS(2);
	void BulletText(const char* fmt, ...) IM_FMTARGS(1);
	void SetTooltip(const char* fmt, ...) IM_FMTARGS(1);
	void TextWrapped(const char* fmt, ...) IM_FMTARGS(1); // Arabic paragraphs: wrapped logically, right-aligned
	void TextUnformatted(const char* text, const char* text_end = NULL);
	ImVec2 CalcTextSize(const char* text, const char* text_end = NULL, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);
}

// From here on, `ImGui::Foo` in the including file means `ImGuiAr::Foo`.
#define ImGui ImGuiAr
