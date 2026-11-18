# Ratucho — guia para o Claude Code

Jogo comercial em **Unreal Engine 5.8.4** (`C:\Users\Chris\Epic Games\UE_5.8`) com arte feita no **Blender 5.3**.
Projeto: `Ratucho.uproject` na raiz do repositório. No momento é só Blueprint; o C++ será adicionado depois (ver `docs/ENVIRONMENT.md`).

## Ferramentas conectadas (MCP)
- **unreal** → plugin nativo da Epic `ModelContextProtocol`, em `http://127.0.0.1:8000/mcp`. Só responde com o editor aberto.
  Com o tool search ligado, o servidor expõe `list_toolsets`, `describe_toolset` e `call_tool`. Descubra a ferramenta antes de chamá-la.
- **blender** → addon `blender-mcp` (socket em `localhost:9876`), servidor iniciado com `uvx blender-mcp`. Precisa do Blender aberto, com "Connect to MCP server" clicado no painel lateral (N).

## Regras de trabalho
- Nunca apague nem mova assets sem autorização. Na Unreal, renomeie e mova pelo editor ou pelo MCP (isso preserva os redirectors), nunca pelo sistema de arquivos.
- Testes e experimentos ficam em `/Game/Ratucho/Developers` ou em `/Game/Ratucho/Maps/Test`. Essas pastas não entram no build.
- Commite com frequência. `.uasset`, `.umap` e `.blend` vão por Git LFS.
- Respeite o perfil de hardware (`docs/HARDWARE.md`): sem Lumen, sem Virtual Shadow Maps, texturas de no máximo 2K e orçamento de polígonos moderado.
- Arquitetura pronta para multiplayer (`docs/ARCHITECTURE.md`): a lógica de regras fica no servidor, e cada estado mora na classe certa.

## Convenção de nomes (prefixos)
| Tipo | Prefixo | Tipo | Prefixo |
|---|---|---|---|
| Blueprint | `BP_` | Widget | `WBP_` |
| Blueprint Interface | `BPI_` | Componente BP | `BPC_` |
| Static Mesh | `SM_` | Skeletal Mesh | `SK_` |
| Material | `M_` | Material Instance | `MI_` |
| Material Function | `MF_` | Textura | `T_` (sufixos `_D`, `_N`, `_ORM`) |
| Animation Sequence | `A_` | Anim Blueprint | `ABP_` |
| Montage | `AM_` | Blend Space | `BS_` |
| Sound Wave/Cue | `SW_` / `SC_` | MetaSound | `MS_` |
| Niagara | `NS_` | Data Asset/Table | `DA_` / `DT_` |
| Input Action/Context | `IA_` / `IMC_` | Mapa | `L_` |
| Enum / Struct | `E_` / `F_` | Game Mode | `GM_` |

## Fluxo Blender → Unreal
1. A fonte fica em `SourceArt/Blender/<Categoria>/<nome>.blend`, com escala métrica (1 unidade = 1 m) e eixo +Z para cima.
2. Exporte o FBX para `SourceArt/Export/FBX/` (Forward -Y, Up Z, Apply Transform, smoothing por face).
3. Importe na pasta certa em `/Game/Ratucho/Assets` ou `/Game/Ratucho/Characters`.
