#include "WinUtils.h"

using namespace Framework;

HBITMAP WinUtils::CreateMask(HBITMAP nBitmap, uint32 nColor)
{
	BITMAP bm;
	GetObject(nBitmap, sizeof(BITMAP), &bm);

	HBITMAP nMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);

	HDC hDC1 = CreateCompatibleDC(0);
	HDC hDC2 = CreateCompatibleDC(0);

	SelectObject(hDC1, nBitmap);
	SelectObject(hDC2, nMask);

	SetBkColor(hDC1, nColor);
	BitBlt(hDC2, 0, 0, bm.bmWidth, bm.bmHeight, hDC1, 0, 0, SRCCOPY);
	BitBlt(hDC1, 0, 0, bm.bmWidth, bm.bmHeight, hDC2, 0, 0, SRCINVERT);

	DeleteDC(hDC1);
	DeleteDC(hDC2);

	return nMask;
}

TCHAR WinUtils::FixSlashes(TCHAR nCharacter)
{
	return (nCharacter == _T('/')) ? _T('\\') : nCharacter;
}
