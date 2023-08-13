#pragma once

enum class BitmapTypes : uint8_t
{
	None = 0,
	RawBitmap = 1,
	DibBitmap = 2,
	Spliced = 3,
};

struct ColorRgba32
{
	static constexpr ColorRgba32 Black() { return ColorRgba32{ 0, 0, 0, 255 }; }
	static constexpr ColorRgba32 White() { return ColorRgba32{ 255, 255, 255, 255 }; }
	static constexpr ColorRgba32 Red() { return ColorRgba32{ 255, 0, 0, 255 }; }
	static constexpr ColorRgba32 Green() { return ColorRgba32{ 0, 255,0, 255 }; }
	static constexpr ColorRgba32 Blue() { return ColorRgba32{ 0, 0, 255, 255 }; }
	uint32_t Color;
	ColorRgba32() = default;
	explicit constexpr ColorRgba32(uint32_t color)
		: Color(color)
	{
	}
	explicit constexpr ColorRgba32(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
		: Color(alpha << alphaOffset | red << redOffset | green << greenOffset | blue << blueOffset)
	{
	}
	uint8_t GetAlpha() const { return (Color >> alphaOffset) & 0xffu; }
	uint8_t GetRed() const { return (Color >> redOffset) & 0xffu; }
	uint8_t GetGreen() const { return (Color >> greenOffset) & 0xffu; }
	uint8_t GetBlue() const { return (Color >> blueOffset) & 0xffu; }
	void SetAlpha(uint8_t val) { Color = (Color & (~(0xffu << alphaOffset))) | (val << alphaOffset); }
	void SetRed(uint8_t val) { Color = (Color & (~(0xffu << redOffset))) | (val << redOffset); }
	void SetGreen(uint8_t val) { Color = (Color & (~(0xffu << greenOffset))) | (val << greenOffset); }
	void SetBlue(uint8_t val) { Color = (Color & (~(0xffu << blueOffset))) | (val << blueOffset); }
private:
	static const unsigned alphaOffset = 3 * 8, redOffset = 2 * 8, greenOffset = 1 * 8, blueOffset = 0 * 8;
};

struct ColorRgba
{
	static constexpr ColorRgba Black() { return ColorRgba{ 0, 0, 0, 255 }; }
	static constexpr ColorRgba White() { return ColorRgba{ 255, 255, 255, 255 }; }
	static constexpr ColorRgba Red() { return ColorRgba{ 255, 0, 0, 255 }; }
	static constexpr ColorRgba Green() { return ColorRgba{ 0, 255,0, 255 }; }
	static constexpr ColorRgba Blue() { return ColorRgba{ 0, 0, 255, 255 }; }

	uint16_t Color;

	ColorRgba() = default;

	explicit constexpr ColorRgba(uint16_t color)
		: Color(color)
	{
	}

	explicit constexpr ColorRgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
		: Color(alpha >> alphaLoss << alphaOffset | red >> redLoss << redOffset | green >> greenLoss << greenOffset | blue >> blueLoss << blueOffset)
	{
	}

	uint8_t GetAlpha() const { return ((Color >> alphaOffset) & 0x0fu) << alphaLoss; }
	uint8_t GetRed() const { return ((Color >> redOffset) & 0x0fu) << redLoss; }
	uint8_t GetGreen() const { return ((Color >> greenOffset) & 0x0fu) << greenLoss; }
	uint8_t GetBlue() const { return ((Color >> blueOffset) & 0x0fu) << blueLoss; }
	void SetAlpha(uint8_t val) { Color = (Color & (~(0x0fu << alphaOffset))) | (val >> alphaLoss << alphaOffset); }
	void SetRed(uint8_t val) { Color = (Color & (~(0x0fu << redOffset))) | (val >> redLoss << redOffset); }
	void SetGreen(uint8_t val) { Color = (Color & (~(0x0fu << greenOffset))) | (val >> greenLoss << greenOffset); }
	void SetBlue(uint8_t val) { Color = (Color & (~(0x0fu << blueOffset))) | (val >> blueLoss << blueOffset); }

private:
	static const unsigned alphaOffset = 3 * 4, redOffset = 2 * 4, greenOffset = 1 * 4, blueOffset = 0 * 4;
	static const unsigned alphaLoss = 4, redLoss = 4, greenLoss = 4, blueLoss = 4;
};

static_assert(sizeof(ColorRgba) == 2, "Wrong size of RGBA color");

struct gdrv_bitmap8
{
	gdrv_bitmap8(int width, int height);
	gdrv_bitmap8(int width, int height, bool indexed);
	gdrv_bitmap8(int width, int height, bool indexed, bool bmpBuff);
	gdrv_bitmap8(const struct dat8BitBmpHeader& header);
	~gdrv_bitmap8();
	void ScaleIndexed(float scaleX, float scaleY);
	void CreateTexture(const char* scaleHint, int access);
	void BlitToTexture();
	ColorRgba* BmpBufPtr1;
	char* IndexedBmpPtr;
	int Width;
	int Height;
	int Stride;
	int IndexedStride;
	BitmapTypes BitmapType;
	int XPosition;
	int YPosition;
	unsigned Resolution;
	SDL_Texture* Texture;
};


class gdrv
{
public:
	static int display_palette(ColorRgba32* plt);
	static void fill_bitmap(gdrv_bitmap8* bmp, int width, int height, int xOff, int yOff, uint8_t fillChar);
	static void fill_bitmap(gdrv_bitmap8* bmp, int width, int height, int xOff, int yOff, ColorRgba fillColor);
	static void copy_bitmap(gdrv_bitmap8* dstBmp, int width, int height, int xOff, int yOff, gdrv_bitmap8* srcBmp,
	                        int srcXOff, int srcYOff);
	static void copy_bitmap_w_transparency(gdrv_bitmap8* dstBmp, int width, int height, int xOff, int yOff,
	                                       gdrv_bitmap8* srcBmp, int srcXOff, int srcYOff);
	static void ScrollBitmapHorizontal(gdrv_bitmap8* bmp, int xStart);
	static void grtext_draw_ttext_in_box();
	static void ApplyPalette(gdrv_bitmap8& bmp);
	static void CreatePreview(gdrv_bitmap8& bmp);
private:
	static ColorRgba current_palette[256];
};
