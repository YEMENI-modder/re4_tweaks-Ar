#include "ArabicText.h"
#include <vector>
#include <algorithm>

namespace ArabicText
{
	typedef unsigned int u32;

	// ---------- UTF-8 ----------
	static void Decode(const std::string& s, std::vector<u32>& out)
	{
		out.clear();
		size_t i = 0, n = s.size();
		while (i < n)
		{
			unsigned char c = (unsigned char)s[i];
			u32 cp = 0xFFFD; int len = 1;
			if (c < 0x80) { cp = c; }
			else if ((c >> 5) == 0x6 && i + 1 < n) { cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F); len = 2; }
			else if ((c >> 4) == 0xE && i + 2 < n) { cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F); len = 3; }
			else if ((c >> 3) == 0x1E && i + 3 < n) { cp = ((c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F); len = 4; }
			out.push_back(cp);
			i += len;
		}
	}

	static void Encode(const std::vector<u32>& in, std::string& out)
	{
		out.clear();
		for (u32 cp : in)
		{
			if (cp < 0x80) out += (char)cp;
			else if (cp < 0x800) { out += (char)(0xC0 | (cp >> 6)); out += (char)(0x80 | (cp & 0x3F)); }
			else if (cp < 0x10000) { out += (char)(0xE0 | (cp >> 12)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
			else { out += (char)(0xF0 | (cp >> 18)); out += (char)(0x80 | ((cp >> 12) & 0x3F)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); out += (char)(0x80 | (cp & 0x3F)); }
		}
	}

	// ---------- character classes ----------
	static bool IsArabicRange(u32 c)
	{
		return (c >= 0x0600 && c <= 0x06FF) || (c >= 0x0750 && c <= 0x077F) || (c >= 0xFB50 && c <= 0xFDFF) || (c >= 0xFE70 && c <= 0xFEFF);
	}
	static bool IsRTLChar(u32 c)
	{
		if (c == 0x060C || c == 0x061B || c == 0x061F) return false; // Arabic punctuation: neutral
		if (c >= 0x0660 && c <= 0x0669) return false;                // Arabic-Indic digits: number
		return (c >= 0x0590 && c <= 0x05FF) || IsArabicRange(c);
	}

	bool HasRTL(const char* utf8)
	{
		if (!utf8) return false;
		std::vector<u32> cps;
		// fast path: pure ASCII never has RTL
		const unsigned char* p = (const unsigned char*)utf8;
		bool ascii = true;
		for (; *p; ++p) if (*p >= 0x80) { ascii = false; break; }
		if (ascii) return false;
		Decode(utf8, cps);
		for (u32 c : cps) if (IsRTLChar(c)) return true;
		return false;
	}

	// ---------- joining ----------
	struct Forms { u32 base, iso, fin, ini, med; }; // ini == 0 -> right-joining only (no joining to next letter)
	static const Forms kForms[] = {
		{0x0621,0xFE80,0,0,0},
		{0x0622,0xFE81,0xFE82,0,0}, {0x0623,0xFE83,0xFE84,0,0}, {0x0624,0xFE85,0xFE86,0,0}, {0x0625,0xFE87,0xFE88,0,0},
		{0x0626,0xFE89,0xFE8A,0xFE8B,0xFE8C}, {0x0627,0xFE8D,0xFE8E,0,0},
		{0x0628,0xFE8F,0xFE90,0xFE91,0xFE92}, {0x0629,0xFE93,0xFE94,0,0},
		{0x062A,0xFE95,0xFE96,0xFE97,0xFE98}, {0x062B,0xFE99,0xFE9A,0xFE9B,0xFE9C},
		{0x062C,0xFE9D,0xFE9E,0xFE9F,0xFEA0}, {0x062D,0xFEA1,0xFEA2,0xFEA3,0xFEA4}, {0x062E,0xFEA5,0xFEA6,0xFEA7,0xFEA8},
		{0x062F,0xFEA9,0xFEAA,0,0}, {0x0630,0xFEAB,0xFEAC,0,0}, {0x0631,0xFEAD,0xFEAE,0,0}, {0x0632,0xFEAF,0xFEB0,0,0},
		{0x0633,0xFEB1,0xFEB2,0xFEB3,0xFEB4}, {0x0634,0xFEB5,0xFEB6,0xFEB7,0xFEB8},
		{0x0635,0xFEB9,0xFEBA,0xFEBB,0xFEBC}, {0x0636,0xFEBD,0xFEBE,0xFEBF,0xFEC0},
		{0x0637,0xFEC1,0xFEC2,0xFEC3,0xFEC4}, {0x0638,0xFEC5,0xFEC6,0xFEC7,0xFEC8},
		{0x0639,0xFEC9,0xFECA,0xFECB,0xFECC}, {0x063A,0xFECD,0xFECE,0xFECF,0xFED0},
		{0x0640,0x0640,0x0640,0x0640,0x0640}, // tatweel
		{0x0641,0xFED1,0xFED2,0xFED3,0xFED4}, {0x0642,0xFED5,0xFED6,0xFED7,0xFED8},
		{0x0643,0xFED9,0xFEDA,0xFEDB,0xFEDC}, {0x0644,0xFEDD,0xFEDE,0xFEDF,0xFEE0},
		{0x0645,0xFEE1,0xFEE2,0xFEE3,0xFEE4}, {0x0646,0xFEE5,0xFEE6,0xFEE7,0xFEE8},
		{0x0647,0xFEE9,0xFEEA,0xFEEB,0xFEEC}, {0x0648,0xFEED,0xFEEE,0,0},
		{0x0649,0xFEEF,0xFEF0,0,0}, {0x064A,0xFEF1,0xFEF2,0xFEF3,0xFEF4},
	};

	static const Forms* FindForms(u32 c)
	{
		if (c < 0x0621 || c > 0x064A) return nullptr;
		for (const Forms& f : kForms) if (f.base == c) return &f;
		return nullptr;
	}
	static bool IsTransparent(u32 c) { return (c >= 0x064B && c <= 0x065F) || c == 0x0670 || (c >= 0x06D6 && c <= 0x06DC) || (c >= 0x06DF && c <= 0x06E4); }
	static bool JoinsNext(u32 c) { const Forms* f = FindForms(c); return f && f->ini != 0; }  // dual-joining
	static bool JoinsPrev(u32 c) { const Forms* f = FindForms(c); return f && f->fin != 0; }  // dual or right

	static void Join(const std::vector<u32>& in, std::vector<u32>& out)
	{
		out.clear();
		const int n = (int)in.size();
		for (int i = 0; i < n; i++)
		{
			u32 c = in[i];
			const Forms* f = FindForms(c);
			if (!f) { out.push_back(c); continue; }

			int p = i - 1; while (p >= 0 && IsTransparent(in[p])) p--;
			int q = i + 1; while (q < n && IsTransparent(in[q])) q++;
			bool prevJoins = p >= 0 && JoinsNext(in[p]);

			// Lam + Alef ligatures
			if (c == 0x0644 && i + 1 < n)
			{
				u32 a = in[i + 1], iso = 0, fin = 0;
				if (a == 0x0622) { iso = 0xFEF5; fin = 0xFEF6; }
				else if (a == 0x0623) { iso = 0xFEF7; fin = 0xFEF8; }
				else if (a == 0x0625) { iso = 0xFEF9; fin = 0xFEFA; }
				else if (a == 0x0627) { iso = 0xFEFB; fin = 0xFEFC; }
				if (iso) { out.push_back(prevJoins ? fin : iso); i++; continue; }
			}

			bool nextJoins = (f->ini != 0) && q < n && JoinsPrev(in[q]);
			u32 g;
			if (prevJoins && nextJoins) g = f->med;
			else if (prevJoins) g = f->fin;
			else if (nextJoins) g = f->ini;
			else g = f->iso;
			out.push_back(g);
		}
	}

	// ---------- bidi (simplified UBA, base direction RTL) ----------
	enum Cls : unsigned char { C_R, C_L, C_E, C_N };

	static Cls Classify(u32 c)
	{
		if (IsRTLChar(c)) return C_R;
		if ((c >= '0' && c <= '9') || (c >= 0x0660 && c <= 0x0669)) return C_E;
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return C_L;
		if (c < 0x80) return C_N;
		if (c <= 0xBF) return C_N;                          // Latin-1 punctuation / symbols
		if (c >= 0x2000 && c <= 0x2BFF) return C_N;         // punctuation, symbols, arrows
		if (c >= 0xE000 && c <= 0xF8FF) return C_N;         // private use (icon fonts)
		if (c == 0x060C || c == 0x061B || c == 0x061F) return C_N;
		return C_L;
	}

	static u32 Mirror(u32 c)
	{
		switch (c)
		{
		case '(': return ')'; case ')': return '(';
		case '[': return ']'; case ']': return '[';
		case '{': return '}'; case '}': return '{';
		case '<': return '>'; case '>': return '<';
		case 0xAB: return 0xBB; case 0xBB: return 0xAB;
		}
		return c;
	}

	static void Reorder(std::vector<u32>& cps)
	{
		const int n = (int)cps.size();
		std::vector<Cls> cls(n);
		for (int i = 0; i < n; i++) cls[i] = Classify(cps[i]);

		// numbers: group separators and signs that touch digits become part of the number
		for (int i = 0; i < n; i++)
		{
			u32 c = cps[i];
			if (cls[i] != C_N) continue;
			bool prevE = i > 0 && cls[i - 1] == C_E;
			bool nextE = i + 1 < n && cls[i + 1] == C_E;
			if ((c == '.' || c == ',' || c == ':' || c == '/' || c == '-') && prevE && nextE) cls[i] = C_E;
			else if ((c == '%' || c == '$' || c == 0xB0) && prevE) cls[i] = C_E;
			else if ((c == '+' || c == '-' || c == '$') && nextE && !prevE) cls[i] = C_E;
		}

		// resolve neutrals (N1/N2, base direction RTL, numbers influence like R)
		std::vector<unsigned char> level(n);
		for (int i = 0; i < n; i++)
		{
			if (cls[i] == C_R) level[i] = 1;
			else if (cls[i] == C_L || cls[i] == C_E) level[i] = 2;
			else level[i] = 0;
		}
		for (int i = 0; i < n;)
		{
			if (level[i] != 0) { i++; continue; }
			int j = i; while (j < n && level[j] == 0) j++;
			// direction influence of left / right neighbour: L -> 2, R or number -> 1, edge -> base (1)
			int left = 1, right = 1;
			if (i > 0) left = (cls[i - 1] == C_L) ? 2 : 1;
			if (j < n) right = (cls[j] == C_L) ? 2 : 1;
			unsigned char lv = (left == right) ? (unsigned char)left : 1;
			for (int k = i; k < j; k++) level[k] = lv;
			i = j;
		}

		// mirror brackets in RTL runs
		for (int i = 0; i < n; i++) if (level[i] == 1) cps[i] = Mirror(cps[i]);

		// L2: reverse runs from highest level down to level 1
		for (int lv = 2; lv >= 1; lv--)
		{
			for (int i = 0; i < n;)
			{
				if (level[i] < lv) { i++; continue; }
				int j = i; while (j < n && level[j] >= lv) j++;
				std::reverse(cps.begin() + i, cps.begin() + j);
				std::reverse(level.begin() + i, level.begin() + j);
				i = j;
			}
		}
	}

	std::string ShapeLine(const std::string& logical)
	{
		if (!HasRTL(logical.c_str())) return logical;
		std::vector<u32> in, joined;
		Decode(logical, in);
		// ImGui cannot position zero-advance marks over their base letter (no GPOS), so harakat/shadda
		// would land beside it. UI text is readable without them: drop them.
		in.erase(std::remove_if(in.begin(), in.end(), [](u32 c) { return IsTransparent(c); }), in.end());
		Join(in, joined);
		Reorder(joined);
		std::string out;
		Encode(joined, out);
		return out;
	}

	std::string Shape(const std::string& logical)
	{
		if (!HasRTL(logical.c_str())) return logical;
		std::string out;
		size_t start = 0;
		while (true)
		{
			size_t nl = logical.find('\n', start);
			std::string line = logical.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
			out += ShapeLine(line);
			if (nl == std::string::npos) break;
			out += '\n';
			start = nl + 1;
		}
		return out;
	}

	std::string ShapeLabel(const std::string& label)
	{
		size_t h = label.find("##");
		if (h == std::string::npos) return Shape(label);
		return Shape(label.substr(0, h)) + label.substr(h);
	}
}
