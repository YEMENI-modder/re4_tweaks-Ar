#include "imgui.h"
#include "imgui_internal.h"
#include "ArabicUI.h"
#include <cstdarg>
#include <cstdio>
#include <vector>
#include <string>

// NOTE: this file must NOT see the `#define ImGui ImGuiAr` macro (it needs the real namespace).
#ifdef ImGui
#undef ImGui
#endif

namespace ImGuiAr
{
	static std::string FormatV(const char* fmt, va_list args)
	{
		va_list a2;
		va_copy(a2, args);
		int n = vsnprintf(nullptr, 0, fmt, args);
		std::string s;
		if (n > 0)
		{
			s.resize((size_t)n);
			vsnprintf(&s[0], (size_t)n + 1, fmt, a2);
		}
		va_end(a2);
		return s;
	}

	const char* L(const char* label)
	{
		if (!label || !ArabicText::HasRTL(label))
			return label; // nothing to do (also the fast path for all English strings)

		static std::string ring[512];
		static unsigned idx = 0;
		std::string& slot = ring[idx++ % 512];
		slot = ArabicText::ShapeLabel(label);
		return slot.c_str();
	}

	void Text(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt);
		std::string s = FormatV(fmt, args);
		va_end(args);
		::ImGui::TextUnformatted(ArabicText::Shape(s).c_str());
	}

	void TextDisabled(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt);
		std::string s = FormatV(fmt, args);
		va_end(args);
		::ImGui::TextDisabled("%s", ArabicText::Shape(s).c_str());
	}

	void TextColored(const ImVec4& col, const char* fmt, ...)
	{
		va_list args; va_start(args, fmt);
		std::string s = FormatV(fmt, args);
		va_end(args);
		::ImGui::TextColored(col, "%s", ArabicText::Shape(s).c_str());
	}

	void BulletText(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt);
		std::string s = FormatV(fmt, args);
		va_end(args);
		::ImGui::BulletText("%s", ArabicText::Shape(s).c_str());
	}

	void SetTooltip(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt);
		std::string s = FormatV(fmt, args);
		va_end(args);
		::ImGui::SetTooltip("%s", ArabicText::Shape(s).c_str());
	}

	void TextUnformatted(const char* text, const char* text_end)
	{
		if (!text) return;
		std::string s = text_end ? std::string(text, text_end) : std::string(text);
		if (!ArabicText::HasRTL(s.c_str()))
		{
			::ImGui::TextUnformatted(text, text_end);
			return;
		}
		::ImGui::TextUnformatted(ArabicText::Shape(s).c_str());
	}

	ImVec2 CalcTextSize(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width)
	{
		if (!text) return ::ImGui::CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width);
		std::string s = text_end ? std::string(text, text_end) : std::string(text);
		if (!ArabicText::HasRTL(s.c_str()))
			return ::ImGui::CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width);
		std::string shaped = ArabicText::ShapeLabel(s);
		return ::ImGui::CalcTextSize(shaped.c_str(), NULL, hide_text_after_double_hash, wrap_width);
	}

	void TextWrapped(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt);
		std::string s = FormatV(fmt, args);
		va_end(args);

		if (!ArabicText::HasRTL(s.c_str()))
		{
			::ImGui::TextWrapped("%s", s.c_str()); // English: identical to the original behaviour
			return;
		}

		ImGuiWindow* window = ::ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;

		const ImVec2 pos = window->DC.CursorPos;
		// same wrap width ImGui::TextWrapped would use (wrap pos 0 = right edge of the work rect)
		const float wrapWidth = ImMax(window->WorkRect.Max.x - pos.x, 1.0f);

		ImFont* font = ::ImGui::GetFont();
		const float fontSize = ::ImGui::GetFontSize();
		const float lineH = fontSize;
		const float spaceW = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, " ").x;

		auto widthOf = [&](const std::string& logical) {
			std::string shaped = ArabicText::ShapeLine(logical);
			return font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, shaped.c_str()).x;
		};

		// 1) greedy word wrap on the LOGICAL text
		std::vector<std::string> lines;
		size_t pstart = 0;
		while (true)
		{
			size_t nl = s.find('\n', pstart);
			std::string para = s.substr(pstart, nl == std::string::npos ? std::string::npos : nl - pstart);

			std::string cur; float curW = 0.0f; bool have = false;
			size_t w0 = 0;
			while (w0 <= para.size())
			{
				size_t sp = para.find(' ', w0);
				std::string word = para.substr(w0, sp == std::string::npos ? std::string::npos : sp - w0);
				float ww = widthOf(word);
				if (!have) { cur = word; curW = ww; have = true; }
				else if (curW + spaceW + ww <= wrapWidth) { cur += ' '; cur += word; curW += spaceW + ww; }
				else { lines.push_back(cur); cur = word; curW = ww; }
				if (sp == std::string::npos) break;
				w0 = sp + 1;
			}
			lines.push_back(cur);

			if (nl == std::string::npos) break;
			pstart = nl + 1;
		}

		// 2) draw every line shaped and right-aligned, then register one layout item
		const ImU32 col = ::ImGui::GetColorU32(ImGuiCol_Text);
		float y = pos.y;
		for (const std::string& line : lines)
		{
			std::string shaped = ArabicText::ShapeLine(line);
			float w = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, shaped.c_str()).x;
			float x = pos.x + ImMax(0.0f, wrapWidth - w);
			if (!shaped.empty())
				window->DrawList->AddText(font, fontSize, ImVec2(x, y), col, shaped.c_str());
			y += lineH;
		}
		::ImGui::Dummy(ImVec2(wrapWidth, lineH * (float)lines.size()));
	}
}
