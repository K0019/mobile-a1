/******************************************************************************/
/*!
\file   TextureCompiler.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/06/2025

\author Rocky Sutarius (100%)
\par    email: rocky.sutarius\@digipen.edu
\par    DigiPen login: rocky.sutarius

\brief
Options for compiling.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "CompileOptions.h"
#include "CompilerTypes.h"
#include <Compressonator/compressonator.h>
#include <vulkan/vulkan.h>
#include <ktx.h>
#include <ktxvulkan.h>
#include <filesystem>

namespace compiler
{
	class TextureCompiler
	{
	public:
		TextureCompiler() = default;
		~TextureCompiler() = default;

		CompilationResult Compile(const CompilerOptions& compileOptions);
		CompilationResult CompileFromMemory(const EmbeddedTextureSource& source, const CompilerOptions& compileOptions);
		// Recompress an existing compiled .ktx2 texture to the target platform format (e.g. BC7 -> ASTC for Android).
		CompilationResult RecompressKTX2(const CompilerOptions& compileOptions);

	private:
		bool LoadSourceTexture(CMP_Texture& outTex);
		bool LoadSourceTextureFromMemory(const EmbeddedTextureSource& source, CMP_Texture& outTex);
		ktxTextureCreateInfo SetupKtxCreateInfo(const CMP_Texture& dest);
		CMP_CompressOptions SetupCompressionOptions();
		bool CompressTexture(CMP_Texture& src, CMP_Texture& dst, const CMP_CompressOptions& opts);
		bool CompressTextureASTC(CMP_Texture& src, CMP_Texture& dst);  // Uses ARM's astc-encoder
		bool SaveAsKTX2(const CMP_Texture& dst, CompilationResult& compilationResult, const std::string& filename = {});

		// Thumbnail generation (64x64 preview for asset browser)
		bool GenerateThumbnail(const CMP_Texture& srcTexture, CompilationResult& result, const std::string& filename = {});

	public:
		// Save raw RGBA pixels as PNG (for material/mesh thumbnails from headless renderer)
		// Note: Function name kept as SaveThumbnailKTX2 for compatibility, but saves as PNG
		bool SaveThumbnailKTX2(const std::vector<uint8_t>& rgbaPixels, uint32_t size, const std::filesystem::path& outputPath);

		// Generate thumbnail for an already-compiled KTX2 texture (thumbnail-only mode)
		// Loads the KTX2, transcodes if needed, resizes to 64x64, saves as _thumb.png, updates meta file
		bool GenerateThumbnailOnly(const std::filesystem::path& compiledKtx2Path, const std::filesystem::path& outputDir);

	private:
		CompilerOptions options;
		static constexpr uint32_t THUMBNAIL_SIZE = 64;
		std::filesystem::path m_lastThumbnailPath;  // Thumbnail path from last GenerateThumbnail call
	};
}

//SELF NOTES:
// regarding options
// options.general.inputPath should be set to the image's filepath, duh
//			      .outputPath should be the directory to save it to.
//			for a standalone texture, name is the same as the input. for embedded textures(glb), name is provided from the caller, passed in the EmbeddedTextureSource struct.



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
