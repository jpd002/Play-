#include "VirtualPad.h"

CVirtualPad::ItemArray CVirtualPad::GetItems(float screenWidth, float screenHeight)
{
	ItemArray items;

	float uiScale = 1;
	if(screenHeight < 480)
	{
		uiScale = 1.5f;
	}
	else if(screenHeight < 768)
	{
		uiScale = 1.25f;
	}

	float analogStickSize = 96.0f / uiScale;
	float lrButtonWidth = 128.0f / uiScale;
	float lrButtonHeight = 64.0f / uiScale;
	float lr3ButtonSize = 64.0f / uiScale;
	float dpadButtonSize = 32.0f / uiScale;
	float margin = 32.0f / uiScale;
	float padButtonSize = 64.0f / uiScale;

	float dpadPosX = margin;
	float dpadPosY = screenHeight - (padButtonSize * 3) - margin;
	float actionPadPosX = screenWidth - (padButtonSize * 3) - margin;
	float actionPadPosY = screenHeight - (padButtonSize * 3) - margin;
	float startSelPadPosX = (screenWidth - (padButtonSize * 3)) / 2;
	float startSelPadPosY = screenHeight - padButtonSize - margin;
	float leftButtonsPosX = margin;
	float leftButtonsPosY = margin;
	float rightButtonsPosX = screenWidth - (margin + lrButtonWidth);
	float rightButtonsPosY = margin;
	float leftAnalogStickPosX = dpadPosX + (padButtonSize * 3) + analogStickSize;
	float rightAnalogStickPosX = actionPadPosX - (analogStickSize * 2);
	float analogStickPosY = screenHeight - (padButtonSize * 3) - margin;
	float l3ButtonPosX = startSelPadPosX - (lr3ButtonSize * 2);
	float r3ButtonPosX = startSelPadPosX + (padButtonSize * 3) + lr3ButtonSize;
	float lr3ButtonPosY = screenHeight - padButtonSize - margin;

	items.push_back(CreateButtonItem(
		leftButtonsPosX, leftButtonsPosY, leftButtonsPosX + lrButtonWidth, leftButtonsPosY + lrButtonHeight,
		PS2::CControllerInfo::L2, "lr", "L2"));
	items.push_back(CreateButtonItem(
		leftButtonsPosX, leftButtonsPosY + lrButtonHeight, leftButtonsPosX + lrButtonWidth, leftButtonsPosY + lrButtonHeight * 2,
		PS2::CControllerInfo::L1, "lr", "L1"));
	items.push_back(CreateButtonItem(
		rightButtonsPosX, rightButtonsPosY, rightButtonsPosX + lrButtonWidth, rightButtonsPosY + lrButtonHeight,
		PS2::CControllerInfo::R2, "lr", "R2"));
	items.push_back(CreateButtonItem(
		rightButtonsPosX, rightButtonsPosY + lrButtonHeight, rightButtonsPosX + lrButtonWidth, rightButtonsPosY + lrButtonHeight * 2,
		PS2::CControllerInfo::R1, "lr", "R1"));

	items.push_back(CreateButtonItem(
		startSelPadPosX + padButtonSize * 0, startSelPadPosY + padButtonSize / 2, startSelPadPosX + padButtonSize * 1, startSelPadPosY + padButtonSize,
		PS2::CControllerInfo::SELECT, "select"));
	items.push_back(CreateButtonItem(
		startSelPadPosX + padButtonSize * 2, startSelPadPosY + padButtonSize / 2, startSelPadPosX + padButtonSize * 3, startSelPadPosY + padButtonSize,
		PS2::CControllerInfo::START, "start"));

	items.push_back(CreateButtonItem(
		dpadPosX + dpadButtonSize * 2, dpadPosY + dpadButtonSize * 0, dpadPosX + dpadButtonSize * 4, dpadPosY + dpadButtonSize * 3,
		PS2::CControllerInfo::DPAD_UP, "up"));
	items.push_back(CreateButtonItem(
		dpadPosX + dpadButtonSize * 2, dpadPosY + dpadButtonSize * 3, dpadPosX + dpadButtonSize * 4, dpadPosY + dpadButtonSize * 6,
		PS2::CControllerInfo::DPAD_DOWN, "down"));
	items.push_back(CreateButtonItem(
		dpadPosX + dpadButtonSize * 0, dpadPosY + dpadButtonSize * 2, dpadPosX + dpadButtonSize * 3, dpadPosY + dpadButtonSize * 4,
		PS2::CControllerInfo::DPAD_LEFT, "left"));
	items.push_back(CreateButtonItem(
		dpadPosX + dpadButtonSize * 3, dpadPosY + dpadButtonSize * 2, dpadPosX + dpadButtonSize * 6, dpadPosY + dpadButtonSize * 4,
		PS2::CControllerInfo::DPAD_RIGHT, "right"));

	items.push_back(CreateButtonItem(
		actionPadPosX + padButtonSize * 1, actionPadPosY + padButtonSize * 0, actionPadPosX + padButtonSize * 2, actionPadPosY + padButtonSize * 1,
		PS2::CControllerInfo::TRIANGLE, "triangle"));
	items.push_back(CreateButtonItem(
		actionPadPosX + padButtonSize * 1, actionPadPosY + padButtonSize * 2, actionPadPosX + padButtonSize * 2, actionPadPosY + padButtonSize * 3,
		PS2::CControllerInfo::CROSS, "cross"));
	items.push_back(CreateButtonItem(
		actionPadPosX + padButtonSize * 0, actionPadPosY + padButtonSize * 1, actionPadPosX + padButtonSize * 1, actionPadPosY + padButtonSize * 2,
		PS2::CControllerInfo::SQUARE, "square"));
	items.push_back(CreateButtonItem(
		actionPadPosX + padButtonSize * 2, actionPadPosY + padButtonSize * 1, actionPadPosX + padButtonSize * 3, actionPadPosY + padButtonSize * 2,
		PS2::CControllerInfo::CIRCLE, "circle"));

	items.push_back(CreateAnalogStickItem(
		leftAnalogStickPosX, analogStickPosY, leftAnalogStickPosX + analogStickSize, analogStickPosY + analogStickSize,
		PS2::CControllerInfo::ANALOG_LEFT_X, PS2::CControllerInfo::ANALOG_LEFT_Y, "analogStick"));
	items.push_back(CreateAnalogStickItem(
		rightAnalogStickPosX, analogStickPosY, rightAnalogStickPosX + analogStickSize, analogStickPosY + analogStickSize,
		PS2::CControllerInfo::ANALOG_RIGHT_X, PS2::CControllerInfo::ANALOG_RIGHT_Y, "analogStick"));

	items.push_back(CreateButtonItem(
		l3ButtonPosX, lr3ButtonPosY, l3ButtonPosX + lr3ButtonSize, lr3ButtonPosY + lr3ButtonSize,
		PS2::CControllerInfo::L3, "lr", "L3"));
	items.push_back(CreateButtonItem(
		r3ButtonPosX, lr3ButtonPosY, r3ButtonPosX + lr3ButtonSize, lr3ButtonPosY + lr3ButtonSize,
		PS2::CControllerInfo::R3, "lr", "R3"));

	return items;
}

CVirtualPad::ITEM CVirtualPad::CreateButtonItem(float x1, float y1, float x2, float y2, PS2::CControllerInfo::BUTTON code, const std::string& imageName, const std::string& caption)
{
	ITEM item;
	item.isAnalog = false;
	item.x1 = x1;
	item.y1 = y1;
	item.x2 = x2;
	item.y2 = y2;
	item.code0 = code;
	item.imageName = imageName;
	item.caption = caption;
	return item;
}

CVirtualPad::ITEM CVirtualPad::CreateAnalogStickItem(float x1, float y1, float x2, float y2, PS2::CControllerInfo::BUTTON codeX, PS2::CControllerInfo::BUTTON codeY, const std::string& imageName)
{
	ITEM item;
	item.isAnalog = true;
	item.x1 = x1;
	item.y1 = y1;
	item.x2 = x2;
	item.y2 = y2;
	item.code0 = codeX;
	item.code1 = codeY;
	item.imageName = imageName;
	return item;
}
