#include "pch.h"
#include "gdrv.h"

#include "GroupData.h"
#include "partman.h"
#include "pb.h"
#include "score.h"
#include "winmain.h"
#include "TTextBox.h"
#include "fullscrn.h"

#include "SDLCompatibilityLayer.h"

ColorRgba gdrv::current_palette[256]{};
SDL_Color gdrv::current_palette_SDL[256]{};

gdrv_bitmap8::gdrv_bitmap8(int width, int height)
{
	assertm(width >= 0 && height >= 0, "Negative bitmap8 dimensions");

	Width = width;
	Height = height;
	Stride = width;
	BitmapType = BitmapTypes::DibBitmap;
	Texture = nullptr;
	ScaledIndexedBmpPtr = nullptr;
	ScaleX = ScaleY = 1.0;
	PrevScaledIndexedSize = 0;
	XPosition = 0;
	YPosition = 0;
	Resolution = 0;

	IndexedBmpPtr = new char[Height * Stride];
}

gdrv_bitmap8::gdrv_bitmap8(const dat8BitBmpHeader& header)
{
	assertm(header.Width >= 0 && header.Height >= 0, "Negative bitmap8 dimensions");

	if (header.IsFlagSet(bmp8Flags::Spliced))
		BitmapType = BitmapTypes::Spliced;
	else if (header.IsFlagSet(bmp8Flags::DibBitmap))
		BitmapType = BitmapTypes::DibBitmap;
	else
		BitmapType = BitmapTypes::RawBitmap;

	Width = header.Width;
	Stride = header.Width;
	Height = header.Height;
	ScaledIndexedBmpPtr = nullptr;
	ScaleX = ScaleY = 1.0;
	PrevScaledIndexedSize = 0;
	XPosition = header.XPosition;
	YPosition = header.YPosition;
	Resolution = header.Resolution;
	Texture = nullptr;

	int sizeInBytes;
	if (BitmapType == BitmapTypes::Spliced)
	{
		sizeInBytes = header.Size;
	}
	else
	{
		if (BitmapType == BitmapTypes::RawBitmap)
			assertm(Width % 4 == 0 || header.IsFlagSet(bmp8Flags::RawBmpUnaligned), "Wrong raw bitmap align flag");
		if (Width % 4)
			Stride = Width - Width % 4 + 4;
		sizeInBytes = Height * Stride;
		assertm(sizeInBytes == header.Size, "Wrong bitmap8 size");
	}

	IndexedBmpPtr = new char[sizeInBytes];
}

gdrv_bitmap8::~gdrv_bitmap8()
{
	if (BitmapType != BitmapTypes::None) {  // !! Maybe we do not need this condition anymore
		delete[] IndexedBmpPtr;
		if (ScaledIndexedBmpPtr)
			delete[] ScaledIndexedBmpPtr;
	}
}

void gdrv_bitmap8::ScaleIndexed(float scaleX, float scaleY)
{
	if (!IndexedBmpPtr)
	{
		assertm(false, "Scaling non-indexed bitmap");
		return;
	}

	int newWidth = static_cast<int>(Width * scaleX), newHeight = static_cast<int>(Height * scaleY);
	if (Width == newWidth && Height == newHeight)
		return;

	auto newIndBuf = new char[newHeight * newWidth];
	for (int dst = 0, y = 0; y < newHeight; y++)
	{
		for (int x = 0; x < newWidth; x++, dst++)
		{
			auto px = static_cast<int>(x / scaleX);
			auto py = static_cast<int>(y / scaleY);
			newIndBuf[dst] = IndexedBmpPtr[(py * Stride) + px];
		}
	}

	Stride = Width = newWidth;
	Height = newHeight;

	delete[] IndexedBmpPtr;
	IndexedBmpPtr = newIndBuf;
}

void gdrv_bitmap8::UpdateScaledIndexedBmp(float scaleX, float scaleY)
{
	ScaleX = scaleX; ScaleY = scaleY;
	printf("17\n");
	UpdateScaledIndexedBmp();
	printf("18\n");
	CreateTexture("nearest", SDL_TEXTUREACCESS_STREAMING);  // !!
}

void gdrv_bitmap8::UpdateScaledIndexedBmp()
{
	int newWidth = static_cast<int>(Width * ScaleX), newHeight = static_cast<int>(Height * ScaleY);

	if (!ScaledIndexedBmpPtr || (newHeight * newWidth != PrevScaledIndexedSize)) {
		printf("New size: %d, old size: %d, reallocating...\n", newHeight * newWidth, PrevScaledIndexedSize);
		delete[] ScaledIndexedBmpPtr;
		PrevScaledIndexedSize = newHeight * newWidth;
		ScaledIndexedBmpPtr = new char[PrevScaledIndexedSize];
	}

	for (int dst = 0, y = 0; y < newHeight; y++)
	{
		for (int x = 0; x < newWidth; x++, dst++)
		{
			auto px = static_cast<int>(x / ScaleX);
			auto py = static_cast<int>(y / ScaleY);
			ScaledIndexedBmpPtr[dst] = IndexedBmpPtr[(py * Stride) + px];
		}
	}
}

void gdrv_bitmap8::CreateTexture(const char* scaleHint, int access)
{
	if (Texture != nullptr)
	{
		SDL_DestroyTexture(Texture);
	}

	int newWidth = static_cast<int>(Width * ScaleX), newHeight = static_cast<int>(Height * ScaleY);

	UsingSdlHint hint{ SDL_HINT_RENDER_SCALE_QUALITY, scaleHint };
	Texture = SDL_CreateTexture
	(
		winmain::Renderer,
		SDL_PIXELFORMAT_INDEX8,
		access,
		newWidth, newHeight
	);
	SDL_SetPaletteColors(Texture->format->palette, gdrv::current_palette_SDL, 0, 256);  // !! Not dynamically updated!
	SDL_SetTextureBlendMode(Texture, SDL_BLENDMODE_NONE);
}

void gdrv_bitmap8::BlitToTexture()
{
	assertm(Texture, "Updating null texture");

	int newWidth = static_cast<int>(Width * ScaleX), newHeight = static_cast<int>(Height * ScaleY);

	int pitch = 0;
	char* lockedPixels;
	auto result = SDL_LockTexture
	(
		Texture,
		nullptr,
		reinterpret_cast<void**>(&lockedPixels),
		&pitch
	);
	assertm(result == 0, "Updating non-streaming texture");
	assertm(static_cast<unsigned>(pitch) == newWidth * sizeof(char), "Padding on vScreen texture");

	printf("Scaling...\n");
	UpdateScaledIndexedBmp();
	printf("done!\nCopying pixels...\n");
	std::memcpy(lockedPixels, ScaledIndexedBmpPtr, newWidth * newHeight * sizeof(char));
	printf("done!\n");

	SDL_UnlockTexture(Texture);
}

int gdrv::display_palette(ColorRgba* plt)
{
	// Colors from Windows system palette
	const ColorRgba sysPaletteColors[10]
	{
		ColorRgba{0, 0, 0, 0}, // Color 0: transparent
		ColorRgba{0x80, 0, 0, 0xff},
		ColorRgba{0, 0x80, 0, 0xff},
		ColorRgba{0x80, 0x80, 0, 0xff},
		ColorRgba{0, 0, 0x80, 0xff},
		ColorRgba{0x80, 0, 0x80, 0xff},
		ColorRgba{0, 0x80, 0x80, 0xff},
		ColorRgba{0xC0, 0xC0, 0xC0, 0xff},
		ColorRgba{0xC0, 0xDC, 0xC0, 0xff},
		ColorRgba{0xA6, 0xCA, 0xF0, 0xff},
	};

	std::memset(current_palette, 0, sizeof current_palette);
	std::memcpy(current_palette, sysPaletteColors, sizeof sysPaletteColors);	

	for (int index = 10; plt && index < 246; index++)
	{
		auto srcClr = plt[index];
		srcClr.SetAlpha(0xff);		
		current_palette[index] = ColorRgba{ srcClr };
		current_palette[index].SetAlpha(2);  // !! ?
	}

	current_palette[255] = ColorRgba::White();

	printf("Converting palette to SDL format...\n");
	for (int i = 0; i < 256; i++)
		current_palette_SDL[i] = SDL_Color{current_palette[i].GetRed(), \
											current_palette[i].GetGreen(), \
											current_palette[i].GetBlue(), \
											current_palette[i].GetAlpha()};
	printf("done!\n");

	return 0;
}

void gdrv::fill_bitmap(gdrv_bitmap8* bmp, int width, int height, int xOff, int yOff, uint8_t fillChar)
{
	auto bmpPtr = &bmp->IndexedBmpPtr[bmp->Width * yOff + xOff];
	for (; height > 0; --height)
	{
		for (int x = width; x > 0; --x)
			*bmpPtr++ = fillChar;
		bmpPtr += bmp->Stride - width;
	}
}

void gdrv::copy_bitmap(gdrv_bitmap8* dstBmp, int width, int height, int xOff, int yOff, gdrv_bitmap8* srcBmp,
                       int srcXOff, int srcYOff)
{
	auto srcPtr = &srcBmp->IndexedBmpPtr[srcBmp->Stride * srcYOff + srcXOff];
	auto dstPtr = &dstBmp->IndexedBmpPtr[dstBmp->Stride * yOff + xOff];

	for (int y = height; y > 0; --y)
	{
		std::memcpy(dstPtr, srcPtr, width * sizeof(char));
		srcPtr += srcBmp->Stride;
		dstPtr += dstBmp->Stride;
	}
}

void gdrv::copy_bitmap_w_transparency(gdrv_bitmap8* dstBmp, int width, int height, int xOff, int yOff,
                                      gdrv_bitmap8* srcBmp, int srcXOff, int srcYOff)
{
	auto srcPtr = &srcBmp->IndexedBmpPtr[srcBmp->Stride * srcYOff + srcXOff];
	auto dstPtr = &dstBmp->IndexedBmpPtr[dstBmp->Stride * yOff + xOff];

	for (int y = height; y > 0; --y)
	{
		for (int x = width; x > 0; --x)
		{
			printf("Color index: %d\n", *srcPtr);
			if (current_palette[*srcPtr].Color)
				*dstPtr = *srcPtr;
			++srcPtr;
			++dstPtr;
		}

		srcPtr += srcBmp->Stride - width;
		dstPtr += dstBmp->Stride - width;
	}
}

void gdrv::ScrollBitmapHorizontal(gdrv_bitmap8* bmp, int xStart)
{
	auto srcPtr = bmp->IndexedBmpPtr;
	auto startOffset = xStart >= 0 ? 0 : -xStart;
	auto endOffset = xStart >= 0 ? xStart : 0;
	auto length = bmp->Width - std::abs(xStart);
	for (int y = bmp->Height; y > 0; --y)
	{
		std::memmove(srcPtr + endOffset, srcPtr + startOffset, length * sizeof(char));
		srcPtr += bmp->Stride;
	}
}


void gdrv::grtext_draw_ttext_in_box()
{
	for (const auto textBox : { pb::InfoTextBox, pb::MissTextBox })
	{
		if (textBox)
		{
			textBox->DrawImGui();
		}
	}
}

void gdrv::CreatePreview(gdrv_bitmap8& bmp)
{
	/*
	if (bmp.Texture)
		return;

	bmp.CreateTexture("nearest", SDL_TEXTUREACCESS_STATIC);
	SDL_UpdateTexture(bmp.Texture, nullptr, bmp.BmpBufPtr1, bmp.Width * sizeof(char));
	*/
}
