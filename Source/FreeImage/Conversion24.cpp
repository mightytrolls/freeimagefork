// ==========================================================
// Bitmap conversion routines
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Dale Larson (dlarson@norsesoft.com)
// - Hervé Drolon (drolon@infonie.fr)
// - Jani Kajala (janik@remedy.fi)
//
// This file is part of FreeImage 3
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// ==========================================================

#include "FreeImage.h"
#include "Utilities.h"

// ----------------------------------------------------------
//  internal conversions X to 24 bits
// ----------------------------------------------------------

void DLL_CALLCONV
FreeImage_ConvertLine1To24(BYTE *target, BYTE *source, int width_in_pixels, RGBQUAD *palette) {
	for (int cols = 0; cols < width_in_pixels; cols++) {
		BYTE index = (source[cols >> 3] & (0x80 >> (cols & 0x07))) != 0 ? 1 : 0;

		target[FI_RGBA_BLUE] = palette[index].rgbBlue;
		target[FI_RGBA_GREEN] = palette[index].rgbGreen;
		target[FI_RGBA_RED] = palette[index].rgbRed;

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine4To24(BYTE *target, BYTE *source, int width_in_pixels, RGBQUAD *palette) {
	BOOL low_nibble = FALSE;
	int x = 0;

	for (int cols = 0; cols < width_in_pixels; ++cols ) {
		if (low_nibble) {
			target[FI_RGBA_BLUE] = palette[LOWNIBBLE(source[x])].rgbBlue;
			target[FI_RGBA_GREEN] = palette[LOWNIBBLE(source[x])].rgbGreen;
			target[FI_RGBA_RED] = palette[LOWNIBBLE(source[x])].rgbRed;

			x++;
		} else {
			target[FI_RGBA_BLUE] = palette[HINIBBLE(source[x]) >> 4].rgbBlue;
			target[FI_RGBA_GREEN] = palette[HINIBBLE(source[x]) >> 4].rgbGreen;
			target[FI_RGBA_RED] = palette[HINIBBLE(source[x]) >> 4].rgbRed;
		}

		low_nibble = !low_nibble;

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine8To24(BYTE *target, BYTE *source, int width_in_pixels, RGBQUAD *palette) {
	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[FI_RGBA_BLUE] = palette[source[cols]].rgbBlue;
		target[FI_RGBA_GREEN] = palette[source[cols]].rgbGreen;
		target[FI_RGBA_RED] = palette[source[cols]].rgbRed;

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine16To24_555(BYTE *target, BYTE *source, int width_in_pixels) {
	WORD *bits = (WORD *)source;

	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[FI_RGBA_RED]   = (BYTE)((((bits[cols] & FI16_555_RED_MASK) >> FI16_555_RED_SHIFT) * 0xFF) / 0x1F);
		target[FI_RGBA_GREEN] = (BYTE)((((bits[cols] & FI16_555_GREEN_MASK) >> FI16_555_GREEN_SHIFT) * 0xFF) / 0x1F);
		target[FI_RGBA_BLUE]  = (BYTE)((((bits[cols] & FI16_555_BLUE_MASK) >> FI16_555_BLUE_SHIFT) * 0xFF) / 0x1F);

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine16To24_565(BYTE *target, BYTE *source, int width_in_pixels) {
	WORD *bits = (WORD *)source;

	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[FI_RGBA_RED]   = (BYTE)((((bits[cols] & FI16_565_RED_MASK) >> FI16_565_RED_SHIFT) * 0xFF) / 0x1F);
		target[FI_RGBA_GREEN] = (BYTE)((((bits[cols] & FI16_565_GREEN_MASK) >> FI16_565_GREEN_SHIFT) * 0xFF) / 0x3F);
		target[FI_RGBA_BLUE]  = (BYTE)((((bits[cols] & FI16_565_BLUE_MASK) >> FI16_565_BLUE_SHIFT) * 0xFF) / 0x1F);

		target += 3;
	}
}

void DLL_CALLCONV
FreeImage_ConvertLine32To24(BYTE *target, BYTE *source, int width_in_pixels) {
	for (int cols = 0; cols < width_in_pixels; cols++) {
		target[FI_RGBA_BLUE] = source[FI_RGBA_BLUE];
		target[FI_RGBA_GREEN] = source[FI_RGBA_GREEN];
		target[FI_RGBA_RED] = source[FI_RGBA_RED];

		target += 3;
		source += 4;
	}
}

// ----------------------------------------------------------
//   smart convert X to 24 bits
// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_ConvertTo24Bits(FIBITMAP *dib) {
	if(!dib) return NULL;

	const unsigned bpp = FreeImage_GetBPP(dib);

	const FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(dib);
	if((image_type != FIT_BITMAP) && (image_type != FIT_RGB16) && (image_type != FIT_RGBA16) && (image_type != FIT_RGBF) && (image_type != FIT_RGBAF)) {
		return NULL;
	}

	if(bpp == 24) {
		return FreeImage_Clone(dib);
	}


	const unsigned width = FreeImage_GetWidth(dib);
	const unsigned height = FreeImage_GetHeight(dib);
	FIBITMAP *new_dib = FreeImage_Allocate(width, height, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);

	if(new_dib == NULL) {
		return NULL;
	}

	const unsigned src_pitch = FreeImage_GetPitch(dib);
	const unsigned dst_pitch = FreeImage_GetPitch(new_dib);
	const BYTE *src_bits = FreeImage_GetBits(dib);
	BYTE *dst_bits = FreeImage_GetBits(new_dib);

	// copy metadata from src to dst
	FreeImage_CloneMetadata(new_dib, dib);

	if(image_type == FIT_BITMAP) {

		switch(bpp) {
			case 1 :
			{
				for (unsigned rows = 0; rows < height; rows++) {
					FreeImage_ConvertLine1To24(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width, FreeImage_GetPalette(dib));					
				}
				return new_dib;
			}

			case 4 :
			{
				for (unsigned rows = 0; rows < height; rows++) {
					FreeImage_ConvertLine4To24(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width, FreeImage_GetPalette(dib));
				}
				return new_dib;
			}
				
			case 8 :
			{
				for (unsigned rows = 0; rows < height; rows++) {
					FreeImage_ConvertLine8To24(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width, FreeImage_GetPalette(dib));
				}
				return new_dib;
			}

			case 16 :
			{
				for (unsigned rows = 0; rows < height; rows++) {
					if ((FreeImage_GetRedMask(dib) == FI16_565_RED_MASK) && (FreeImage_GetGreenMask(dib) == FI16_565_GREEN_MASK) && (FreeImage_GetBlueMask(dib) == FI16_565_BLUE_MASK)) {
						FreeImage_ConvertLine16To24_565(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
					} else {
						// includes case where all the masks are 0
						FreeImage_ConvertLine16To24_555(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
					}
				}
				return new_dib;
			}

			case 32 :
			{
				for (unsigned rows = 0; rows < height; rows++) {
					FreeImage_ConvertLine32To24(FreeImage_GetScanLine(new_dib, rows), FreeImage_GetScanLine(dib, rows), width);
				}
				return new_dib;
			}
		}
	}
	else
		if(image_type == FIT_RGB16) {
			for (unsigned rows = 0; rows < height; rows++) {
				const FIRGB16 *src_pixel = (FIRGB16*)src_bits;
				RGBTRIPLE *dst_pixel = (RGBTRIPLE*)dst_bits;
				for(unsigned cols = 0; cols < width; cols++) {
					dst_pixel[cols].rgbtRed		= (BYTE)(src_pixel[cols].red   >> 8);
					dst_pixel[cols].rgbtGreen	= (BYTE)(src_pixel[cols].green >> 8);
					dst_pixel[cols].rgbtBlue		= (BYTE)(src_pixel[cols].blue  >> 8);										
				}
				src_bits += src_pitch;
				dst_bits += dst_pitch;
			}		

		return new_dib;
	}
	else
		if(image_type == FIT_RGBA16) {
			for (unsigned rows = 0; rows < height; rows++) {
				const FIRGBA16 *src_pixel = (FIRGBA16*)src_bits;
				RGBTRIPLE *dst_pixel = (RGBTRIPLE*)dst_bits;
				for(unsigned cols = 0; cols < width; cols++) {
					dst_pixel[cols].rgbtRed		= (BYTE)(src_pixel[cols].red   >> 8);
					dst_pixel[cols].rgbtGreen	= (BYTE)(src_pixel[cols].green >> 8);
					dst_pixel[cols].rgbtBlue		= (BYTE)(src_pixel[cols].blue  >> 8);										
				}
				src_bits += src_pitch;
				dst_bits += dst_pitch;
			}		

		return new_dib;
	}
	else
		if(image_type == FIT_RGBF) {
			// Convert to 24 bits from RGBF and adjust gamma
			float slope = 4.5F;
			float start = 0.018F;
		//	gammaval = 2.2;
			
			const float fgamma = 0.409090;

			for(unsigned y = 0; y < height; y++) {
				const FIRGBF *src_pixel = (FIRGBF*)src_bits;

				BYTE *dst_pixel = (BYTE*)dst_bits;
				for(unsigned x = 0; x < width; x++) {
					const float red   = (src_pixel[x].red > 1)   ? 1 : src_pixel[x].red;
					const float green = (src_pixel[x].green > 1) ? 1 : src_pixel[x].green;
					const float blue  = (src_pixel[x].blue > 1)  ? 1 : src_pixel[x].blue;
					
					// each channel has float values in the interval [0-1]
					//dst_pixel[FI_RGBA_RED]   = (BYTE)(255 * red   + 0.5);
					dst_pixel[FI_RGBA_RED] = (BYTE)((red <= start) ? red * slope : (float)(1.099 * pow(red, fgamma) - 0.099) * 255 + 0.5);
					//dst_pixel[FI_RGBA_GREEN] = (BYTE)(255 * green + 0.5);
					dst_pixel[FI_RGBA_GREEN] = (BYTE)((green <= start) ? green * slope : (float)(1.099 * pow(green, fgamma) - 0.099) * 255 + 0.5);
					//dst_pixel[FI_RGBA_BLUE]  = (BYTE)(255 * blue  + 0.5);
					dst_pixel[FI_RGBA_BLUE] = (BYTE)((blue <= start) ? blue * slope : (float)(1.099 * pow(blue, fgamma) - 0.099) * 255 + 0.5);

					dst_pixel += 3;
				}
				src_bits += src_pitch;
				dst_bits += dst_pitch;
			}

	 return new_dib;
	}
	else
		if(image_type == FIT_RGBAF) {
			// Convert to 24 bits from RGBAF and adjust gamma
			float slope = 4.5F;
			float start = 0.018F;
		//	gammaval = 2.2;
			
			const float fgamma = 0.409090;

			for(unsigned y = 0; y < height; y++) {
				const FIRGBAF *src_pixel = (FIRGBAF*)src_bits;

				BYTE *dst_pixel = (BYTE*)dst_bits;
				for(unsigned x = 0; x < width; x++) {
					const float red   = (src_pixel[x].red > 1)   ? 1 : src_pixel[x].red;
					const float green = (src_pixel[x].green > 1) ? 1 : src_pixel[x].green;
					const float blue  = (src_pixel[x].blue > 1)  ? 1 : src_pixel[x].blue;
					
					// each channel has float values in the interval [0-1]
					//dst_pixel[FI_RGBA_RED]   = (BYTE)(255 * red   + 0.5);
					dst_pixel[FI_RGBA_RED] = (BYTE)((red <= start) ? red * slope : (float)(1.099 * pow(red, fgamma) - 0.099) * 255 + 0.5);
					//dst_pixel[FI_RGBA_GREEN] = (BYTE)(255 * green + 0.5);
					dst_pixel[FI_RGBA_GREEN] = (BYTE)((green <= start) ? green * slope : (float)(1.099 * pow(green, fgamma) - 0.099) * 255 + 0.5);
					//dst_pixel[FI_RGBA_BLUE]  = (BYTE)(255 * blue  + 0.5);
					dst_pixel[FI_RGBA_BLUE] = (BYTE)((blue <= start) ? blue * slope : (float)(1.099 * pow(blue, fgamma) - 0.099) * 255 + 0.5);

					dst_pixel += 3;
				}
				src_bits += src_pitch;
				dst_bits += dst_pitch;
			}

	 return new_dib;
	}
	
	return NULL;
}