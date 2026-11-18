# Ambiente de desenvolvimento

| Ferramenta | Versão | Local / observação |
|---|---|---|
| Unreal Engine | 5.8.4 (CL 58902052) | `C:\Users\Chris\Epic Games\UE_5.8` |
| Blender | 5.3.0 | `C:\Program Files\Blender Foundation\Blender 5.3` |
| uv / uvx | 0.13.9 (winget) | Gerencia o Python 3.12 do servidor blender-mcp. **Não há Python instalado no sistema, e não é preciso.** |
| Git / Git LFS | do GitHub Desktop 3.6.11, LFS 3.8.1 | `%LOCALAPPDATA%\GitHubDesktop\app-*\resources\app\git\cmd\git.exe`. Não está no PATH |
| Visual Studio | **não instalado** | Só é necessário para C++ |

## MCP (`.mcp.json` na raiz)
- **unreal**: HTTP em `http://127.0.0.1:8000/mcp`. Ele sobe sozinho ao abrir `Ratucho.uproject` (config em `Config/DefaultEditorPerProjectUserSettings.ini`).
  Console da Unreal: `ModelContextProtocol.StartServer 8000` / `ModelContextProtocol.StopServer`.
- **blender**: `uvx --python 3.12 blender-mcp@2.0.0`, que fala com o addon pelo socket `localhost:9876`.
  No Blender: N → aba "MCP for Blender" → "Connect to MCP server". A telemetria fica **desligada** (é opt-in nas preferências do addon).
- O Claude Code só carrega o `.mcp.json` ao **iniciar uma sessão** nesta pasta. Na primeira vez, aprove os servidores `unreal` e `blender`.

## Para ativar C++ (quando decidirmos)
A UE 5.8.4 aceita (conforme `Engine/Config/Windows/Windows_SDK.json`):
- **Visual Studio 2022 17.14** (MSVC 14.44.35211 ou mais novo), **ou** Visual Studio 2026 18.x (MSVC 14.50.35723 ou mais novo).
- Workloads: *Desenvolvimento para desktop com C++*, *Desenvolvimento de jogos com C++* (inclui os componentes Unreal IDE/Debugger) e *.NET desktop*.
- Componentes: MSVC v143 14.44 x64/x86, Windows 11 SDK 10.0.22621, Clang/LLVM (opcional) e .NET Framework 4.6.2 targeting pack.
- O download tem de 20 a 40 GB. Alternativa de IDE: JetBrains Rider, que também precisa das build tools do MSVC.
- Depois de instalar: Tools → New C++ Class na Unreal, e o projeto passa a ter `Source/Ratucho`.
