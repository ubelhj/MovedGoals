#include "pch.h"
#include "MovedGoals.h"

std::string MovedGoals::GetPluginName() {
    return "Moved Goals";
}

void MovedGoals::SetImGuiContext(uintptr_t ctx) {
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

void MovedGoals::RenderSettings() {
    ImGui::Columns(2);
    ImGui::TextUnformatted("Blue team settings");
    ImGui::NextColumn();
    ImGui::TextUnformatted("Orange team settings");
    ImGui::Columns(1);

    CVarWrapper distanceCvar = cvarManager->getCvar("moved_goals_back");
    if (!distanceCvar) { return; }
    int distance = distanceCvar.getIntValue();

    if (ImGui::SliderInt("Goal y value", &distance, 4900, 5200)) {
        distanceCvar.setValue(distance);
    }
    if (ImGui::IsItemHovered()) {
        std::string hoverText = "distance is " + std::to_string(distance);
        ImGui::SetTooltip(hoverText.c_str());
    }

    ImGui::Separator();
    ImGui::Columns(2);
    
    CVarWrapper enableBlueCvar = cvarManager->getCvar("moved_goals_blue");

    if (!enableBlueCvar) {
        return;
    }

    bool enabledBlue = enableBlueCvar.getBoolValue();

    if (ImGui::Checkbox("Enable for blue", &enabledBlue)) {
        gameWrapper->Execute([enableBlueCvar, enabledBlue](...) mutable {
            enableBlueCvar.setValue(enabledBlue);
            });
    }

    ImGui::NextColumn();

    CVarWrapper enableOrangeCvar = cvarManager->getCvar("moved_goals_orange");

    if (!enableOrangeCvar) {
        return;
    }

    bool enabledOrange = enableOrangeCvar.getBoolValue();

    if (ImGui::Checkbox("Enable for orange", &enabledOrange)) {
        gameWrapper->Execute([enableOrangeCvar, enabledOrange](...) mutable { 
            enableOrangeCvar.setValue(enabledOrange); 
        });
    }

    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::TextUnformatted("Plugin commissioned by Striped");
    ImGui::TextUnformatted("Plugin made by JerryTheBee#1117 - DM me on discord for custom plugin commissions!");
}

/*
// Do ImGui rendering here
void MovedGoals::Render()
{
	if (!ImGui::Begin(menuTitle_.c_str(), &isWindowOpen_, ImGuiWindowFlags_None))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImGui::End();

	if (!isWindowOpen_)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
}

// Name of the menu that is used to toggle the window.
std::string MovedGoals::GetMenuName()
{
	return "MovedGoals";
}

// Title to give the menu
std::string MovedGoals::GetMenuTitle()
{
	return menuTitle_;
}

// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
void MovedGoals::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

// Should events such as mouse clicks/key inputs be blocked so they won't reach the game
bool MovedGoals::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

// Return true if window should be interactive
bool MovedGoals::IsActiveOverlay()
{
	return true;
}

// Called when window is opened
void MovedGoals::OnOpen()
{
	isWindowOpen_ = true;
}

// Called when window is closed
void MovedGoals::OnClose()
{
	isWindowOpen_ = false;
}
*/
