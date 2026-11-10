re4_tweaks - ترجمة الواجهة للعربية + بناء تلقائي على GitHub
============================================================

1) الترجمة
   - تمت ترجمة قائمة الإعدادات والمدرّب ومدير العناصر ونوافذ التحديث والتصحيح (حوالي 780 نصًا) داخل dllmain/.
   - ImGui لا يوصّل الحروف العربية ولا يعرف اليمين لليسار، لذلك أُضيفت الملفات:
       dllmain/ArabicText.h/.cpp  توصيل الحروف + ترتيب اليمين لليسار
       dllmain/ArabicUI.h/.cpp    يمرّر تلقائيًا كل ImGui::Text/Button/Checkbox... في الملفات المترجمة عبر المحرك
       dllmain/ArabicFont.hpp     خط IBM Plex Sans Arabic (رخصة OFL، انظر ArabicFont_LICENSE_OFL.txt)
   - النصوص في الكود عربية عادية بالترتيب الطبيعي، فتعديلها سهل.
   - لو أضفت ملفًا جديدًا فيه ImGui:: وتريده بالعربية: اجعل  #include "ArabicUI.h"  آخر include فيه.

2) البناء التلقائي (.github/workflows/build-release.yml)
   - ارفع المشروع إلى مستودع GitHub (الفرع main أو master).
   - مع كل push يبني الـ DLL (Release / Win32) ويرفع الناتج في Releases بوسم  build-arabic
     بملف  re4_tweaks_arabic.zip  يحتوي: dinput8.dll + dinput8.ini + مجلد re4_tweaks.
   - تقدر تشغّله يدويًا: Actions > Build Arabic Release > Run workflow.
   - رابط التحميل المباشر:
       https://github.com/<USER>/<REPO>/releases/download/build-arabic/re4_tweaks_arabic.zip
   - فك الـ ZIP داخل مجلد اللعبة (بجانب bio4.exe).
   - إن رفعت المشروع كملف ZIP بدون submodules، الـ workflow ينزّل المكتبات الناقصة من GitHub تلقائيًا (آخر نسخة).
     الأفضل لتثبيت نفس النسخ الأصلية: git clone --recursive للمستودع الأصلي ثم استبدل مجلد dllmain.
   - ci.yml الأصلي ما زال موجودًا ويبني أيضًا؛ احذفه إن لم تحتجه.
