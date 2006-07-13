#include "WinUtils.h"

using namespace Framework;

HBITMAP WinUtils::CreateMask(HBITMAP nBitmap, uint32 nColor)
{
	BITMAP bm;
	HBITMAP nMask;
	HDC hDC1, hDC2;
	GetObject(nBitmap, sizeof(BITMAP), &bm);
	nMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);

	hDC1 = CreateCompatibleDC(0);
	hDC2 = CreateCompatibleDC(0);

	SelectObject(hDC1, nBitmap);
	SelectObject(hDC2, nMask);

	SetBkColor(hDC1, nColor);
	BitBlt(hDC2, 0, 0, bm.bmWidth, bm.bmHeight, hDC1, 0, 0, SRCCOPY);
	BitBlt(hDC1, 0, 0, bm.bmWidth, bm.bmHeight, hDC2, 0, 0, SRCINVERT);

	DeleteDC(hDC1);
	DeleteDC(hDC2);

	return nMask;
}
