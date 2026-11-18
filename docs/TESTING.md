# Como jogar e testar o Ratucho

## Controles
| Tecla | Ação |
|---|---|
| WASD / mouse | mover / câmera |
| Shift | correr (só para frente) |
| Espaço | pular (escalando: pulo de parede) |
| Botão esquerdo | atirar (segure para repetir) |
| R | recarregar |
| E | pegar o queijo (até ~1,9 m) |
| G | lançar o queijo (quando você é o portador) |
| Tab | **segure** para ver o ranking (fecha ao soltar) |
| Esc | menu (continuar / voltar ao menu / sair). A partida **não pausa**. |

**Escalada:** chegue numa superfície com a **faixa verde** (frente da bancada, pernas da mesa, lateral das cadeiras, prateleira baixa) e empurre em direção a ela. W sobe, S desce, A/D vão para o lado e Espaço salta para longe. No topo o rato sobe a borda sozinho.
**Rota sem escalada:** a tábua de cortar inclinada leva do chão até a bancada.

## No Editor
1. Abra `Ratucho.uproject`. Ele abre direto em `L_Cozinha`.
2. Clique em Play. A partida começa com 5 bots e a arma padrão (estilingue).
3. Para testar a rede no Editor: Play → Number of Players = 2 → Net Mode = **Play As Listen Server**.
4. Para testar o menu, abra `Content/Ratucho/Maps/L_MainMenu` e clique em Play.

## Fora do Editor (fluxo real)
- **Build empacotado:** `Packaged/Windows/Ratucho.exe`. Abre no menu.
- **Host:** Iniciar → Cozinha → bots → arma → Começar partida. Isso cria um Listen Server na porta UDP 7777.
- **Cliente:** Entrar em partida (LAN) → IP do host (`127.0.0.1` na mesma máquina) → arma → Conectar.
  Em outra máquina da LAN, use o IP do host e libere a porta UDP 7777 no firewall do Windows do host.

## Teste automatizado com 2 processos (o mesmo usado na validação)
```powershell
$exe = "C:\Users\Chris\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$proj = '"C:\Users\Chris\Documents\GitHub\ratucho\Ratucho.uproject"'
Start-Process $exe -ArgumentList $proj,'/Game/Ratucho/Maps/Levels/L_Cozinha?listen?Bots=5?BotLog=1?AutoRestart=8','-game','-windowed','-ResX=1280','-ResY=720','-ABSLOG="C:\Users\Chris\Documents\GitHub\ratucho\Saved\Logs\Host.log"','-ExecCmds="RatAutoMove 2"'
Start-Process $exe -ArgumentList $proj,'127.0.0.1?Weapon=1?RatName=Cliente','-game','-windowed','-ResX=960','-ResY=540','-ABSLOG="C:\Users\Chris\Documents\GitHub\ratucho\Saved\Logs\Client.log"','-ExecCmds="RatAutoMove 2"'
```
- `RatAutoMove 1|2` (console): o rato anda, corre e pula sozinho. No modo 2 ele também atira.
- `?BotLog=1`: registra no log, a cada 5 s, o objetivo, a posição e a vida de cada bot.
- `?AutoRestart=N`: o host reinicia a rodada N segundos depois do resultado.
- Compare as linhas `[Feed][Server]` do Host.log com `[Feed][Client]` do Client.log. Devem ser os mesmos eventos.

## Comportamento de rede (protótipo)
| Situação | O que acontece |
|---|---|
| Cliente desconecta | O pawn dele é destruído. Se estava com o queijo, o queijo cai onde ele estava ("O portador saiu. O queijo está livre!"). A partida continua. |
| Host sai / volta ao menu | A sessão acaba para todos (Listen Server). Os clientes voltam ao menu com erro de conexão. |
| Cliente entra no meio da rodada | Entra como novo participante, nasce num ponto seguro e vê o estado atual (tempo, queijo, ranking). O feed antigo não é reenviado. |
| Jogar novamente | Só o host pode reiniciar. Os clientes veem "Aguardando o host..." e acompanham o reinício. |

## Validado em 18/11/2026 (2 processos: host + cliente)
- Cliente conectou ao Listen Server (`Welcomed by server`) e carregou a Cozinha.
- Feed idêntico nos dois lados: eliminações, coleta e queda do queijo, vencedor e "Nova rodada".
- Bot eliminou o cliente: o host viu "Queijudo eliminou Cliente!" e o cliente viu "Queijudo eliminou você!".
- Rodadas de 120 s, com vencedor = portador do queijo no fim (Fubá, Parafuso) e reinício automático.
- Exatamente 1 queijo por rodada. Nenhum duplicado (3 rodadas, 3 queijos criados).
- 33 eliminações e 7 trocas de portador em ~2 rodadas: os bots disputam o queijo e se eliminam.
- Escalada da bancada confirmada (o rato em piloto automático subiu de Z 210 até 794, o topo da bancada).
- Zero falhas de spawn depois da correção (pawn nasce acima do ponto, que fica reservado por 1,5 s).

## Ainda não testado
- Teste em LAN entre **dois computadores**. Também ainda não testei se **"Ninguém venceu"** é exibido quando o queijo termina livre: a lógica existe, mas nos testes sempre havia um portador no fim.
- Jogar manualmente (teclado e mouse): tudo foi testado com input automático e bots.
- Um jogador humano usando o botão "Jogar novamente". Testei o reinício pelo `AutoRestart`, que chama a mesma função.
