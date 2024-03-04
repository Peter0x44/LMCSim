#include "raylib.h"
#include "raymath.h"

#include "lmc.h"

#include "imgui.h"
#include "imgui_stdlib.h" // InputText with std::string

#include "rlImGui.h"

#include <string>
#include <iostream>

// TODO investigate if it is possible omit writing the function call, by providing a conversion operator for s8
s8 stringTos8(std::string_view s)
{
	return s8{(unsigned char*)s.data(), (ptrdiff_t)s.length()};
}

bool IntInputBoxZeroPadded(const char* label, int* v, ImGuiInputTextFlags flags)
{
	const char format[] = "%03d";

	// NOTE: ImGui doesn't seem to care about overflow - writing too large values can be negative.
	// The same is true for too small values, they can be positive. It's not ideal, but I don't really care.
	// From what I can tell, I'd have to replace InputScalar to get that to work.
	bool ret = ImGui::InputScalar(label, ImGuiDataType_S32, (void*)v, nullptr, nullptr, format, flags);

	// Peter higginson LMC has these limits for its entry boxes
	// Annoyingly, it pops up a Javascript alert() box if the inputs are out of range.
	// This is also done in a loop - it seems "buggy", I managed to get it in several weird states where I couldn't
	// write anything into the input box. I'm just going to silently make the value fit in that range here.
	if (*v > 999) *v = 999;
	if (*v < -999) *v = -999;


	return ret;

}

void InitProgram(void)
{
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

	InitWindow(0, 0, "LMCSim - GUI");
	SetExitKey(KEY_NULL);
	SetTargetFPS(300);

	bool dark = true;
	rlImGuiSetup(dark);
}

void ShutdownProgram(void)
{
	rlImGuiShutdown();

	CloseWindow();
}

int main(void)
{
	InitProgram();

	std::string code("");

	LMCContext x = {};

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(DARKGRAY);


		rlImGuiBegin();

		const ImGuiWindowFlags flags = 0
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoFocusOnAppearing
			;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		if (ImGui::Begin("Root", nullptr, flags))
		{
			ImGui::InputTextMultiline("##CodeEditor", &code, ImVec2(GetScreenWidth()*(1/3.0f), GetScreenHeight()-60.0f), ImGuiInputTextFlags_AllowTabInput);

			if (ImGui::Button("Assemble into RAM"))
			{
				Assemble(stringTos8(code), &x, true);
				puts("");
				for (int i = 0; i < 10; ++i)
				{
					for (int j = 0; j < 10; ++j)
					{
						printf("%03d ", x.mailBoxes[i*10+j]);
					}
					puts("");
				}
			}

			ImGui::SameLine();
			//if (running) {
			if (ImGui::Button("Run"))
			{
			}
			//else {
			/*
				if (ImGui::Button("Stop"))
				{
				}
			}
			*/

			if (ImGui::Button("Reset"))
			{
			}

			if (ImGui::BeginTable("opcodetable", 10, ImGuiTableFlags_Borders))
			{
				for (int i = 0; i < 10; ++i)
				{
					ImGui::TableNextRow();
					for (int j = 0; j < 10; ++j)
					{
						ImGui::TableSetColumnIndex(j);

						const char* label = TextFormat("%d", i*10+j);
						ImGui::TextUnformatted(label);
						const char* idName = TextFormat("###Item %d", i*10+j);
						IntInputBoxZeroPadded(idName, &x.mailBoxes[i*10+j], ImGuiInputTextFlags_CharsDecimal);
					}
				}
				ImGui::EndTable();
			}

			ImGui::End();
		}
		ImGui::ShowDemoWindow();

		rlImGuiEnd();

		EndDrawing();
	}

	ShutdownProgram();

	return 0;
}
