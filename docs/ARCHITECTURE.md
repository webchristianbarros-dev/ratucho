# Arquitetura do projeto

> Nada aqui está implementado ainda. Este documento é a regra a seguir quando as funcionalidades começarem.

## Estrutura de conteúdo (`/Game/Ratucho`)
| Pasta | Conteúdo |
|---|---|
| `Core/` | GameInstance, GameMode, GameState, PlayerController, PlayerState: o esqueleto do framework |
| `Systems/` | Sistemas de jogo independentes (Interaction, Inventory, SaveGame, Abilities, Data) feitos como **Actor Components** e Data Assets |
| `Blueprints/` | Atores genéricos, componentes reutilizáveis, Interfaces e Function Libraries |
| `Characters/` | Personagens (Player, NPC, Shared): BP, meshes e materiais específicos |
| `Animations/` | ABPs, montages, blend spaces |
| `Materials/` | `Master` (poucos materiais-mestre), `Instances` (MIs) e `Functions` |
| `Assets/` | Meshes, texturas, props, ambiente e VFX compartilhados |
| `Maps/` | `Levels` (mapas do jogo) e `Test` (fora do build) |
| `UI/` | HUD, menus, widgets, fontes e texturas de UI |
| `Audio/` | Music, SFX, Ambience e Mixes (submixes/classes) |
| `Input/` | Enhanced Input: `IA_` e `IMC_` |
| `Developers/` | Área de rascunho (fora do build) |

Fora de `Content/`: `SourceArt/` guarda os arquivos-fonte (.blend, .psd, FBX exportado) e `Tools/` guarda scripts auxiliares.

## Preparado para multiplayer (sem implementar)
Seguir o modelo **servidor autoritativo** da Unreal desde o primeiro Blueprint, mesmo que o jogo seja single-player no início:

| Classe | Existe em | Responsável por |
|---|---|---|
| GameMode | só no servidor | Regras, spawn, condições de vitória. **Nunca** guardar estado que o cliente precise ler. |
| GameState | servidor + todos os clientes | Estado global replicado (tempo, placar, fase da partida) |
| PlayerController | servidor + o próprio dono | Input, câmera, UI. Pedidos ao servidor via RPC `Server_` |
| PlayerState | servidor + todos os clientes | Dados persistentes do jogador na partida (nome, pontuação) |
| Pawn/Character | servidor + clientes relevantes | Representação física. Movimento pelo CharacterMovementComponent (já replicado) |
| GameInstance | local | Dados entre mapas, sessão, opções |
| HUD/Widgets | só no cliente | Apenas leem o estado, nunca decidem regras |

Regras práticas:
- Qualquer mudança de gameplay acontece com **"Has Authority"** ou via RPC `Server_`, e o estado vai por variáveis **Replicated / RepNotify**.
- Os sistemas em `Systems/` são **Actor Components** com a replicação ligada quando for preciso. Isso evita colocar a lógica dentro do Character.
- Cosméticos (som, partículas, UI) ficam no cliente, disparados por RepNotify ou por RPC `Multicast_`.
- Use Gameplay Tags e Data Assets em vez de strings e valores fixos no código.
- Teste com *Play → Net Mode: Play As Listen Server*, com 2 jogadores, assim que existir gameplay.
- Se o projeto crescer, avaliar Gameplay Ability System (GAS) e Online Subsystem / EOS, mas só quando forem necessários.

## Blueprint × C++
- **Agora:** projeto só Blueprint. Dá para prototipar com Blueprint e com o MCP da Unreal.
- **Depois:** adicionar um módulo C++ `Ratucho` (precisa do Visual Studio, ver `docs/ENVIRONMENT.md`) para as classes-base do Core e dos sistemas que forem críticos em desempenho ou em rede. Os Blueprints passam a herdar dessas classes C++.
