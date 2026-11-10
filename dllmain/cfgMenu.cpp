#include <iostream>
#include "dllmain.h"
#include "Patches.h"
#include "Settings.h"
#include "imgui.h"
#include "imgui\misc\cpp\imgui_stdlib.h"
#include "input.hpp"
#include "ConsoleWnd.h"
#include <FAhashes.h>
#include "Utils.h"
#include "UI_DebugWindows.h"
#include "UI_Utility.h"
#include "Trainer.h"
#include "AudioTweaks.h"
#include "HUDTweaks.h"
#include "ArabicUI.h" // must stay the LAST include: routes ImGui:: text calls through the Arabic shaper

bool bCfgMenuOpen = false;
bool NeedsToRestart = false;
bool bWaitingForHotkey = false;
bool bPauseGameWhileInCfgMenu = true;

int iGCBlurMode = 0;

float fLaserColorPicker[3]{ 0.0f, 0.0f, 0.0f };

ImVec2 PrevRightColumnPos;
ImVec2 PrevLeftColumnPos;

int MenuTipTimer = 500; // cfgMenu tooltip timer

std::string cfgMenuTitle = "re4_tweaks";

std::vector<uint32_t> cfgMenuCombo;

void PauseGame(bool setPaused)
{
	static bool stopFlagsBackedUp = false;
	static int stopFlagsBackup = -1;

	// Disable pausing when not needed or when pausing would casue issues
	bool disable =
		!PlayerPtr() ||
		SubScreenWk->open_flag_2C != 0 ||
		OptionOpenFlag() ||
		GlobalPtr()->playerHpCur_4FB4 == 0 ||
		GlobalPtr()->subHpCur_4FB8 == 0 ||
		EvtMgr->IsAliveEvt(EvtMgr->NowExeEvtName_1C, 0, AliveEvtType::AliveEvtTypeNormal);

	if (disable)
		return;

	if (setPaused && !stopFlagsBackedUp)
	{
		stopFlagsBackup = GlobalPtr()->flags_STOP_0_170[0];
		stopFlagsBackedUp = true;
		GlobalPtr()->flags_STOP_0_170[0] = 0xBFFFFF3F; // Same flags the game sets during its pause screen minus SPF_ID_SYSTEM 
	}
	else if (!setPaused && stopFlagsBackedUp)
	{
		GlobalPtr()->flags_STOP_0_170[0] = stopFlagsBackup;
		stopFlagsBackedUp = false;
	}
}

bool ParseConfigMenuKeyCombo(std::string_view in_combo)
{
	if (in_combo.empty())
		return false;

	cfgMenuCombo.clear();
	cfgMenuCombo = re4t::cfg->ParseKeyCombo(in_combo);

	pInput->register_hotkey({ []() {
		bCfgMenuOpen = !bCfgMenuOpen;

		// Pause game while cfgMenu is open
		PauseGame(bCfgMenuOpen && bPauseGameWhileInCfgMenu);
	}, &cfgMenuCombo });

	return cfgMenuCombo.size() > 0;
}

void cfgMenuRender()
{
	// Min/Max window sizes
	const float min_x = 940.0f * esHook._cur_monitor_dpi;
	const float min_y = 640.0f * esHook._cur_monitor_dpi;

	const float max_x = 1920.0f * esHook._cur_monitor_dpi;
	const float max_y = 1080.0f * esHook._cur_monitor_dpi;

	ImGui::SetNextWindowSizeConstraints(ImVec2(min_x, min_y), ImVec2(max_x, max_y));

	ImGui::Begin("cfgMenu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);
	{
		auto pos = ImGui::GetWindowPos();
		auto draw = ImGui::GetWindowDrawList();

		float leftside_size_x = 200; // Base
		leftside_size_x *= esHook._cur_monitor_dpi;

		float topright_size_y = 71; // Base
		topright_size_y *= re4t::cfg->fFontSizeScale;
		topright_size_y *= esHook._cur_monitor_dpi;

		// Left side BG
		draw->AddRectFilled(ImVec2(pos.x + 0, pos.y + 0), ImVec2(pos.x + leftside_size_x + 1, pos.y + ImGui::GetWindowHeight()), ImGui::GetColorU32(ImGuiCol_WindowBg), 10.f, 5);

		// Right side BG
		draw->AddRectFilled(ImVec2(pos.x + leftside_size_x, pos.y + 0), ImVec2(pos.x + ImGui::GetWindowWidth(), pos.y + ImGui::GetWindowHeight()), ImGui::GetColorU32(ImGuiCol_ChildBg), 8.f, 10);

		// Top right separator
		draw->AddLine(ImVec2(pos.x + leftside_size_x + 10, round(pos.y + topright_size_y)), ImVec2(pos.x + ImGui::GetItemRectSize().x - 15, round(pos.y + topright_size_y)), ImColor(150, 10, 40));

		// Setup tab buttons
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::BeginChild("left side", ImVec2(leftside_size_x, 0));
			ImGui::PopStyleColor();

			ImColor icn_color = ImColor(230, 15, 95);

			ImColor active = ImColor(150, 10, 40, 255);
			ImColor inactive = ImColor(31, 30, 31, 0);

			ImVec2 btn_size = ImVec2(leftside_size_x - 28, 31 * re4t::cfg->fFontSizeScale * esHook._cur_monitor_dpi);

			// ImGui font size
			ImGui::SetCursorPos(ImVec2(12, 0));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.51f, 0.00f, 0.14f, 0.00f));
			if (ImGui::Button("-"))
			{
				if (re4t::cfg->fFontSizeScale > 1.0f)
				{
					re4t::cfg->fFontSizeScale -= 0.05f;

					bRebuildFont = true;
					re4t::cfg->HasUnsavedChanges = true;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("+"))
			{
				if (re4t::cfg->fFontSizeScale < 1.20f)
				{
					re4t::cfg->fFontSizeScale += 0.05f;
					bRebuildFont = true;
					re4t::cfg->HasUnsavedChanges = true;
				}
			}

			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::Text("حجم الخط");

			// cfgMenu title
			ImGui::SetCursorPos(ImVec2(12, 45));

			ImGui::Text(cfgMenuTitle.data());

			ImGui::Dummy(ImVec2(30, 15 * esHook._cur_monitor_dpi));

			// Display
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##display", "العرض", active, inactive, MenuTab::Display, ICON_FA_DESKTOP, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Audio
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##audio", "الصوت", active, inactive, MenuTab::Audio, ICON_FA_HEADPHONES, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Mouse
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##mouse", "الماوس", active, inactive, MenuTab::Mouse, ICON_FA_LOCATION_ARROW, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Keyboard
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##keyboard", "لوحة المفاتيح", active, inactive, MenuTab::Keyboard, ICON_FA_KEYBOARD, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Controller
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##controller", "يد التحكم", active, inactive, MenuTab::Controller, ICON_FA_JOYSTICK, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Frame rate
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##framerate", "معدل الإطارات", active, inactive, MenuTab::Framerate, ICON_FA_CHESS_CLOCK, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Gameplay
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##gameplay", "طريقة اللعب", active, inactive, MenuTab::Gameplay, ICON_FA_BIOHAZARD, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();
			
			// Misc
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##misc", "متفرقات", active, inactive, MenuTab::Misc, ICON_FA_COG, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Memory
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##memory", "الذاكرة", active, inactive, MenuTab::Memory, ICON_FA_CALCULATOR, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			// Hotkeys
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			ImGui_TabButton("##hotkeys", "الاختصارات", active, inactive, MenuTab::Hotkeys, ICON_FA_LAMBDA, icn_color, IM_COL32_WHITE, btn_size);
			ImGui::Spacing();

			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			if (!re4t::cfg->bTrainerEnable)
			{
				if (ImGui_TabButton("##trainer", "المدرّب", active, inactive, MenuTab::Trainer, ICON_FA_SHOE_PRINTS, ImColor(255, 255, 255, 60), ImColor(255, 255, 255, 60), btn_size))
					ImGui::OpenPopup("قائمة المدرّب");

				// Always center this window when appearing
				ImVec2 center = ImGui::GetMainViewport()->GetCenter();
				ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

				if (ImGui::BeginPopupModal("قائمة المدرّب", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("تحذير: يحتوي هذا القسم على غش وتعديلات (patches) قد تفسد تجربة لعبك،\nأو تتسبب في أعطال داخل اللعبة.\n\nهل أنت متأكد أنك تريد استخدام قائمة المدرّب؟\n\n");
					ImGui::Separator();

					ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 265, ImGui::GetCursorPos().y));

					if (ImGui::Button("نعم", ImVec2(120, 0)))
					{
						re4t::cfg->bTrainerEnable = true;
						re4t::cfg->WriteSettings(true);
						ImGui::CloseCurrentPopup();
					}

					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();

					if (ImGui::Button("إلغاء", ImVec2(120, 0)))
					{
						Tab = MenuTab::Display;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

			}
			else
				ImGui_TabButton("##trainer", "المدرّب", active, inactive, MenuTab::Trainer, ICON_FA_SHOE_PRINTS, icn_color, IM_COL32_WHITE, btn_size);

			ImGui::Dummy(ImVec2(0.0f, 12.0f * esHook._cur_monitor_dpi));

			// Save/Load
			float sl_btn_size_x = (leftside_size_x / 2) - 18;
			float sl_btn_size_y = 35.0f;
			sl_btn_size_y *= re4t::cfg->fFontSizeScale;
			sl_btn_size_y *= esHook._cur_monitor_dpi;

			ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 10 - sl_btn_size_x, ImGui::GetContentRegionAvail().y - 10 - sl_btn_size_y));

			// Separator
			draw->AddLine(ImVec2(ImGui::GetCursorScreenPos().x + 10, ImGui::GetCursorScreenPos().y - 10), ImVec2(ImGui::GetCursorScreenPos().x + leftside_size_x - 20, ImGui::GetCursorScreenPos().y - 10), ImColor(150, 10, 40));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.51f, 0.00f, 0.14f, 0.00f));
			ImGui::Dummy(ImVec2(0, 13 * esHook._cur_monitor_dpi)); ImGui::SameLine();
			if (ImGui::Button(ICON_FA_ERASER" تجاهل", ImVec2(sl_btn_size_x, sl_btn_size_y)))
			{
				float oldSize = re4t::cfg->fFontSizeScale;

				re4t::cfg->ReadSettings();

				if (oldSize != re4t::cfg->fFontSizeScale)
					bRebuildFont = true;
			}

			ImGui::SameLine();

			if (ImGui::Button(ICON_FA_CODE" حفظ", ImVec2(sl_btn_size_x, sl_btn_size_y)))
			{
				// Parse key combos on save
				re4t::cfg->ParseHotkeys();

				// Update console title
				con.TitleKeyCombo = re4t::cfg->sConsoleKeyCombo;

				re4t::cfg->WriteSettings(false);
			}

			ImGui::PopStyleColor();

			ImGui::EndChild();
		}

		ImGui::SameLine();

		// Setup top right
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::BeginChild("top right", ImVec2(0, topright_size_y));
			ImGui::PopStyleColor();

			// "Pause game" option
			if (ImGui::Checkbox("إيقاف اللعبة مؤقتًا", &bPauseGameWhileInCfgMenu))
			{
				PauseGame(bCfgMenuOpen && bPauseGameWhileInCfgMenu);
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("إبقاء اللعبة متجمدة أثناء فتح قائمة الإعدادات");

			// Unsaved changes tip
			float tip_offset = 45.0f;
			tip_offset *= re4t::cfg->fFontSizeScale;
			tip_offset *= esHook._cur_monitor_dpi;

			if (re4t::cfg->HasUnsavedChanges)
			{
				ImGui::SameLine();

				const char* txt = "لديك تغييرات غير محفوظة!";

				ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(txt).x - tip_offset, ImGui::GetContentRegionAvail().y / 3.0f));
				ImGui::SameLine();
				ImGui::BulletText(txt);
			}

			// Search bar
			ImGui::PushID("#optionsfilter");
			static const std::string searchLabel = ICON_FA_SEARCH + std::string(" بحث");
			OptionsFilter.Draw2(searchLabel.c_str(), 150.0f * re4t::cfg->fFontSizeScale * esHook._cur_monitor_dpi);
			ImGui::PopID();
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("البحث عن الخيارات في هذا التبويب");

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.51f, 0.00f, 0.14f, 0.00f));
			if (ImGui::SmallButton(ICON_FA_BACKSPACE))
				OptionsFilter.Clear();
			ImGui::PopStyleColor();

			// Restart tip
			if (NeedsToRestart)
			{
				ImGui::SameLine();

				const char* txt = "التغييرات التي تم إجراؤها تتطلب إعادة تشغيل اللعبة!";

				ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(txt).x - tip_offset, ImGui::GetContentRegionAvail().y));
				ImGui::SameLine();
				ImGui::BulletText(txt);
			}

			ImGui::EndChild();
		}

		// Setup item view
		{
			ImGui::SetCursorPos(ImVec2(leftside_size_x + 16, topright_size_y + 10));

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::BeginChild("right side", ImVec2(0, 0));
			ImGui::PopStyleColor();

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(16.f, 16.f));

			int columnCount = 1 + (int)((ImGui::GetWindowWidth() / (716.0f * esHook._cur_monitor_dpi)) * 1.5f);

			if (Tab == MenuTab::Display)
			{
				if (ImGui::BeginTable("Display", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// FOVAdditional
					if ((OptionsFilter.PassFilter("FOV زاوية الرؤية مجال الرؤية") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("زاوية رؤية إضافية (FOVAdditional)", &re4t::cfg->bEnableFOV))
						{
							re4t::cfg->HasUnsavedChanges = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("قيمة FOV إضافية.");
						ImGui::TextWrapped("20 تبدو مناسبة لمعظم الحالات.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("منزلق FOV").x);
						ImGui::BeginDisabled(!re4t::cfg->bEnableFOV);
						ImGui::SliderFloat("منزلق FOV", &re4t::cfg->fFOVAdditional, 0.0f, 50.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();

						if (!re4t::cfg->bEnableFOV)
							re4t::cfg->fFOVAdditional = 0.0f;
					}

					// FOVAdditional
					if ((OptionsFilter.PassFilter("CameraSmooth سلاسة الكاميرا تنعيم") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("نسبة تنعيم الكاميرا");

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("يضبط نسبة السلاسة المطبقة على حركات الكاميرا، وقد يفضل البعض خفض هذه القيمة.");
						ImGui::TextWrapped("ملاحظة: ضبطها على 0 غير مُوصى به، لأنه يجعل حركات الكاميرا الصغيرة متقطعة جدًا (مثلًا عندما تتسلق الشخصية نتوءًا صغيرًا)");
						
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("التنعيم").x);
						if (ImGui::SliderFloat("التنعيم", &re4t::cfg->fCameraSmoothing, 0.0f, 100.0f, "%.0f", ImGuiSliderFlags_AlwaysClamp))
							Game_SetCameraSmoothness(re4t::cfg->fCameraSmoothing / 100.0f);

						ImGui::PopItemWidth();
					}

					// DisableVsync
					if ((OptionsFilter.PassFilter("DisableVsync تعطيل V-Sync المزامنة العمودية") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("تعطيل V-Sync (DisableVsync)", &re4t::cfg->bDisableVsync))
						{
							re4t::cfg->HasUnsavedChanges = true;

							// Trigger resolution change to make apply the new v-sync option
							bio4::D3D_SetupResolution(*bio4::g_D3D::Width_1, *bio4::g_D3D::Height_1);
							bio4::ScreenReSize(0x200, 0x1C0);
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("إجبار V-Sync على التعطيل. لسبب ما لا توفر اللعبة الأصلية طريقة فعالة لذلك.");
					}

					// Aspect ratio tweaks
					if ((OptionsFilter.PassFilter("Aspect ratio UltraWideAspectSupport SideAlignHUD Stretch Black Bars نسبة العرض إلى الارتفاع شاشة عريضة أشرطة سوداء") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::Spacing();
						ImGui::TextWrapped("تعديلات نسبة العرض إلى الارتفاع");

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("دعم الشاشات فائقة العرض (UltraWideAspectSupport)", &re4t::cfg->bUltraWideAspectSupport);
						ImGui::TextWrapped("يصلح نسبة العرض إلى الارتفاع الخاطئة عند اللعب بدقات الشاشات فائقة العرض، فيمنع قص الصورة وظهور HUD خارج الشاشة.");
						ImGui::TextWrapped("(غيّر الدقة حتى يسري مفعول هذا الإعداد)");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::BeginDisabled(!re4t::cfg->bUltraWideAspectSupport);
						if (ImGui::Checkbox("محاذاة HUD إلى الجانب (SideAlignHUD)", &re4t::cfg->bSideAlignHUD))
						{
							re4t::cfg->HasUnsavedChanges = true;
							re4t::HUDTweaks::ResetLifeMeter();
						}

						ImGui::TextWrapped("ينقل HUD إلى الجهة اليمنى من الشاشة.");

						ImGui::Spacing();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تمديد صور ملء الشاشة (StretchFullscreenImages)", &re4t::cfg->bStretchFullscreenImages);
						ImGui::TextWrapped("يمدد بعض الصور لتناسب الشاشة، مثل الصور التي تظهر عند قراءة \"الملفات\".");

						ImGui::Spacing();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تمديد مقاطع الفيديو (StretchVideos)", &re4t::cfg->bStretchVideos);
						ImGui::TextWrapped("يمدد مقاطع الفيديو المُصيَّرة مسبقًا لتناسب الشاشة.");
						ImGui::EndDisabled();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إزالة الأشرطة السوداء 16:10 (Remove16by10BlackBars)", &re4t::cfg->bRemove16by10BlackBars);
						ImGui::TextWrapped("يزيل الأشرطة السوداء العلوية والسفلية الموجودة عند اللعب بنسبة 16:10. سيقص بضع بيكسلات من كل جانب من الشاشة.");
						ImGui::TextWrapped("(غيّر الدقة حتى يسري مفعول هذا الإعداد)");
					}

					// FixDPIScale
					if ((OptionsFilter.PassFilter("FixDPIScale تكبير الشاشة DPI") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("إصلاح تكبير DPI (FixDPIScale)", &re4t::cfg->bFixDPIScale))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يجبر اللعبة على العمل بتكبير DPI عادي 100%%، ويصلح مشاكل الدقة للاعبين الذين يستخدمون تكبير DPI أعلى من 100%%.");
					}

					// FixDisplayMode
					if ((OptionsFilter.PassFilter("Display Mode وضع العرض معدل التحديث") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("إصلاح وضع العرض (FixDisplayMode)", &re4t::cfg->bFixDisplayMode))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يسمح للعبة باستخدام معدلات تحديث غير 60Hz في وضع ملء الشاشة، مما يصلح مشكلة الشاشة السوداء عند بدء اللعبة.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::BeginDisabled(!re4t::cfg->bFixDisplayMode);
						if (ImGui::Checkbox("عرض أعلى معدلات التحديث فقط (OnlyShowHighestRefreshRates)", &re4t::cfg->bOnlyShowHighestRefreshRates))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}
						ImGui::TextWrapped("عند استخدام FixDisplayMode، يتم عرض أعلى معدل تحديث متاح فقط لكل دقة في قائمة إعدادات اللعبة.");
						ImGui::EndDisabled();
					}

					// OverrideLaserColor
					if ((OptionsFilter.PassFilter("OverrideLaserColor RainbowLaser لون الليزر ليزر قوس قزح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تجاوز لون الليزر (OverrideLaserColor)", &re4t::cfg->bOverrideLaserColor);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يتجاوز لون الليزر الخاص بالأسلحة.");
						ImGui::TextWrapped("ملاحظة: يحتوي ملف config.ini الأصلي للعبة على إعداد لتغيير شفافية الليزر، لكنه لا يعمل، لذلك لم نضمّنه هنا.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::BeginDisabled(!re4t::cfg->bOverrideLaserColor || re4t::cfg->bRainbowLaser);
						ImGui::ColorEdit4("منتقي لون الليزر", fLaserColorPicker, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
						if (ImGui::IsItemEdited())
						{
							re4t::cfg->iLaserR = (int)(fLaserColorPicker[0] * 255.0f);
							re4t::cfg->iLaserG = (int)(fLaserColorPicker[1] * 255.0f);
							re4t::cfg->iLaserB = (int)(fLaserColorPicker[2] * 255.0f);
						}
						ImGui::EndDisabled();

						ImGui::Spacing();

						ImGui::BeginDisabled(!re4t::cfg->bOverrideLaserColor);
						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("ليزر قوس قزح (RainbowLaser)", &re4t::cfg->bRainbowLaser);
						ImGui::EndDisabled();

						if (!re4t::cfg->bOverrideLaserColor)
							re4t::cfg->bRainbowLaser = false;
					}

					// RestorePickupTransparency
					if ((OptionsFilter.PassFilter("RestorePickupTransparency شفافية الغرض الالتقاط") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("استعادة شفافية الالتقاط (RestorePickupTransparency)", &re4t::cfg->bRestorePickupTransparency);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يستعيد الشفافية في شاشة التقاط الأغراض.");
					}

					// DisableBrokenFilter03
					if ((OptionsFilter.PassFilter("DisableBrokenFilter03 فلتر معطوب لون برتقالي") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تعطيل الفلتر المعطوب 03 (DisableBrokenFilter03)", &re4t::cfg->bDisableBrokenFilter03);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("كان هذا الفلتر مخصصًا لإضافة تأثير توهج إضافي على بعض مصادر النار، لكنه تعطل عند نقل اللعبة إلى Xbox 360، فأصبحت الصورة كلها مغطاة بطبقة برتقالية.");
						ImGui::TextWrapped("(إذا كنت تستخدم HD Project، فيجب عليك تعطيل هذا الخيار)");
					}

					// FixBlurryImage
					if ((OptionsFilter.PassFilter("FixBlurryImage صورة ضبابية وضوح حدة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح الصورة الضبابية (FixBlurryImage)", &re4t::cfg->bFixBlurryImage);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يصلح مشكلة متعلقة بمخزن رؤوس (vertex buffer) تسببت في تشويش الصورة قليلًا، مما يجعلها أكثر حدة ووضوحًا.");
					}

					// DisableFilmGrain
					if ((OptionsFilter.PassFilter("DisableFilmGrain حبوب الفيلم تأثير الفيلم") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تعطيل حبوب الفيلم (DisableFilmGrain)", &re4t::cfg->bDisableFilmGrain);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يعطل طبقة حبوب الفيلم الموجودة في معظم أجزاء اللعبة.");
					}

					// FixWaterScaling
					if ((OptionsFilter.PassFilter("FixWaterScaling الماء تحسين الماء") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تحسين الماء (ImproveWater)", &re4t::cfg->bImproveWater);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10));
						ImGui::TextWrapped("يحسّن بعض مؤثرات الماء، ويصلح الماء \"الهلامي\" الموجود في بعض المناطق.");
						ImGui::TextWrapped("(إذا كنت تستخدم HD Project، فيجب عليك تعطيل هذا الخيار)");
					}

					// EnableGCBlur
					if ((OptionsFilter.PassFilter("EnableGCBlur ضبابية عمق المجال GC") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("تفعيل ضبابية GC (EnableGCBlur)", &re4t::cfg->bEnableGCBlur))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يستعيد ضبابية DoF من نسخة GC، والتي أُزيلت أو لم تُنفَّذ في النسخ اللاحقة.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						if (ImGui::RadioButton("محسّن", &iGCBlurMode, 0))
							re4t::cfg->bUseEnhancedGCBlur = true;
						ImGui::Indent(29.0);
						ImGui::TextWrapped("تنفيذ محسّن قليلًا ليبدو أفضل على شاشات الدقة العالية الحديثة.");
						ImGui::Unindent(29.0);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						if (ImGui::RadioButton("كلاسيكي", &iGCBlurMode, 1))
							re4t::cfg->bUseEnhancedGCBlur = false;
						ImGui::Indent(29.0);
						ImGui::TextWrapped("تنفيذ GC الأصلي.");
						ImGui::Unindent(29.0);

					}

					// EnableGCScopeBlur
					if ((OptionsFilter.PassFilter("EnableGCScopeBlur ضبابية المنظار GC") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("تفعيل ضبابية المنظار GC (EnableGCScopeBlur)", &re4t::cfg->bEnableGCScopeBlur))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يستعيد ضبابية المحيط الخارجي عند استخدام المنظار، والتي أُزيلت أو لم تُنفَّذ في النسخ اللاحقة.");
					}

					// WindowBorderless
					if ((OptionsFilter.PassFilter("WindowBorderless نافذة بلا إطار حدود") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("نافذة بلا إطار (WindowBorderless)", &re4t::cfg->bWindowBorderless))
						{
							re4t::cfg->HasUnsavedChanges = true;

							// Trigger resolution change to apply changes
							if (!*bio4::g_D3D::Fullscreen)
							{
								bio4::D3D_SetupResolution(*bio4::g_D3D::Width_1, *bio4::g_D3D::Height_1);
								bio4::ScreenReSize(0x200, 0x1C0);
							}
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("تحديد ما إذا كان سيتم استخدام نافذة بلا إطار عند اللعب في وضع النافذة.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						if (ImGui::Checkbox("السماح بتغيير حجم النافذة (EnableWindowResize)", &re4t::cfg->bEnableWindowResize))
						{
							re4t::cfg->HasUnsavedChanges = true;

							// Trigger resolution change to apply changes
							if (!*bio4::g_D3D::Fullscreen)
							{
								bio4::D3D_SetupResolution(*bio4::g_D3D::Width_1, *bio4::g_D3D::Height_1);
								bio4::ScreenReSize(0x200, 0x1C0);
							}
						}

						ImGui::TextWrapped("عند اللعب في وضع النافذة، يسمح بتغيير حجم نافذة اللعبة بسحب حدودها.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("موضع رسم نافذة اللعبة عند استخدام وضع النافذة.");
						ImGui::TextWrapped("القيمة -1 تستخدم الافتراضي للعبة (يضعها عادةً عند 0,0)");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::BeginDisabled(re4t::cfg->bRememberWindowPos);
						ImGui::PushItemWidth(150 * re4t::cfg->fFontSizeScale * esHook._cur_monitor_dpi);
						ImGui::InputInt("موضع X", &re4t::cfg->iWindowPositionX);
						ImGui::InputInt("موضع Y", &re4t::cfg->iWindowPositionY);
						ImGui::PopItemWidth();
						ImGui::EndDisabled();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						if (ImGui::Checkbox("تذكّر موضع النافذة (RememberWindowPos)", &re4t::cfg->bRememberWindowPos))
						{
							re4t::cfg->HasUnsavedChanges = true;

							if (re4t::cfg->bRememberWindowPos)
							{
								RECT rect;
								GetWindowRect(hWindow, &rect);

								re4t::cfg->iWindowPositionX = rect.left;
								re4t::cfg->iWindowPositionY = rect.top;
							}
						}
						ImGui::TextWrapped("يتذكر آخر موضع للنافذة. يحدّث هذا تلقائيًا قيمتي \"موضع X\" و\"موضع Y\" أعلاه.");
					}

					if ((OptionsFilter.PassFilter("RepositionHUD Offset إعادة تموضع HUD إزاحة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						// Reposition HUD
						if (ImGui::Checkbox("إعادة تموضع HUD (RepositionHUD)", &re4t::cfg->bRepositionHUD))
						{
							re4t::cfg->HasUnsavedChanges = true;
							if (re4t::cfg->bRepositionHUD)
								re4t::HUDTweaks::UpdateHUDOffsets();
							else if (re4t::cfg->fHUDOffsetX || re4t::cfg->fHUDOffsetY)
								re4t::HUDTweaks::ResetLifeMeter();
						}
						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("ينقل HUD عداد الصحة بالنسبة لموضعه الافتراضي.");
						ImGui::TextWrapped("(اضغط Ctrl مع الاستمرار لإجراء تعديلات أكبر)");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::BeginDisabled(!re4t::cfg->bRepositionHUD);
						ImGui::PushItemWidth(150 * re4t::cfg->fFontSizeScale * esHook._cur_monitor_dpi);
						if (ImGui::InputFloat("إزاحة X", &re4t::cfg->fHUDOffsetX, 0.1f, 1.0f))
						{
							re4t::cfg->HasUnsavedChanges = true;
							re4t::HUDTweaks::UpdateHUDOffsets();
						}
						if (ImGui::InputFloat("إزاحة Y", &re4t::cfg->fHUDOffsetY, 0.1f, 1.0f))
						{
							re4t::cfg->HasUnsavedChanges = true;
							re4t::HUDTweaks::UpdateHUDOffsets();
						}
						ImGui::PopItemWidth();
						ImGui::EndDisabled();
					}

					if ((OptionsFilter.PassFilter("SmallerHUD SmallerActionPrompts HUD أصغر مطالبات الإجراءات") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						//SmallerHUD 
						if (ImGui::Checkbox("HUD أصغر (SmallerHUD)", &re4t::cfg->bSmallerHUD))
						{
							re4t::cfg->HasUnsavedChanges = true;
							re4t::HUDTweaks::ResetLifeMeter();
						}
						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يجعل HUD عداد الصحة أصغر.");

						// SmallerActionPrompts
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("مطالبات إجراءات أصغر (SmallerActionPrompts)", &re4t::cfg->bSmallerActionPrompts);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يجعل أزرار التفاعل ومطالبات QTE أصغر.");
					}

					// HideZoomControlHints 
					if ((OptionsFilter.PassFilter("HideZoomControlHints Rifle Scope Binoculars إخفاء تلميحات التكبير منظار") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إخفاء تلميحات التحكم بالتكبير (HideZoomControlHints)", &re4t::cfg->bHideZoomControlHints);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يخفي تذكيرات التحكم بالتكبير من HUD منظار السلاح والمنظار المقرّب.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}
			
			if (Tab == MenuTab::Audio)
			{
				if (ImGui::BeginTable("Audio", 2, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// Audio volume
					if ((OptionsFilter.PassFilter("Audio volume مستوى الصوت") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::Spacing();
						ImGui::TextWrapped("ضبط مستوى صوت اللعبة");

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("سيؤثر صوت المشاهد السينمائية أيضًا على صوت حوار شاشة التاجر، لأنه يُعالج بالطريقة نفسها المستخدمة للمشاهد السينمائية.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("صوت المشاهد السينمائية").x);
						bool changed = ImGui::SliderInt("الصوت الرئيسي", &re4t::cfg->iVolumeMaster, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
						changed |= ImGui::SliderInt("صوت الموسيقى", &re4t::cfg->iVolumeBGM, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
						changed |= ImGui::SliderInt("صوت المؤثرات", &re4t::cfg->iVolumeSE, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
						changed |= ImGui::SliderInt("صوت المشاهد السينمائية", &re4t::cfg->iVolumeCutscene, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
						ImGui::PopItemWidth();

						if (changed)
						{
							re4t::cfg->HasUnsavedChanges = true;
							re4t::AudioTweaks::UpdateVolume();
						}
					}

					// GC sound effects
					if ((OptionsFilter.PassFilter("RestoreGCSoundEffects مؤثرات صوتية GC") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("استعادة المؤثرات الصوتية GC (RestoreGCSoundEffects)", &re4t::cfg->bRestoreGCSoundEffects);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يستعيد بعض المؤثرات الصوتية التي تغيّرت عن إصدار GC الأصلي.");
						ImGui::TextWrapped("(يغيّر حاليًا صوت السكين فقط)");
					}

					// SilenceArmoredAshley
					if ((OptionsFilter.PassFilter("SilenceArmoredAshley إسكات أشلي درع") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إسكات درع Ashley (SilenceArmoredAshley)", &re4t::cfg->bSilenceArmoredAshley);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يُسكت صوت زي Ashley المدرّع (\"Special 2\").");
						ImGui::TextWrapped("لمن يكرهون أيضًا صوت \"Clank Clank Clank\" المستمر.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}
						
			if (Tab == MenuTab::Mouse)
			{
				if (ImGui::BeginTable("Mouse", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// CameraImprovements
					if ((OptionsFilter.PassFilter("CameraImprovements تحسينات الكاميرا") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تحسينات الكاميرا (CameraImprovements)", &re4t::cfg->bCameraImprovements);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يوفر مجموعة صغيرة من تعديلات الكاميرا:");

						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("توجيه الأسلحة في الاتجاه الذي تشير إليه الكاميرا");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("منع إعادة ضبط الكاميرا عشوائيًا");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("السماح بحركة كاملة بزاوية 360 درجة");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("حركة أكثر سلاسة واستجابة");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::BeginDisabled(!re4t::cfg->bCameraImprovements);
						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إعادة توسيط الكاميرا بعد استخدام الأسلحة (ResetCameraAfterUsingWeapons)", &re4t::cfg->bResetCameraAfterUsingWeapons);
						ImGui::TextWrapped("توسيط الكاميرا بعد استخدام الأسلحة.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إعادة توسيط الكاميرا بعد استخدام السكين (ResetCameraAfterUsingKnife)", &re4t::cfg->bResetCameraAfterUsingKnife);
						ImGui::TextWrapped("توسيط الكاميرا بعد استخدام السكين.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إعادة توسيط الكاميرا عند الجري (ResetCameraWhenRunning)", &re4t::cfg->bResetCameraWhenRunning);
						ImGui::TextWrapped("توسيط الكاميرا عند الضغط على زر الجري.");
						ImGui::TextWrapped("يُستخدم فقط إذا لم يكن MouseTurning مفعّلًا.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("حساسية الكاميرا").x);
						ImGui::SliderFloat("حساسية الكاميرا", &re4t::cfg->fCameraSensitivity, 0.50f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();
					}

					// UseMouseTurning
					if ((OptionsFilter.PassFilter("UseMouseTurning الدوران بالماوس") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("استخدام الدوران بالماوس (UseMouseTurning)", &re4t::cfg->bUseMouseTurning);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يوفر طرقًا بديلة لتدوير الشخصية باستخدام الماوس.");
						ImGui::TextWrapped("يُوصى بوضع التصويب \"Modern\" في إعدادات اللعبة.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::RadioButton("النوع A", &re4t::cfg->iMouseTurnType, MouseTurnTypes::TypeA);
						ImGui::Indent(29.0);
						ImGui::TextWrapped("يتأثر دوران الشخصية بموضع الكاميرا، على غرار Resident Evil 6.");
						ImGui::TextWrapped("يُوصى باستخدام CameraImprovements.");
						ImGui::Unindent(29.0);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::RadioButton("النوع B", &re4t::cfg->iMouseTurnType, MouseTurnTypes::TypeB);
						ImGui::Indent(29.0);
						ImGui::TextWrapped("يتغير دوران الشخصية مباشرة بواسطة الماوس، على غرار Resident Evil 5.");
						ImGui::Unindent(29.0);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("حساسية النوع B").x - 60);
						ImGui::BeginDisabled(!(re4t::cfg->bUseMouseTurning && (re4t::cfg->iMouseTurnType == MouseTurnTypes::TypeB)));
						ImGui::Indent(29.0);
						ImGui::SliderFloat("حساسية النوع B", &re4t::cfg->fTurnTypeBSensitivity, 0.50f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();
						ImGui::Unindent(29.0);
					}

					// UseRawMouseInput
					if ((OptionsFilter.PassFilter("UseRawMouseInput الإدخال الخام للماوس") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("استخدام إدخال الماوس الخام (UseRawMouseInput)", &re4t::cfg->bUseRawMouseInput);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يجعل اللعبة تستخدم Raw Input للتصويب والدوران (إذا كان MouseTurning مفعّلًا).");
						ImGui::TextWrapped("يحسّن إدخال الماوس بشكل كبير بإزالة التسارع السالب/الموجب الذي كانت تطبقه اللعبة و Direct Input.");
						ImGui::TextWrapped("يفعّل هذا الخيار تلقائيًا وضع التصويب \"Modern\" في اللعبة، وهو غير متوافق مع الوضع \"Classic\".");
					}

					// DetachCameraFromAim
					if ((OptionsFilter.PassFilter("DetachCameraFromAim فصل الكاميرا عن التصويب") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("فصل الكاميرا عن التصويب (DetachCameraFromAim)", &re4t::cfg->bDetachCameraFromAim);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("عند استخدام إعداد الماوس \"Modern\"، تقفل اللعبة موضع الكاميرا على موضع التصويب، فيتحركان معًا.");
						ImGui::TextWrapped("رغم أن هذا هو السلوك المتوقع في معظم الألعاب، قد يفضّل بعض اللاعبين الإبقاء على سلوك الكاميرا الأصلي مع الاستفادة من مزايا التصويب \"Modern\".");
						ImGui::TextWrapped("تفعيل هذا الخيار سيستعيد أيضًا اهتزاز التصويب الأفقي الذي فُقد عندما طبّق المطورون التصويب \"Modern\".");
					}

					// FixSniperZoom
					if ((OptionsFilter.PassFilter("FixSniperZoom تكبير القناص إصلاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح تكبير القناص (FixSniperZoom)", &re4t::cfg->bFixSniperZoom);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يمنع إزاحة الكاميرا عشوائيًا بعد التكبير ببندقية القناص عند استخدام لوحة المفاتيح والماوس.");
					}

					// FixSniperFocus
					if ((OptionsFilter.PassFilter("FixSniperFocus تركيز القناص") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("إصلاح تركيز القناص (FixSniperFocus)", &re4t::cfg->bFixSniperFocus))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("عند التكبير والتصغير ببندقية القناص، يجعل هذا الخيار حركة \"التركيز\" تبدو مشابهة لما تبدو عليه عند استخدام يد التحكم.");
						ImGui::TextWrapped("يتطلب تفعيل EnableGCBlur.");
					}

					// FixRetryLoadMouseSelector
					if ((OptionsFilter.PassFilter("FixRetryLoadMouseSelector اختيار الماوس إعادة المحاولة تحميل") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح اختيار الماوس في إعادة المحاولة/التحميل (FixRetryLoadMouseSelector)", &re4t::cfg->bFixRetryLoadMouseSelector);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يمنع اللعبة من تجاوز اختيارك في شاشة \"Retry / Load\" عند تحريك الماوس قبل تأكيد الإجراء.");
						ImGui::TextWrapped("يتسبب هذا الخطأ عادةً في عودة اللاعبين إلى القائمة الرئيسية عن طريق الخطأ، بينما أرادوا فقط إعادة المحاولة من آخر نقطة حفظ.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Keyboard)
			{
				if (ImGui::BeginTable("Keyboard", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// FallbackToEnglishKeyIcons
					if ((OptionsFilter.PassFilter("FallbackToEnglishKeyIcons أيقونات المفاتيح الإنجليزية") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("الرجوع لأيقونات المفاتيح الإنجليزية (FallbackToEnglishKeyIcons)", &re4t::cfg->bFallbackToEnglishKeyIcons))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("ستجعل اللعبة بعض المفاتيح غير مرئية في لغات لوحة المفاتيح غير المدعومة، وتفعيل هذا الخيار يجعل اللعبة تستخدم المفاتيح الإنجليزية بدلاً منها (يتطلب إعادة تشغيل اللعبة)");
					}

					// AllowReloadWithoutAiming_kbm
					if ((OptionsFilter.PassFilter("AllowReloadWithoutAiming إعادة تحميل بدون تصويب") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("السماح بإعادة التحميل دون تصويب (AllowReloadWithoutAiming)", &re4t::cfg->bAllowReloadWithoutAiming_kbm))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("يلغي الحاجة إلى تصويب السلاح قبل إعادة تحميله.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إعادة التحميل دون تقريب (ReloadWithoutZoom)", &re4t::cfg->bReloadWithoutZoom_kbm);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("عدم التقريب عند إعادة التحميل دون تصويب.");
						ImGui::TextWrapped("لا يُوصى به عادةً، لأن التقريب والابتعاد يخفيان بعض عيوب الحركة عند انتهاء إعادة التحميل.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Controller)
			{
				if (ImGui::BeginTable("Controller", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// OverrideControllerSensitivity
					if ((OptionsFilter.PassFilter("OverrideControllerSensitivity حساسية يد التحكم تجاوز") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تجاوز حساسية يد التحكم (OverrideControllerSensitivity)", &re4t::cfg->bOverrideControllerSensitivity);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("تغيير حساسية يد التحكم. لسبب ما لا تحتوي اللعبة الأصلية على خيار لتغييرها ليد التحكم، بل للماوس فقط.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("شريط الحساسية").x);
						ImGui::BeginDisabled(!re4t::cfg->bOverrideControllerSensitivity);
						ImGui::SliderFloat("Sensitivity Slider", &re4t::cfg->fControllerSensitivity, 0.50f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();

						if (!re4t::cfg->bOverrideControllerSensitivity)
							re4t::cfg->fControllerSensitivity = 1.0f;
					}

					// RemoveExtraXinputDeadzone
					if ((OptionsFilter.PassFilter("RemoveExtraXinputDeadzone المنطقة الميتة يد التحكم إزالة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إزالة المنطقة الميتة الإضافية لـ XInput (RemoveExtraXinputDeadzone)", &re4t::cfg->bRemoveExtraXinputDeadzone);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يزيل المناطق الميتة غير الضرورية التي أُضيفت ليدات تحكم XInput.");
					}

					// EnableDeadzoneOverride
					if ((OptionsFilter.PassFilter("EnableDeadzoneOverride المنطقة الميتة تجاوز يد التحكم") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تفعيل تجاوز المنطقة الميتة (EnableDeadzoneOverride)", &re4t::cfg->bOverrideXinputDeadzone);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("تغيير المنطقة الميتة ليد تحكم XInput.");
						ImGui::TextWrapped("(لا علاقة له بالخيار السابق)");
						ImGui::TextWrapped("القيمة الافتراضية في اللعبة هي 1، لكنها تبدو كبيرة بلا داعٍ، لذلك نستخدم 0.4 افتراضياً.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("شريط المنطقة الميتة").x);
						ImGui::BeginDisabled(!re4t::cfg->bOverrideXinputDeadzone);
						ImGui::SliderFloat("Deadzone Slider", &re4t::cfg->fXinputDeadzone, 0.0f, 3.5f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
						ImGui::EndDisabled();
						ImGui::PopItemWidth();

						if (ImGui::IsItemEdited())
							re4t::cfg->HasUnsavedChanges = true;

						if (!re4t::cfg->bOverrideXinputDeadzone)
							re4t::cfg->fXinputDeadzone = 1.0f;
					}

					// SmoothAnalogTurning
					if ((OptionsFilter.PassFilter("SmoothAnalogTurning دوران سلس عصا تناظرية") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("دوران تناظري سلس (SmoothAnalogTurning)", &re4t::cfg->bSmoothAnalogTurning);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("استخدام الإدخال الاتجاهي التناظري للدوران عند التحرك للأمام والخلف، مشابهاً لنمط التحكم A/B في Resident Evil 5.");
					}

					// AllowReloadWithoutAiming_controller
					if ((OptionsFilter.PassFilter("AllowReloadWithoutAiming إعادة تحميل بدون تصويب") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("السماح بإعادة التحميل دون تصويب (AllowReloadWithoutAiming)", &re4t::cfg->bAllowReloadWithoutAiming_controller);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يلغي الحاجة إلى تصويب السلاح قبل إعادة تحميله.");
						ImGui::TextWrapped("تحذير: سيغيّر هذا أيضاً زر إعادة التحميل بحسب نوع إعداد يد التحكم الحالي، لأن الزر الأصلي يُستخدم للركض عند عدم التصويب.");
						
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("زر إعادة التحميل الجديد (XInput):");
						ImGui::BulletText("نوع الإعداد I: B");
						ImGui::BulletText("نوع الإعداد II و III: X");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui_ItemSeparator2();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إعادة التحميل دون تقريب (ReloadWithoutZoom)", &re4t::cfg->bReloadWithoutZoom_controller);

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("عدم التقريب عند إعادة التحميل دون تصويب.");
						ImGui::TextWrapped("لا يُوصى به عادةً، لأن التقريب والابتعاد يخفيان بعض عيوب الحركة عند انتهاء إعادة التحميل.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Framerate)
			{
				if (ImGui::BeginTable("Framerate", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// MotionFixes
					if ((OptionsFilter.PassFilter("MotionFixes إصلاح الحركة الرسوم المتحركة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاحات الحركة (MotionFixes)", &re4t::cfg->bMotionFixes);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يصلح استيفاء الحركة عند 60fps (وما فوق؟)");
						ImGui::TextWrapped("عند 60fps تُعرض بعض الحركات بسرعة 30fps كحل بديل بسبب مشكلة في كود استيفاء الحركة الأصلي.");
						ImGui::TextWrapped("يحاول هذا إعادة تنفيذ جزء من ذلك الكود لعرض تلك الحركات بشكل صحيح عند معدلات إطارات أعلى.");
						ImGui::TextWrapped("أبرز الحركات المتأثرة:");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("إعادة تحميل وإطلاق بندقية اللاعب (تلقيم الترباس)، لكلتا البندقيتين؛");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("حركة \"الاستعداد\" لقوس أدا؛");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("طفيليات Plaga التي تلتهم الرأس.");
					}

					// FixFallingItemsSpeed
					if ((OptionsFilter.PassFilter("FixFallingItemsSpeed سرعة سقوط الأغراض إصلاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح سرعة سقوط الأغراض (FixFallingItemsSpeed)", &re4t::cfg->bFixFallingItemsSpeed);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يصلح سرعة سقوط الأغراض عند استخدام معدل إطارات غير 30FPS، فلا تسقط بسرعة مضاعفة.");
					}

					// FixCompartmentsOpeningSpeed
					if ((OptionsFilter.PassFilter("FixCompartmentsOpeningSpeed سرعة فتح الأدراج والخزائن إصلاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح سرعة فتح الحجرات (FixCompartmentsOpeningSpeed)", &re4t::cfg->bFixCompartmentsOpeningSpeed);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يصلح سرعة فتح الحجرات مثل الأدراج والخزائن والصناديق والرفوف وغيرها.");
					}

					// FixMovingGeometrySpeed
					if ((OptionsFilter.PassFilter("FixMovingGeometrySpeed سرعة الأبواب والبوابات إصلاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح سرعة الأجسام المتحركة (FixMovingGeometrySpeed)", &re4t::cfg->bFixMovingGeometrySpeed);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يصلح سرعة الأجسام المتحركة مثل: الأبواب والممرات والبوابات والأسقف وغيرها.");
					}

					// FixTurningSpeed
					if ((OptionsFilter.PassFilter("FixTurningSpeed سرعة الدوران إصلاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح سرعة الدوران (FixTurningSpeed)", &re4t::cfg->bFixTurningSpeed);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يصلح سرعة الدوران للخلف عند استخدام معدل إطارات غير 30FPS.");
					}

					// FixQTE
					if ((OptionsFilter.PassFilter("FixQTE إصلاح QTE ضغط الأزرار") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح QTE (FixQTE)", &re4t::cfg->bFixQTE);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("عند التشغيل بسرعة 60 FPS تتطلب بعض أحداث QTE ضغطاً سريعاً جداً على الأزرار. ويزداد الأمر سوءاً في صعوبة Professional، حتى يبدو النجاة من QTE عربة المنجم وجسر التمثال شبه مستحيلة.");
						ImGui::TextWrapped("يجعل هذا الإصلاح أحداث QTE التي تتطلب ضغطاً سريعاً على الأزرار أكثر تساهلاً بكثير.");
					}

					// FixAshleyBustPhysics
					if ((OptionsFilter.PassFilter("FixAshleyBustPhysics فيزياء أشلي إصلاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح فيزياء صدر Ashley (FixAshleyBustPhysics)", &re4t::cfg->bFixAshleyBustPhysics);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يصلح الاختلاف بين 30/60FPS في الفيزياء المطبقة على Ashley.");
					}

					// EnableFastMath
					if ((OptionsFilter.PassFilter("EnableFastMath رياضيات سريعة تحسين الأداء") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("تفعيل الرياضيات السريعة (EnableFastMath)", &re4t::cfg->bEnableFastMath))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يستبدل دوال الرياضيات القديمة في اللعبة ببدائل أكثر كفاءة بكثير.");
						ImGui::TextWrapped("تجريبي، ويؤمل أن يحسّن معدل الإطارات في بعض المناطق التي كانت تشهد انخفاضاً.");
					}

					// ReplaceFramelimiter
					if ((OptionsFilter.PassFilter("ReplaceFramelimiter محدد الإطارات استبدال") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("استبدال محدد الإطارات (ReplaceFramelimiter)", &re4t::cfg->bReplaceFramelimiter))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يستبدل محدد الإطارات 60/30FPS في اللعبة بنسختنا الخاصة، مما يقلل استخدام المعالج كثيراً.");
						ImGui::TextWrapped("(تجريبي، ولا يُعرف بعد إن كان المحدد الجديد يعمل مثل القديم)");
					}

					// MultithreadFix
					if ((OptionsFilter.PassFilter("MultithreadFix تعدد المسارات إصلاح الأداء") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("إصلاح تعدد المسارات (MultithreadFix)", &re4t::cfg->bMultithreadFix))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يفعّل إصلاحات DirectX9 متعدد المسارات، مما يمنح تحسناً طفيفاً في الأداء");
						ImGui::TextWrapped("تجريبي، وإذا واجهت توقف اللعبة أو تجمدها فجرّب تعطيل هذا الإعداد ثم العب مجدداً.");
					}

					// PrecacheModels
					if ((OptionsFilter.PassFilter("PrecacheModels تحميل مسبق للنماذج") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تحميل النماذج مسبقاً (PrecacheModels)", &re4t::cfg->bPrecacheModels);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يجبر اللعبة على تخزين جميع النماذج في المرحلة مؤقتاً بالكامل بعد التحميل.");
						ImGui::TextWrapped("قد يساعد في تقليل انخفاض معدل الإطارات عند رؤية نموذج لأول مرة.");
						ImGui::TextWrapped("(لم يُختبر بالكامل، وقد يسبب مشاكل إذا كانت المرحلة تحتوي على نماذج كثيرة!)");
						ImGui::TextWrapped("تُطبَّق التغييرات عند تحميل المرحلة التالية.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Gameplay)
			{
				if (ImGui::BeginTable("Gameplay", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// AshleyJPCameraAngles
					if ((OptionsFilter.PassFilter("AshleyJPCameraAngles زوايا الكاميرا أشلي اليابانية") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("زوايا كاميرا Ashley اليابانية (AshleyJPCameraAngles)", &re4t::cfg->bAshleyJPCameraAngles);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يفتح زوايا الكاميرا الكلاسيكية الخاصة بالنسخة اليابانية أثناء مقطع Ashley.");
					}

					// SeparateWaysProfessional
					if ((OptionsFilter.PassFilter("SeparateWaysProfessional طرق منفصلة الصعوبة المحترف") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("طرق منفصلة بصعوبة Professional (SeparateWaysProfessional)", &re4t::cfg->bSeparateWaysProfessional);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يضيف قائمة للاختيار بين صعوبة Normal وProfessional عند بدء لعبة جديدة من Separate Ways.");
					}

					// EnableNTSCMode
					if ((OptionsFilter.PassFilter("EnableNTSCMode وضع NTSC الصعوبة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("تفعيل وضع NTSC (EnableNTSCMode)", &re4t::cfg->bEnableNTSCMode))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يفتح تعديلات صعوبة بسيطة كانت حصرية لنسخ الأجهزة المنزلية الأمريكية من RE4.");
						ImGui::TextWrapped("صعوبة تكيفية ابتدائية أعلى في وضع Normal وSeparate Ways، وصعوبة ثابتة أعلى في Assignment Ada، وصعوبة ديناميكية مفتوحة في مرحلة القرية في Mercenaries.");
						ImGui::TextWrapped("تتطلب أغطية الزجاجات 3000 نقطة في ميدان الرماية، وتُزال صعوبة Easy من القائمة الرئيسية.");
					}

					// AllowAshleySuplex
					if ((OptionsFilter.PassFilter("AllowAshleySuplex أشلي رمي الأعداء") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("السماح بـ Suplex لـ Ashley (AllowAshleySuplex)", &re4t::cfg->bAllowAshleySuplex);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يسمح لـ Ashley بتنفيذ Suplex على الأعداء في مواقف محددة جداً.");
						ImGui::TextWrapped("(كان ممكناً سابقاً في نسخة NTSC GameCube الأولى فقط، وأُزيل في جميع النسخ اللاحقة.)");
					}

					// AllowSellingHandgunSilencer
					if ((OptionsFilter.PassFilter("AllowSellingHandgunSilencer كاتم الصوت المسدس بيع") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("السماح ببيع كاتم صوت المسدس (AllowSellingHandgunSilencer)", &re4t::cfg->bAllowSellingHandgunSilencer))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يسمح ببيع كاتم صوت المسدس (غير المستخدم عادةً) إلى التاجر.");
					}

					// BalancedChicagoTypewriter
					if ((OptionsFilter.PassFilter("BalancedChicagoTypewriter شيكاغو تايبرايتر موازنة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("موازنة Chicago Typewriter (BalancedChicagoTypewriter)", &re4t::cfg->bBalancedChicagoTypewriter))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يغيّر طريقة عمل Chicago Typewriter الخاصة بـ Leon:");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("يمكن الآن أن تسقط ذخيرة Chicago Typewriter من الأعداء القتلى إذا كان السلاح في حقيبة اللاعب.");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("لم تعد ذخيرتها غير محدودة افتراضياً. أصبحت الذخيرة غير المحدودة جزءاً من ترقيتها الحصرية.");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("قوة النيران منخفضة كثيراً. وعند الترقية الكاملة تصل إلى قوتها الأصلية (10).");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("أصبحت ترقياتها متاحة لدى التاجر، ويفترض أن تكون جميع المستويات مفتوحة افتراضياً.");
					}

					// FixDitmanGlitch
					if ((OptionsFilter.PassFilter("FixDitmanGlitch ثغرة ديتمان إصلاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح ثغرة Ditman (FixDitmanGlitch)", &re4t::cfg->bFixDitmanGlitch);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يصلح ثغرة Ditman التي كانت تتيح للاعبين زيادة سرعة المشي.");
					}

					// UseSprintToggle
					if ((OptionsFilter.PassFilter("UseSprintToggle الركض تبديل") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("استخدام تبديل الركض (UseSprintToggle)", &re4t::cfg->bUseSprintToggle);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يجعل مفتاح الركض يعمل كمفتاح تبديل بدلاً من الحاجة إلى الاستمرار بالضغط عليه.");
					}

					// DisableAutomaticReload
					if ((OptionsFilter.PassFilter("DisableAutomaticReload إعادة التحميل التلقائي تعطيل") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تعطيل إعادة التحميل التلقائي (DisableAutomaticReload)", &re4t::cfg->bDisableAutomaticReload);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يعطّل إعادة التحميل التلقائي عند إطلاق سلاح نفدت ذخيرته.");
					}

					// RifleScreenShake
					if ((OptionsFilter.PassFilter("RifleScreenShake اهتزاز الشاشة البندقية") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("اهتزاز الشاشة للبندقية (RifleScreenShake)", &re4t::cfg->bRifleScreenShake))
						{
							re4t::cfg->HasUnsavedChanges = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("يضيف تأثير اهتزاز للشاشة عند إطلاق البندقية.");
					}

					// QTE options
					if ((OptionsFilter.PassFilter("QTE disable automatic تعطيل QTE تلقائي") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						// DisableQTE
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تعطيل QTE (DisableQTE)", &re4t::cfg->bDisableQTE);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يعطّل معظم أحداث QTE ويجعلها تنجح تلقائياً.");

						// AutomaticMashingQTE
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("QTE الضغط المتكرر التلقائي (AutomaticMashingQTE)", &re4t::cfg->bAutomaticMashingQTE);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("على عكس الخيار السابق، يؤتمت هذا الخيار أحداث QTE الخاصة بـ \"الضغط المتكرر\" فقط ويجعلها تنجح تلقائياً. وتبقى الإشارات ظاهرة!");
					}

					// Matilda options
					if ((OptionsFilter.PassFilter("AllowMatildaQuickturn LimitMatildaBurst ماتيلدا دوران سريع رشقة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						// AllowMatildaQuickturn
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("السماح بالدوران السريع مع Matilda (AllowMatildaQuickturn)", &re4t::cfg->bAllowMatildaQuickturn);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يسمح بالدوران السريع للشخصية نحو اتجاه الكاميرا عند حمل Matilda.");
						ImGui::TextWrapped("(فعّال فقط إذا كان MouseTurning معطلاً)");

						// LimitMatildaBurst
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تحديد رشقة Matilda (LimitMatildaBurst)", &re4t::cfg->bLimitMatildaBurst);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يحدد Matilda برشقة واحدة من ثلاث طلقات لكل ضغطة على الزناد.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Misc)
			{
				if (ImGui::BeginTable("Misc", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// RestoreDemoVideos
					if ((OptionsFilter.PassFilter("RestoreDemoVideos فيديوهات العرض استعادة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("استعادة فيديوهات العرض (RestoreDemoVideos)", &re4t::cfg->bRestoreDemoVideos);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يعيد تشغيل فيديوهات العرض التي تُعرض عندما تبقى اللعبة خاملة في القائمة الرئيسية أو شاشة \"اضغط أي مفتاح\" لنحو 20 ثانية.");
						ImGui::TextWrapped("افتراضياً تتضمن نسخة الحاسوب فيديو عرض واحداً فقط (demo0.sfd/demo0eng.sfd).");
						ImGui::TextWrapped("لكن re4_tweaks يدعم أيضاً تشغيل فيديوهات GameCube الأصلية إذا وُجدت في المجلد BIO4/movie.");
						ImGui::TextWrapped("فيديوهات العرض المدعومة:");
						ImGui::BulletText("demo0_gc.sfd");
						ImGui::BulletText("demo1_gc.sfd");
						ImGui::Bullet(); ImGui::SameLine(); ImGui::TextWrapped("demo0.sfd/demo0eng.sfd (مضمّن في اللعبة الأصلية، وموجود أيضاً في نسخة Wii باسم \"demo2.sfd\")");
					}

					// RestoreAnalogTitleScroll
					if ((OptionsFilter.PassFilter("RestoreAnalogTitleScroll تمرير خلفية القائمة الرئيسية استعادة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("استعادة تمرير القائمة الرئيسية بالعصا (RestoreAnalogTitleScroll)", &re4t::cfg->bRestoreAnalogTitleScroll);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يعيد القدرة على التحكم بتمرير خلفية القائمة الرئيسية بعد إنهاء اللعبة باستخدام العصا التناظرية اليمنى أو حركة الماوس.");
						ImGui::TextWrapped("للوحة المفاتيح والماوس: حرّك الماوس مع الضغط على CTRL.");
					}

					// ViolenceLevelOverride
					if ((OptionsFilter.PassFilter("ViolenceLevelOverride مستوى العنف تجاوز") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::Text("تجاوز مستوى العنف (ViolenceLevelOverride)");

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يسمح بتجاوز مستوى العنف في اللعبة.");
						ImGui::TextWrapped("استخدم -1 لترك الإعداد الافتراضي للعبة، أو 0 لوضع العنف المنخفض (كما في النسخة اليابانية/الألمانية)، أو 2 للعنف الكامل.");
						
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::PushItemWidth(100 * re4t::cfg->fFontSizeScale * esHook._cur_monitor_dpi);
						if (ImGui::InputInt("تجاوز مستوى العنف", &re4t::cfg->iViolenceLevelOverride))
						{
							if (re4t::cfg->iViolenceLevelOverride < -1)
								re4t::cfg->iViolenceLevelOverride = -1;

							if (re4t::cfg->iViolenceLevelOverride > 2)
								re4t::cfg->iViolenceLevelOverride = 2;

							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true; // unfortunately required as game only reads pSys from savegame during first loading screen...
						}
						ImGui::PopItemWidth();
					}

					// AllowMafiaLeonCutscenes
					if ((OptionsFilter.PassFilter("AllowMafiaLeonCutscenes زي المافيا ليون المشاهد") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("السماح بمشاهد Leon بزي المافيا (AllowMafiaLeonCutscenes)", &re4t::cfg->bAllowMafiaLeonCutscenes))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يسمح للعبة بعرض زي المافيا الخاص بـ Leon (\"Special 2\") في المشاهد السينمائية.");
					}

					// SkipIntroLogos
					if ((OptionsFilter.PassFilter("SkipIntroLogos SkipMenuFades تخطي الشعارات المقدمة التلاشي") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تخطي شعارات البداية (SkipIntroLogos)", &re4t::cfg->bSkipIntroLogos);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("تحديد ما إذا كان سيتم تخطي شعارات Capcom وغيرها عند بدء اللعبة.");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::BeginDisabled(!re4t::cfg->bSkipIntroLogos);
						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("تخطي تلاشي القوائم (SkipMenuFades)", &re4t::cfg->bSkipMenuFades);
						ImGui::EndDisabled();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يقلل مدة تلاشي القوائم/الرسائل (مثل 'تم الحفظ/التحميل بنجاح!') ويتخطى شعار \"Resident Evil 4\" الأولي / شاشة \"PRESS ANY KEY\".");
						ImGui::TextWrapped("(لن يعمل إلا إذا كان SkipIntroLogos مفعّلاً أيضاً)");
					}

					// SpeedUpQuitGame
					if ((OptionsFilter.PassFilter("SpeedUpQuitGame تسريع الخروج من اللعبة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("تسريع الخروج من اللعبة (SpeedUpQuitGame)", &re4t::cfg->bSpeedUpQuitGame))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يسرّع تلاشي الشاشة البطيء بلا داعٍ الذي يحدث بعد الضغط على \"Quit\" في القائمة الرئيسية للعبة.");
					}

					// AlwaysShowOriginalTitleBackground
					if ((OptionsFilter.PassFilter("AlwaysShowOriginalTitleBackground خلفية القائمة الرئيسية الأصلية") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("عرض خلفية القائمة الأصلية دائماً (AlwaysShowOriginalTitleBackground)", &re4t::cfg->bAlwaysShowOriginalTitleBackground);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("بعد إنهاء اللعبة، يجبر اللعبة على عرض صورة خلفية القائمة الرئيسية الأصلية لـ Leon وAshley دائماً.");
					}

					// FixSilencedHandgunDescription
					if ((OptionsFilter.PassFilter("FixSilencedHandgunDescription Silencer وصف المسدس كاتم الصوت إصلاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("إصلاح وصف المسدس الكاتم (FixSilencedHandgunDescription)", &re4t::cfg->bFixSilencedHandgunDescription);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يحدّث وصف المسدس الكاتم ليذكر زيادة فرصة الضربة الحرجة فقط عند الترقية الكاملة للمسدس.");
					}

					// EnableDebugMenu
					if ((OptionsFilter.PassFilter("EnableDebugMenu قائمة التصحيح تفعيل") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("تفعيل قائمة التصحيح (EnableDebugMenu)", &re4t::cfg->bEnableDebugMenu))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يفعّل قائمة التصحيح \"tool menu\" الموجودة داخل اللعبة وغير المستخدمة، ويضيف بعض الإدخالات المخصصة (\"SAVE GAME\"، \"DOF / BLUR MENU\"، إلخ).");
						ImGui::TextWrapped("يمكن فتحها بالضغط على LT+LS (أو CTRL+F3 على لوحة المفاتيح افتراضياً).");
						ImGui::TextWrapped("عند تفعيله على نسخة التصحيح 1.0.6 سيطبق بعض الإصلاحات على قائمة التصحيح الموجودة، مثل AREA JUMP وغيرها، لكنه لن يضيف إدخالاتنا المخصصة لضيق المساحة.");
						ImGui::TextWrapped("(قد يسبب نادراً انهياراً بسبب تلف الذاكرة عند تحميل حفظ، أما إذا تم تحميل اللعبة بشكل سليم فلا ينبغي أن يحدث أي انهيار)");
					}

					// ShowGameOutput
					if ((OptionsFilter.PassFilter("ShowGameOutput مخرجات اللعبة السجل") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("عرض مخرجات اللعبة (ShowGameOutput)", &re4t::cfg->bShowGameOutput);

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("يعرض سجلات اللعبة الأصلية/مخرجات التصحيح في نافذة وحدة تحكم. (F2 افتراضياً)");

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						re4t::cfg->HasUnsavedChanges |= ImGui::Checkbox("حفظ مخرجات اللعبة (SaveGameOutput)", &re4t::cfg->bSaveGameOutput);
						ImGui::TextWrapped("حفظ رسائل مخرجات اللعبة في \"game_output.log\".");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Memory)
			{
				if (ImGui::BeginTable("Memory", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// AllowHighResolutionSFD
					if ((OptionsFilter.PassFilter("AllowHighResolutionSFD فيديو SFD دقة عالية الذاكرة") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("السماح بدقة عالية لفيديو SFD (AllowHighResolutionSFD)", &re4t::cfg->bAllowHighResolutionSFD))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("تخصيص ذاكرة أكبر لملفات فيديو SFD، وتحجيم دقة العرض بشكل صحيح فوق 512x336.");
						ImGui::TextWrapped("لم يُختبر فوق 1920x1080.");
					}

					// RaiseVertexAlloc
					if ((OptionsFilter.PassFilter("RaiseVertexAlloc الذاكرة زيادة التخصيص") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("رفع تخصيص الرؤوس (RaiseVertexAlloc)", &re4t::cfg->bRaiseVertexAlloc))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("تخصيص ذاكرة أكبر لبعض مخازن الرؤوس.");
						ImGui::TextWrapped("يمنع هذا انهياراً قد يحدث عند اللعب بـ FOV عالٍ.");
					}

					// RaiseInventoryAlloc
					if ((OptionsFilter.PassFilter("RaiseInventoryAlloc الحقيبة الذاكرة زيادة التخصيص") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						if (ImGui::Checkbox("رفع تخصيص الحقيبة (RaiseInventoryAlloc)", &re4t::cfg->bRaiseInventoryAlloc))
						{
							re4t::cfg->HasUnsavedChanges = true;
							NeedsToRestart = true;
						}

						ImGui_ItemSeparator();

						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						ImGui::TextWrapped("تخصيص ذاكرة أكبر لشاشة الحقيبة، لمنع الانهيار مع النماذج عالية التفاصيل داخل ss_pzzl.dat.");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Hotkeys)
			{
				float btn_size_x = 150 * esHook._cur_monitor_dpi;

				if (ImGui::BeginTable("HotkeysTop", 1, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui::TableNextColumn();

					ImGui_BeginBackground();
					ImGui::BeginGroup();
					ImGui::TextWrapped("توليفات المفاتيح لمختلف ميزات re4_tweaks.");
					ImGui::TextWrapped("يمكن دمج معظم المفاتيح (بحيث يلزم الضغط على عدة مفاتيح في الوقت نفسه). للدمج، اضغط مع الاستمرار على مفتاح واحد ثم اضغط على آخر.");
					ImGui::TextWrapped("(اضغط \"حفظ\" لتطبيق التغييرات.)");
					ImGui::EndGroup();
					ImGui_EndBackground();

					ImGui::EndTable();
				}

				ImGui::Dummy(ImVec2(0.0f, 10.0f));

				if (ImGui::BeginTable("Hotkeys", columnCount, ImGuiTableFlags_PadOuterX, ImVec2(ImGui::GetItemRectSize().x - 12, 0)))
				{
					ImGui_ColumnInit();

					// cfgMenu 
					if ((OptionsFilter.PassFilter("config menu قائمة الإعدادات فتح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("توليفة المفاتيح لفتح قائمة إعدادات re4_tweaks");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(1);
						if (ImGui::Button(re4t::cfg->sConfigMenuKeyCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							re4t::cfg->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyComboThread, &re4t::cfg->sConfigMenuKeyCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("فتح قائمة الإعدادات");
					}

					// Console
					if ((OptionsFilter.PassFilter("debug console وحدة التحكم التصحيح فتح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("توليفة المفاتيح لفتح وحدة تحكم التصحيح في re4_tweaks");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(2);
						if (ImGui::Button(re4t::cfg->sConsoleKeyCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							re4t::cfg->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyComboThread, &re4t::cfg->sConsoleKeyCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("فتح وحدة التحكم");
					}

					// Inventory rotate / flip
					if ((OptionsFilter.PassFilter("inventory rotate flip الحقيبة تدوير قلب") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("اختصارات تدوير الأغراض وقلبها في شاشة الحقيبة عند استخدام لوحة المفاتيح والماوس.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("التدوير يغيّر المساحة التي يشغلها الغرض في الحقيبة (مثلاً من 1x2 إلى 2x1). أما القلب فيغيّر اتجاه النموذج ثلاثي الأبعاد فقط دون تغيير المساحة.");
						ImGui::Spacing();
						ImGui::BulletText("توليفات المفاتيح غير مدعومة");
						ImGui::Spacing();

						ImGui::TextWrapped("تدوير باتجاه عقارب الساعة (يغيّر المساحة)");
						ImGui::PushID("RotateItem");
						if (ImGui::Button(re4t::cfg->sRotateItem.c_str(), ImVec2(btn_size_x, 0)))
						{
							re4t::cfg->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyThread, &re4t::cfg->sRotateItem, 0, NULL);
						}
						ImGui::PopID();
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));
						
						if (ImGui::BeginTable("inv flip", 2))
						{
							ImGui::TableNextColumn();
							ImGui::Spacing();

							// UP
							ImGui::TextWrapped("قلب للأعلى");
							ImGui::PushID(3);
							if (ImGui::Button(re4t::cfg->sFlipItemUp.c_str(), ImVec2(btn_size_x, 0)))
							{
								re4t::cfg->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyThread, &re4t::cfg->sFlipItemUp, 0, NULL);
							}
							ImGui::PopID();

							ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

							// DOWN
							ImGui::TextWrapped("قلب للأسفل");
							ImGui::PushID(4);
							if (ImGui::Button(re4t::cfg->sFlipItemDown.c_str(), ImVec2(btn_size_x, 0)))
							{
								re4t::cfg->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyThread, &re4t::cfg->sFlipItemDown, 0, NULL);
							}
							ImGui::PopID();

							ImGui::TableNextColumn();

							// LEFT
							ImGui::TextWrapped("قلب لليسار");
							ImGui::PushID(5);
							if (ImGui::Button(re4t::cfg->sFlipItemLeft.c_str(), ImVec2(btn_size_x, 0)))
							{
								re4t::cfg->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyThread, &re4t::cfg->sFlipItemLeft, 0, NULL);
							}
							ImGui::PopID();

							ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

							// RIGHT
							ImGui::TextWrapped("قلب لليمين");
							ImGui::PushID(6);
							if (ImGui::Button(re4t::cfg->sFlipItemRight.c_str(), ImVec2(btn_size_x, 0)))
							{
								re4t::cfg->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyThread, &re4t::cfg->sFlipItemRight, 0, NULL);
							}
							ImGui::PopID();

							ImGui::EndTable();
						}
					}

					// QTE 
					if ((OptionsFilter.PassFilter("QTE مفاتيح QTE اختصارات") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("اختصارات مفاتيح QTE عند اللعب بلوحة المفاتيح والماوس.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("بخلاف الطريقة \"الرسمية\" لإعادة تعيين المفاتيح عبر usrpInput.ini، يغيّر هذا الخيار أيضاً الإشارة على الشاشة لتطابق المفتاح المحدد.");
						ImGui::BulletText("توليفات المفاتيح غير مدعومة");
						ImGui::Spacing();

						if (ImGui::BeginTable("qtekey", 2))
						{
							ImGui::TableNextColumn();
							ImGui::Spacing();

							// QTE1
							ImGui::TextWrapped("مفتاح QTE 1");
							ImGui::PushID(7);
							if (ImGui::Button(re4t::cfg->sQTE_key_1.c_str(), ImVec2(btn_size_x, 0)))
							{
								re4t::cfg->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyThread, &re4t::cfg->sQTE_key_1, 0, NULL);
							}
							ImGui::PopID();

							ImGui::TableNextColumn();

							// QTE2
							ImGui::TextWrapped("مفتاح QTE 2");
							ImGui::PushID(8);
							if (ImGui::Button(re4t::cfg->sQTE_key_2.c_str(), ImVec2(btn_size_x, 0)))
							{
								re4t::cfg->HasUnsavedChanges = true;
								CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyThread, &re4t::cfg->sQTE_key_2, 0, NULL);
							}
							ImGui::PopID();

							ImGui::EndTable();
						}
					}

					// Debug menu
					if ((OptionsFilter.PassFilter("debug menu قائمة التصحيح فتح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("توليفة المفاتيح لإظهار قائمة التصحيح \"tool menu\".");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("يتطلب تفعيل EnableDebugMenu.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(9);
						if (ImGui::Button(re4t::cfg->sDebugMenuKeyCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							re4t::cfg->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyComboThread, &re4t::cfg->sDebugMenuKeyCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("فتح قائمة التصحيح");
					}

					// MouseTurningModifier
					if ((OptionsFilter.PassFilter("MouseTurningModifier الماوس الدوران مفتاح") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("مفتاح تعديل كاميرا MouseTurning.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::TextWrapped("عند الضغط مع الاستمرار على هذه التوليفة، يُفعَّل MouseTurning أو يُعطَّل مؤقتاً.");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(10);
						if (ImGui::Button(re4t::cfg->sMouseTurnModifierKeyCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							re4t::cfg->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyComboThread, &re4t::cfg->sMouseTurnModifierKeyCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("مفتاح تعديل MouseTurning");
					}

					// JetSkiTricks 
					if ((OptionsFilter.PassFilter("JetSkiTricks الدراجة المائية حركات") && OptionsFilter.IsActive()) || !OptionsFilter.IsActive())
					{
						ImGui_ColumnSwitch();

						ImGui::TextWrapped("توليفة المفاتيح لحركات القفز أثناء مقطع الدراجة المائية (عادةً RT+LT على يد التحكم)");
						ImGui::Dummy(ImVec2(10, 10 * esHook._cur_monitor_dpi));

						ImGui::PushID(11);
						if (ImGui::Button(re4t::cfg->sJetSkiTrickCombo.c_str(), ImVec2(btn_size_x, 0)))
						{
							re4t::cfg->HasUnsavedChanges = true;
							CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&ImGui_SetHotkeyComboThread, &re4t::cfg->sJetSkiTrickCombo, 0, NULL);
						}
						ImGui::PopID();

						ImGui::SameLine();

						ImGui::TextWrapped("حركة أثناء ركوب الدراجة المائية");
					}

					ImGui_ColumnFinish();
					ImGui::EndTable();
				}
			}

			if (Tab == MenuTab::Trainer)
			{
				Trainer_RenderUI(columnCount);
			}

			ImGui::PopStyleVar();

			ImGui::EndChild();
		}

		//
		ImGui::End();
	}
}

void ShowCfgMenuTip()
{
	if (!bCfgMenuOpen)
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(20, viewport->Pos.y + 20));
		ImGui::SetNextWindowBgAlpha(0.5);

		ImGui::Begin("cfgMenuTip", nullptr, ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);

		std::string tooltip = std::string("re4_tweaks: اضغط ") + re4t::cfg->sConfigMenuKeyCombo.c_str() + std::string(" لفتح قائمة الإعدادات");

		ImGui::TextUnformatted(tooltip.data());

		ImGui::End();
	}
}
