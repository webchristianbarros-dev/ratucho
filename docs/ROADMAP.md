# Ratucho — Roadmap da primeira versão jogável

Referências: `docs/CONCEPT.md` (documento principal) e o prompt de produção de 18/11/2026.

## Etapa 0 — Auditoria (18/11/2026)

| Item | Situação encontrada |
|---|---|
| Projeto oficial | `Ratucho.uproject` na **raiz** do repositório (decisão do usuário). Já tinha o perfil de hardware, a estrutura de pastas, o MCP e o Git configurados. |
| `ratucho\Ratucho\Ratucho.uproject` | Projeto em branco criado pelo usuário: sem assets, com Lumen, RT e VSM ligados. **Não foi alterado.** Remover só com autorização. |
| Conteúdo | Só os assets de teste do MCP (`Developers/MCPTest`). Nenhum mapa, Blueprint ou sistema de jogo. |
| Plugins | MCP + Toolsets (só para o editor), PythonScript, EditorScriptingUtilities, ModelingTools, EnhancedInput |
| Input | Enhanced Input (padrão da 5.8). As ações são criadas em código no `ARatPlayerController`. |
| Build/packaging | Configuração de desenvolvimento, IoStore. `Developers/` e `Maps/Test` ficam fora do cook. |
| C++ | Visual Studio 2026 Insiders, MSVC 14.52.37032 e Windows SDK 10.0.26100. A UBT compila, com o aviso "versão não preferida" (a preferida pela 5.8.4 é a 14.50). |
| Git | 2 commits locais, sem push. O Git não está no PATH (é o do GitHub Desktop). |

### Divergências registradas (o prompt prevalece no protótipo)
1. **Rede:** `CONCEPT.md` §1 e §12 dizem para "não implementar rede antes da partida local divertida" e tiram "multiplayer online" do escopo. O prompt e o §11 exigem servidor autoritativo e Listen Server validado com 2 processos. **Decisão:** rede LAN com Listen Server desde a Etapa 1. Steam e internet ficam para depois.
2. **Caminho do projeto:** `CONCEPT.md` §4 cita `ratucho\Ratucho\`. Por decisão do usuário, o projeto oficial é o da raiz.
3. **Exemplo do feed:** `CONCEPT.md` usa "Ratão eliminou Pipoca!". Adotamos o padrão do prompt ("Pipoca eliminou Bigode!", "Você eliminou Fubá!").

### Riscos
- **GPU de 4 GB:** a direção de arte exige uma cozinha bonita. É preciso medir no build empacotado antes de prometer qualidade ou FPS.
- **Desempenho dos bots:** com a navegação da IA e a CPU de 4 núcleos, o limite de bots será definido por medição (começa conservador).
- **Escalada em rede:** é um modo customizado do CharacterMovement, determinístico por posição e input. Ainda precisa ser validado com o `p.NetShowCorrections`.
- **MSVC 14.52** não é a versão preferida pela Epic. Se aparecer um bug de compilador, instalar o componente MSVC 14.50 (LTSC).
- **Arte:** os modelos de qualidade (rato, cozinha) são o maior volume de trabalho, no Blender e com MCP.

## Arquitetura (C++ para regras e rede, Blueprint para visual e ajustes)

| Sistema | Classe | Autoridade |
|---|---|---|
| Regras da rodada, spawn, bots | `ARatuchoGameMode` | servidor |
| Cronômetro, fase, estado do queijo | `ARatuchoGameState` (Etapa 3) | replicado |
| Nome, eliminações, cor | `ARatPlayerState` (Etapa 3) | replicado |
| Movimento, corrida, escalada | `URatMovementComponent` | CharacterMovement com predição |
| Personagem, câmera, visual | `ARatCharacter` | — |
| Vida, dano, morte | `URatHealthComponent` (Etapa 4) | servidor |
| Armas e projéteis | `URatWeaponDefinition` (Data Asset), `ARatProjectile` (Etapa 4) | servidor valida |
| Queijo | `ACheese` (Etapa 3) | servidor |
| Superfícies escaláveis | `AClimbVolume` | geometria do mapa |
| Pontos de spawn e queijo | `ARatSpawnPoint`, `ACheeseSpawnPoint` (Etapa 2/3) | servidor escolhe |
| Bots | `ARatBotController` + perfis (Etapa 5) | só no servidor |
| Input local e pedidos ao servidor | `ARatPlayerController` | cliente → RPC |
| HUD, feed, Tab, resultado, menu | UMG (Etapa 6) | só exibe |

## Etapas
| # | Etapa | Status |
|---|---|---|
| 0 | Auditoria e plano | ✅ |
| 1 | Movimento e câmera: rato placeholder, corrida predita, pulo, escalada em `AClimbVolume` | ✅ validado (escalada da bancada no log) |
| 2 | Cozinha greybox: plataformas, rotas, pontos de spawn e de queijo, navmesh e navlinks | ✅ gerada por `Tools/Unreal/build_levels.py` |
| 3 | Queijo e partida: queijo único, posse, E/G, queda na morte, 120 s, vitória/derrota | ✅ (o caso "ninguém venceu" ainda não foi observado) |
| 4 | Armas e combate: estilingue de rolhas, lançador de tampinhas, vida, kills, respawn seguro | ✅ |
| 5 | Bots: nomes únicos, perfis, percepção, navegação, combate | ✅ v1 simples |
| 6 | Menu e UI: Iniciar, mapa, bots, arma, HUD, feed, Tab, resultado, reinício | ✅ |
| 7 | Arte e polimento: modelos do Blender, materiais, iluminação, perfis gráficos | 🔧 só a iluminação interna quente e o material sólido; modelos ainda não feitos |
| 8 | Validação de replicação: auditoria de autoridade e estado | ✅ regras no servidor e feed idêntico em host e cliente |
| 9 | Listen Server com 2 processos: host + cliente, fluxo de conexão no menu | ✅ 2 processos locais. LAN entre 2 PCs não testada |
| 10 | Build empacotado: teste sem Editor/MCP, FPS e VRAM | 🔧 ver o relatório da Etapa 10 abaixo |

Detalhes de teste: `docs/TESTING.md`. Valores: `docs/BALANCE.md`.

Padrão do Tab: **segurar para mostrar** (fecha ao soltar), como em shooters competitivos.
