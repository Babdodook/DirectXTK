![DirectX Logo](https://github.com/Microsoft/DirectXTK/wiki/X_jpg.jpg)

# DirectX Tool Kit for DirectX 11

http://go.microsoft.com/fwlink/?LinkId=248929

Copyright (c) Microsoft Corporation.

**August 1, 2021**

This package contains the "DirectX Tool Kit", a collection of helper classes for writing Direct3D 11 C++ code for Universal Windows Platform (UWP) apps for Windows 10, Xbox One, and Win32 desktop applications for Windows 7 Service Pack 1 or later.

This code is designed to build with Visual Studio 2017 ([15.9](https://walbourn.github.io/vs-2017-15-9-update/)), Visual Studio 2019, or clang for Windows v11 or later. It is recommended that you make use of the Windows 10 May 2020 Update SDK ([19041](https://walbourn.github.io/windows-10-may-2020-update-sdk/)) or later.

These components are designed to work without requiring any content from the legacy DirectX SDK. For details, see [Where is the DirectX SDK?](https://aka.ms/dxsdk).

## Directory Layout

* ``Inc\``

  + Public Header Files (in the DirectX C++ namespace):

    * Audio.h - low-level audio API using XAudio2 (DirectXTK for Audio public header)
    * BufferHelpers.h - C++ helpers for creating D3D resources from CPU data
    * CommonStates.h - factory providing commonly used D3D state objects
    * DDSTextureLoader.h - light-weight DDS file texture loader
    * DirectXHelpers.h - misc C++ helpers for D3D programming
    * Effects.h - set of built-in shaders for common rendering tasks
    * GamePad.h - gamepad controller helper using XInput
    * GeometricPrimitive.h - draws basic shapes such as cubes and spheres
    * GraphicsMemory.h - helper for managing dynamic graphics memory allocation
    * Keyboard.h - keyboard state tracking helper
    * Model.h - draws meshes loaded from .CMO, .SDKMESH, or .VBO files
    * Mouse.h - mouse helper
    * PostProcess.h - set of built-in shaders for common post-processing operations
    * PrimitiveBatch.h - simple and efficient way to draw user primitives
    * ScreenGrab.h - light-weight screen shot saver
    * SimpleMath.h - simplified C++ wrapper for DirectXMath
    * SpriteBatch.h - simple & efficient 2D sprite rendering
    * SpriteFont.h - bitmap based text rendering
    * VertexTypes.h - structures for commonly used vertex data formats
    * WICTextureLoader.h - WIC-based image file texture loader
    * XboxDDSTextureLoader.h - Xbox One exclusive apps variant of DDSTextureLoader

* ``Src\``

  + DirectXTK source files and internal implementation headers

* ``Audio\``

  + DirectXTK for Audio source files and internal implementation headers

* ``MakeSpriteFont\``

  + Command line tool used to generate binary resources for use with SpriteFont

* ``XWBTool\``

  +  Command line tool for building XACT-style wave banks for use with DirectXTK for Audio's WaveBank class

# Documentation

Documentation is available on the [GitHub wiki](https://github.com/Microsoft/DirectXTK/wiki).

## Notices

All content and source code for this package are subject to the terms of the [MIT License](http://opensource.org/licenses/MIT).

For the latest version of DirectXTK, bug reports, etc. please visit the project site on [GitHub](https://github.com/microsoft/DirectXTK).

## Release Notes

* As of the August 2021 release, **MakeSpriteFont** requires the .NET version 4.7.2 targeting pack to be installed to build. For VS 2017, this is an optional individual component *.NET Framework 4.7.2 development tools* (``Microsoft.Net.Component.4.7.2.TargetingPack``).

* Starting with the June 2020 release, this library makes use of [typed enum bitmask flags](https://walbourn.github.io/modern-c++-bitmask-types/) per the recommendation of the _C++ Standard_ section *17.5.2.1.3 Bitmask types*. This may have *breaking change* impacts to client code:

  * You cannot pass the ``0`` literal as your flags value. Instead you must make use of the appropriate default enum value: ``AudioEngine_Default``, ``SoundEffectInstance_Default``, ``ModelLoader_Clockwise``, or ``WIC_LOADER_DEFAULT``.

  * Use the enum type instead of ``DWORD`` if building up flags values locally with bitmask operations. For example, ```WIC_LOADER_FLAGS flags = WIC_LOADER_DEFAULT; if (...) flags |= WIC_LOADER_FORCE_SRGB;```

* The UWP projects and the VS 2019 Win10 classic desktop project include configurations for the ARM64 platform. These require VS 2017 (15.9 update) or VS 2019 to build, with the ARM64 toolset installed.

* The ``CompileShaders.cmd`` script must have Windows-style (CRLF) line-endings. If it is changed to Linux-style (LF) line-endings, it can fail to build all the required shaders.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general). Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party's policies.
