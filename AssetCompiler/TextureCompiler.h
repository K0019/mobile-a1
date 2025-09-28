#pragma once
#include "CompilerBase.h"
#include <Compressonator/compressonator.h>
#include <vulkan/vulkan.h>
#include <ktx.h>
#include <ktxvulkan.h>
#include <filesystem>

namespace compiler
{
	enum ChannelFormat
	{
		RGBA_8888,
		RGBA_8888_S,
		ARGB_8888,
		ARGB_8888_S,
		RGBA_16F,
		ARGB_16F,
		ARGB_32F,
		RGBA_1010102
	};

	enum CompressionFormat
	{
		UNCOMPRESSED,
		BC1,
		BC3,
		BC4,
		BC5,
		BC7,

		//???????
		ASTC,
		ETC
	};

	enum class WrapMode 
	{ 
		CLAMP_TO_EDGE, 
		WRAP,
		MIRROR
	};
	enum class AlphaMode 
	{ 
		NONE, 
		MASK, 
		BLEND
	};

	struct TextureCompilerOptions
	{
		CommonCompileOptions commonOptions;

		ChannelFormat channelFormat;
		CompressionFormat compressionFormat;
		WrapMode wrapMode;
		AlphaMode alphaMode;

		float quality; // 0.0f-1.0f

		bool generateMips;
		int mipCount = 1;
		
		bool useSRGB;

		// Tiling and Wrapping requires their own settings
		//
	};

	class TextureCompiler
	{
	public:
		TextureCompiler() = default;
		~TextureCompiler() = default;

		bool Compile(const TextureCompilerOptions& compileOptions);


	private:
		bool LoadSourceTexture(CMP_Texture& outTex);

		ktxTextureCreateInfo SetupKtxCreateInfo(const CMP_Texture& dest);
		CMP_CompressOptions SetupCompressionOptions();
		bool CompressTexture(CMP_Texture& src, CMP_Texture& dst, const CMP_CompressOptions& opts);
		bool SaveAsKTX2(const CMP_Texture& dst);

		TextureCompilerOptions options;
	};



}
//SELF NOTES:

	// Key to format types 0xFnbC
	// C = 0 is uncompressed C > 0 is compressed
	//
	// For C = 0 uncompressed
	// F = 1 is Float data, F = 0 is Byte data,
	// nb is a format type
	//
	// For C >= 1 Compressed
	// F = 1 is signed data, F = 0 is unsigned data,
	// b = format is a BCn block comprerssor where b is 1..7 for BC1..BC7,
	// C > 1 is a varaiant of the format (example: swizzled format for DXTC, or a signed version)

/*
https://docs.unity3d.com/2020.1/Documentation/Manual/class-TextureImporterOverride.html
For devices with DirectX 11 or better class GPUs, where support for BC7 and BC6H formats is guaranteed to be available, 
the recommended choice of compression formats is : 
RGB textures - DXT1 at four bits / pixel.
RGBA textures - BC7(higher quality, slower to compress) or DXT5(BC3)(faster to compress), both at eight bits / pixel.
HDR textures - BC6H at eight bits / pixel.

BC7 quality > BC3 and BC1. but the latter's performance will be higher. 
For RGB, use BC7 or BC1.
FOr RGBA, use BC7 or BC3.
For height/displacement maps or grayscale, use BC4. Why? Single channel, 4bpp compared to BC7's 8bpp
FOr normal maps, two channel textures, BC5.


ANDROID:
ASTC is the best. ETC needed for older devices.
*/
