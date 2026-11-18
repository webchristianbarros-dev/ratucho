# RATUCHO — Documento Principal de Conceito e Design
**Versão atualizada: 18 de novembro de 2026**

> **Cheese. Chaos. Survive.**  
> O gato é uma ameaça futura. Os verdadeiros rivais são os outros ratos.

---

## 1. Visão do jogo

**Ratucho** é um jogo multiplayer competitivo de ação em terceira pessoa, ambientado em uma cozinha humana vista na escala de ratos. Os jogadores controlam ratos que disputam um único queijo enquanto exploram o cenário, correm, pulam, escalam objetos, usam armas improvisadas e eliminam os adversários.

A graça está no caos competitivo: roubar o queijo, derrubar quem está carregando, recuperar o queijo, surpreender um rival e tentar sobreviver até o fim da rodada.

O primeiro protótipo será focado em partidas locais contra bots. A arquitetura deve evitar decisões que impeçam uma futura versão multiplayer online, mas não deve tentar implementar rede antes de existir uma partida local divertida e estável.

### Princípios centrais
- Competição individual: cada rato busca vencer por conta própria.
- Um único queijo por partida: o objetivo é claro e cria disputa constante.
- Combate acessível e legível: duas armas iniciais, com funções distintas.
- Movimento divertido: correr, saltar e escalar partes preparadas da cozinha.
- Rodadas rápidas: duração de dois minutos.
- Caos com leitura: nomes dos bots, notificações de eliminações e ranking durante a partida.
- Humor visual e personalidade, sem transformar o jogo em algo infantil ou visualmente genérico.

---

## 2. Fantasia e experiência desejada

O jogador deve sentir que é um rato pequeno em um ambiente humano gigantesco. Uma cadeira vira uma estrutura de escalada; uma bancada é um território perigoso; uma gaveta pode ser uma rota; um pedaço de queijo é um prêmio enorme.

A câmera é em **terceira pessoa**, posicionada atrás e ligeiramente acima do rato, com enquadramento que permita enxergar o personagem, o terreno imediato e os rivais. O controle deve ser responsivo, com leitura clara de saltos, quedas, impactos e combate.

A partida ideal:
1. O jogador abre o jogo.
2. Clica em **Iniciar**.
3. Escolhe o mapa **Cozinha**.
4. Define quantos bots participarão.
5. Escolhe uma das duas armas iniciais.
6. Inicia a rodada de dois minutos.
7. Todos disputam o único queijo, combatem e tentam terminar a partida como vencedores.

O primeiro menu pode ser simples, mas precisa conduzir a essa experiência sem exigir configuração técnica.

---

## 3. Identidade visual e meta gráfica

### Direção visual

A referência aprovada é uma captura de tela que pareça mostrar **o jogo realmente rodando**, não uma ilustração promocional ou uma cena de personagens posando.

Características obrigatórias:
- Câmera em terceira pessoa atrás do rato controlado pelo jogador.
- O rato do jogador visível em primeiro plano, ocupando uma parte útil da tela.
- Outros ratos espalhados pelo cenário, com aparência de jogadores independentes.
- Cozinha ampla, detalhada e visualmente atraente, em escala gigantesca para os ratos.
- Objetos cotidianos — armários, gavetas, cadeiras, pés de mesa, utensílios, embalagens, rodapés, bancadas — transformados em elementos de navegação e cobertura.
- Paleta clara, acolhedora e colorida, com iluminação legível. Evitar imagem escura, enlameada ou excessivamente amarelada.
- Materiais convincentes: madeira, metal, cerâmica, plástico, tecido, queijo e superfícies com variações de rugosidade.
- Personagens estilizados, expressivos e reconhecíveis à distância, com silhuetas distintas.
- Humor visual nas proporções e nos equipamentos, mantendo uma apresentação de jogo competitivo.

### Gato

O gato não faz parte do primeiro protótipo jogável. Ele fica fora do escopo inicial para concentrar o desenvolvimento no combate entre ratos e na disputa pelo queijo. Quando for introduzido, deverá ser claramente um gato quadrúpede, com anatomia e comportamento felinos — nunca um humano fantasiado, nem uma criatura bípede segurando outro gato.

### Qualidade desejada

A meta é aproximar-se da qualidade e da linguagem visual da referência aprovada, dentro do orçamento técnico do projeto. Não se deve reduzir a direção de arte a um visual genérico ou excessivamente simples só porque a máquina de desenvolvimento tem uma GPU antiga.

O projeto deve ser otimizado desde o início para gerar um **executável independente do Editor**. A performance no Editor e a performance do jogo empacotado são métricas diferentes e devem ser medidas separadamente. O executável não carrega o custo integral do Editor, mas continua limitado pelo processamento gráfico, memória de vídeo, CPU e memória do próprio jogo.

### Estratégia técnica de renderização

Hardware de desenvolvimento conhecido:
- GPU: NVIDIA GTX 1050 Ti, 4 GB VRAM.
- CPU: Intel Core i7-7700HQ.
- RAM: 32 GB.
- Motor: Unreal Engine 5.8.4.
- Modelagem: Blender 5.3.

A GTX 1050 Ti é uma limitação a administrar, não motivo para abandonar a qualidade visual. O fato de o usuário conseguir jogar outros títulos em qualidade alta/ultra é um indício útil, mas não garante desempenho idêntico: cada jogo tem uma carga gráfica diferente.

Diretrizes:
- Não depender de Lumen, Ray Tracing ou recursos que exijam GPU moderna.
- Priorizar iluminação baked quando adequada, luzes dinâmicas seletivas e sombras controladas.
- Usar LODs, instanciamento, occlusion culling, colisões simples e limites de materiais.
- Controlar resolução e tamanho de texturas, evitando desperdício de VRAM.
- Usar geometria detalhada onde a câmera realmente percebe; simplificar o que fica distante.
- Criar perfis gráficos (Baixo, Médio, Alto) e testar o executável empacotado.
- Não assumir que “Scalable” significa obrigatoriamente arte simples: qualidade de modelos, materiais e composição pode ser alta mesmo com técnicas de renderização econômicas.
- Medir FPS, uso de VRAM, travamentos e tempos de carregamento em cenas reais.
- Não prometer uma taxa de quadros ou qualidade Ultra antes de medir no executável.

A primeira cozinha deve ser visualmente forte, mas compacta e inteligentemente construída. A meta é obter uma cena bonita e legível, não encher o mapa de objetos sem função.

---

## 4. Plataforma e ferramentas

- Unreal Engine 5.8.4 como motor principal.
- Blueprint pode ser usado para prototipar e estruturar funcionalidades, mas evitar uma arquitetura monolítica.
- Blender 5.3 para modelagem, UVs, materiais e preparação de assets.
- Projeto versionado em Git, com Git LFS para arquivos grandes.
- O projeto local está em `C:\Users\Chris\Documents\GitHub\ratucho\Ratucho\`.
- O repositório está em `C:\Users\Chris\Documents\GitHub\ratucho`.

O Unreal MCP experimental, se usado, é apenas uma ponte opcional de desenvolvimento/editor. O jogo não pode depender dele para funcionar. Antes de qualquer build de distribuição, validar que o projeto empacota e roda sem o MCP/plugin habilitado. O Blender MCP comunitário também é opcional e deve ser avaliado antes de instalar/usar.

---

## 5. Estrutura da partida

### Configuração pré-partida
Tela inicial simples:
- Botão **Iniciar**.
- Seleção de mapa: inicialmente apenas **Cozinha**.
- Quantidade de bots configurável.
- Seleção de uma das duas armas iniciais.
- Botão para começar.

A interface deve deixar claro o que foi selecionado. Não criar menus elaborados, loja, inventário complexo ou progressão nesta fase.

### Duração
- Cada rodada dura **2 minutos**.
- O cronômetro começa quando os participantes podem se mover.
- Mostrar o tempo restante de forma permanente e legível.

### Objetivo: um único queijo
- Existe **exatamente um queijo ativo por partida**.
- No início da rodada, o queijo aparece em um ponto aleatório entre uma lista de locais válidos do mapa.
- Apenas um participante pode portar o queijo por vez.
- O queijo pode ser roubado/retirado de quem o carrega por meio das regras de combate/interação. O portador também pode soltá-lo voluntariamente (tecla G).
- Se o portador morrer, o queijo cai exatamente no local da morte, respeitando uma pequena correção para não ficar preso dentro de geometria.
- O portador pode soltar/lançar voluntariamente o queijo usando uma tecla dedicada (padrão sugerido: G); o queijo cai no ponto em que foi solto/lançado, com uma pequena trajetória física e correção para não atravessar o cenário.
- O queijo não deve reaparecer em outro local enquanto estiver no chão. O reaparecimento aleatório é apenas no início da rodada, salvo se uma regra futura for aprovada.
- O indicador de interface deve mostrar claramente quem está com o queijo. Se estiver no chão, indicar que está livre e, opcionalmente, apontar sua direção.
- Não criar queijo duplicado por erro de respawn, morte ou colisão.

### Condição de vitória
A condição principal é terminar os dois minutos carregando o único queijo.
- Se um participante estiver com o queijo quando o cronômetro chegar a zero, ele vence.
- Se ninguém estiver carregando o queijo no fim, **todos perdem**.
- A interface deve apresentar o vencedor ou a mensagem de que ninguém venceu.
- O ranking de eliminações é uma estatística da rodada; não substitui o objetivo do queijo.

### Morte e respawn
- Ao morrer, o rato é eliminado temporariamente e reaparece após um intervalo curto e configurável.
- O respawn ocorre em um ponto aleatório de uma lista de pontos válidos e seguros do mapa.
- Não reaparecer dentro de paredes, no ar, sobre o queijo, em cima de outro personagem ou diretamente diante de um inimigo.
- A seleção deve tentar manter distância razoável de adversários e do portador do queijo.
- O jogador mantém a arma escolhida.
- Definir e documentar se as eliminações contam para o ranking e como evitar farming de kills. Para o protótipo, cada morte válida do adversário conta uma eliminação; suicídios/queda causada pelo próprio jogador não contam como kill de outro participante.

---

## 6. Combate e armas iniciais

O combate precisa ser funcional, previsível e divertido. Os ratos devem causar dano real, poder eliminar outros ratos e ter feedback visual/sonoro claro. Não basta a arma disparar uma animação sem afetar o adversário.

### Duas armas selecionáveis

**Arma A — Estilingue de rolhas**
- Arma de disparo à distância.
- Dispara pequenas rolhas/projéteis improvisados.
- Deve ter trajetória e velocidade perceptíveis.
- Causa dano ao acertar outro rato.
- Tem cadência moderada e munição/recarga simples.
- Deve ser a opção mais fácil para acertar alvos a média distância.

**Arma B — Lançador de elástico / disparador de tampinhas**
- Arma improvisada de disparo rápido ou rajada curta.
- Deve ter identidade visual e comportamento diferentes do estilingue.
- Causa dano real aos ratos.
- Pode ter menor dano por acerto, maior cadência ou dispersão — calibrar por teste.
- Não deve ser simplesmente uma cópia visual da primeira arma.

As duas armas devem ser equilibradas para que nenhuma seja claramente superior em todas as situações. Expor os parâmetros principais (dano, cadência, alcance, dispersão, tempo de recarga) para ajuste sem reescrever lógica.

### Sistema de dano e eliminação
- Cada rato possui pontos de vida.
- Acertos válidos aplicam dano ao alvo.
- Exibir feedback de impacto: animação, reação, efeito e som.
- Quando a vida chega a zero, o personagem morre.
- Registrar quem causou o último dano e atribuir a eliminação ao atacante correto.
- Ao morrer, soltar o queijo se o personagem for o portador.
- Exibir notificação clara, por exemplo: **“Ratão eliminou Pipoca!”**
- Não atribuir kill a dano ambiental ou a si mesmo.
- Incluir invulnerabilidade curta após respawn para evitar morte instantânea no ponto de reaparecimento; mostrar feedback visual discreto durante esse período.
- Não permitir dano entre o jogador e ele mesmo na primeira versão, salvo se houver razão técnica específica.

### Controles mínimos
- WASD: movimentar.
- Mouse: controlar câmera.
- Botão esquerdo: disparar.
- Espaço: pular.
- Shift: correr.
- Tecla de interação/pegar queijo: definir e exibir na tela (sugestão: E).
- Tab: abrir/fechar ranking da partida.

---

## 7. Movimento e exploração

O rato deve:
- Andar e correr com aceleração e desaceleração agradáveis.
- Pular com altura e duração controláveis.
- Subir/escalar objetos preparados para isso no mapa.
- Descer e cair de superfícies com comportamento consistente.
- Não atravessar objetos sólidos nem ficar preso em colisões.
- Ter animações coerentes de repouso, caminhada, corrida, salto, queda, disparo, dano, morte, respawn e carregamento do queijo.

### Escalada
A escalada é uma parte importante da fantasia de exploração, mas precisa ser implementada de forma controlada:
- Criar superfícies e rotas de escalada explícitas (cadeiras, caixas, prateleiras baixas, puxadores, estruturas de madeira etc.).
- Usar pontos/volumes ou componentes de escalada bem definidos; não tentar permitir que o rato escale qualquer malha arbitrária.
- Permitir transições claras entre chão, escalada, plataforma e salto.
- A câmera não deve atravessar paredes ou ficar presa no cenário.
- O mapa deve ter rotas alternativas e diferentes alturas, sem virar um labirinto confuso.

---

## 8. Bots e nomes

O primeiro protótipo deve ser jogável sem outras pessoas. Bots são obrigatórios desde a primeira versão funcional.

### Configuração
- Quantidade configurável na tela inicial.
- Definir um limite técnico inicial razoável e documentá-lo.
- Bots usam a mesma movimentação, armas, vida, regras de queijo, dano, morte e respawn que o jogador.
- Bots não podem trapacear usando informação que um jogador não teria.

### Personalidade
Criar nomes legíveis e perfis comportamentais variados, por exemplo:
- **Pipoca** — agressivo, procura combate.
- **Queijudo** — prioriza encontrar e carregar o queijo.
- **Fubá** — oportunista, ataca alvos enfraquecidos.
- **Bigode** — tenta flanquear e surpreender.
- **Nina** — cautelosa, evita confrontos quando está com pouca vida.
- **Parafuso** — imprevisível, alterna entre perseguir o queijo e brigar.

Os nomes devem ser atribuídos no início da rodada e permanecer consistentes durante aquela partida. Evitar nomes duplicados na mesma partida.

### Comportamento mínimo dos bots
- Procurar o queijo quando ele estiver livre.
- Perseguir o portador quando tiver oportunidade.
- Tentar pegar o queijo quando estiver ao alcance.
- Atacar adversários dentro do alcance e com linha de visão adequada.
- Fugir ou reposicionar-se quando estiver com pouca vida.
- Usar a arma escolhida e respeitar cadência, alcance e recarga.
- Saltar e percorrer rotas de navegação simples.
- Reaparecer usando as mesmas regras do jogador.
- Reagir à morte, ao respawn e à queda do queijo.
- Não saber a localização de inimigos/queijo se não tiverem percepção ou informação legítima para isso.

O comportamento pode começar simples, mas deve ser modular e depurável. Não implementar uma IA complexa antes de validar que os bots conseguem navegar, encontrar o queijo e lutar.

---

## 9. Interface e feedback da partida

### HUD
Durante a rodada, exibir:
- Cronômetro de dois minutos.
- Estado do queijo: livre ou nome do portador.
- Vida do jogador.
- Arma equipada e indicação de recarga.
- Notificações de eliminações.
- Mensagens curtas de eventos relevantes.

### Feed de eliminações
Mostrar mensagens temporárias e legíveis, por exemplo:
- **“Pipoca eliminou Bigode”**
- **“Fubá pegou o queijo!”**
- **“Queijudo derrubou o queijo!”**
- **“O queijo está livre!”**

Evitar que notificações cubram o centro da tela ou permaneçam indefinidamente.

### Ranking — tecla Tab
Ao pressionar Tab, abrir um painel sobreposto; ao soltar/pressionar novamente, fechar conforme padrão escolhido e documentado.

O ranking deve mostrar:
- Posição.
- Nome do participante.
- Eliminações.
- Estado do queijo (ex.: **COM O QUEIJO**, **SEM QUEIJO**).
- Indicador visual do jogador local.

A classificação pode ordenar primeiro por eliminações, mas deve destacar separadamente o portador do queijo. Não sugerir que mais kills significa vitória automática.

### Fim de rodada
Mostrar:
- Nome do vencedor e motivo: terminou com o queijo.
- Ou: **“Ninguém venceu — o queijo ficou livre quando o tempo acabou.”**
- Ranking final.
- Botão para jogar novamente e voltar ao menu.

---

## 10. Mapa inicial: Cozinha

O primeiro mapa é uma cozinha compacta em conteúdo, mas rica em composição e rotas. Deve parecer enorme na escala dos ratos.

### Elementos de cenário
- Piso e rodapés.
- Armários inferiores e superiores.
- Gavetas e puxadores.
- Bancada.
- Mesa e cadeiras.
- Caixas, potes e embalagens.
- Utensílios de cozinha.
- Migalhas e pequenos objetos.
- Pontos de cobertura e rotas elevadas.
- Áreas de escalada identificadas.
- Pontos de respawn seguros distribuídos pelo mapa.
- Locais candidatos para o surgimento inicial aleatório do queijo.

O cenário deve ser navegável e funcional, não apenas bonito. Colisões, rotas, pontos de salto, pontos de escalada e locais de queijo precisam ser planejados juntos.

### Design de arena
- Várias rotas entre áreas importantes.
- Alturas e atalhos, sem criar locais impossíveis de alcançar.
- Linhas de visão interrompidas por objetos para permitir aproximação e fuga.
- Espaço suficiente para combate, sem corredores tão estreitos que impeçam a câmera.
- Pontos de queijo variados para que as partidas não comecem sempre iguais.

---

## 11. Arquitetura multiplayer desde a primeira versão

**Decisão de arquitetura:** Ratucho não deve ser construído como um jogo exclusivamente single-player para depois ser refeito. Desde o primeiro protótipo, as regras centrais devem funcionar em uma arquitetura cliente-servidor autoritativa, mesmo que a primeira validação seja feita localmente contra bots.

Isso não significa implementar Steam, matchmaking público ou servidores dedicados agora. Significa criar corretamente os fundamentos de rede e validar a partida com mais de um cliente em uma sessão local/LAN antes de considerar a base pronta.

### Modelo inicial recomendado

- Usar **Listen Server** como primeiro modelo online: um jogador cria/hospeda a partida e também joga nela; os demais entram como clientes.
- A primeira versão deve permitir testar localmente com um processo host e pelo menos um cliente adicional, usando os recursos de multiplayer da Unreal.
- O jogo deve continuar sendo testável sozinho: o jogador pode iniciar uma sessão local hospedada por ele e preencher a partida com bots.
- Não exigir servidor dedicado ou infraestrutura paga nesta fase.
- Dedicated Server fica como evolução futura se escala, disponibilidade ou competitividade justificarem.

### Autoridade do servidor

O servidor é a fonte da verdade para:
- início/fim da partida e cronômetro;
- criação e posição inicial do único queijo;
- coleta, posse, soltura, lançamento e queda do queijo;
- vida, dano, acertos válidos, morte e eliminações;
- respawn, escolha de ponto e invulnerabilidade;
- criação e comportamento dos bots;
- resultado final e ranking oficial.

Clientes enviam pedidos/inputs ao servidor. O servidor valida e executa as ações; os clientes recebem o estado replicado. Não confiar no cliente para decidir dano, kills, posse do queijo, tempo ou vencedor.

### Organização sugerida na Unreal

Usar as classes e responsabilidades adequadas à Unreal 5.8.4:
- **GameMode:** regras executadas somente no servidor, fluxo da rodada e validações.
- **GameState:** estado compartilhado e replicado da partida, cronômetro, estado do queijo e informações públicas necessárias.
- **PlayerState:** nome/identidade da partida, eliminações e estatísticas replicadas.
- **Character/Pawn:** movimento de rede, vida, estado de morte, arma e interação.
- **PlayerController:** input local e pedidos de ações ao servidor; não guardar nele dados globais da partida.
- **Weapon/Projectile:** disparos validados pelo servidor, dano aplicado no servidor e apresentação replicada conforme necessário.
- **Cheese Actor/Component:** estado de posse e posição replicados; coleta e soltura validadas no servidor.
- **AIController/Bot systems:** bots criados e controlados no servidor. Clientes recebem a representação replicada dos bots.
- **SpawnManager:** seleção de respawn executada no servidor.
- **HUD/UI:** exibe dados locais e dados replicados; nunca decide regras oficiais.

Não é obrigatório criar uma classe separada para cada item se componentes ou sistemas existentes resolverem melhor. O importante é manter autoridade e responsabilidades claras.

### Regras específicas de replicação

- Habilitar replicação para personagens, movimento, estado relevante das armas/projéteis, queijo e dados públicos da partida.
- Usar o sistema de Character Movement da Unreal e validar movimento em rede; não replicar posição manualmente a cada frame.
- Disparos: cliente solicita disparo; servidor valida cadência, recarga, munição/estado, origem e direção plausíveis; servidor gera/valida o projétil e aplica dano.
- Evitar que todos os efeitos cosméticos precisem ser executados no servidor; separar apresentação local de resultado autoritativo.
- Queijo: nunca permitir duas posses simultâneas. O servidor controla transições entre livre, carregado e solto; clientes apenas solicitam interação.
- Morte do portador: o servidor solta o queijo na posição válida da morte e replica a nova posição/estado.
- Respawn: o servidor escolhe e valida o ponto seguro.
- Bots: nunca criar uma cópia independente da IA em cada cliente.
- Ranking/HUD: consumir PlayerState/GameState replicados, sem manter uma versão divergente em cada máquina.
- Desconexão do host/cliente: tratar de forma controlada e documentar o comportamento esperado para o protótipo.

### Etapas online nesta primeira fase

1. Fazer o jogo funcionar em sessão local hospedada pelo jogador, com bots.
2. Validar a partida em modo de rede com pelo menos dois jogadores reais simulados por processos/clientes locais.
3. Confirmar que ambos veem os mesmos participantes, movimentos, tiros, dano, mortes, respawns, queijo, cronômetro e resultado.
4. Testar que clientes não conseguem assumir posse do queijo ou aplicar dano sem validação do servidor.
5. Só depois integrar a camada Steam.

### Steam e distribuição — etapa posterior, mas prevista

A integração Steam não precisa estar pronta para validar a primeira versão, mas a arquitetura deve permitir adicioná-la sem reescrever gameplay:
- Steam lobbies/sessões para criar e encontrar salas privadas;
- convites de amigos e entrada por convite;
- descoberta/entrada em partidas hospedadas;
- configuração de presença e nome do jogador conforme necessário;
- considerar Steam Networking/Steam Sockets ou a solução recomendada e compatível com a versão da Unreal adotada.

A primeira experiência com amigos pode usar Listen Server. Não prometer conexão universal em qualquer rede antes de testar NAT, firewall, roteador e a solução de transporte escolhida. Para o lançamento, testar o fluxo real de convite e conexão entre redes diferentes.

O jogo não deve depender de Unreal MCP, Blender MCP ou ferramentas de desenvolvimento para funcionar, conectar jogadores ou ser empacotado.

---

---

## 12. Escopo da primeira versão

### Obrigatório
- Executável inicializável.
- Menu simples com Iniciar.
- Seleção do mapa Cozinha.
- Configuração da quantidade de bots.
- Escolha entre duas armas.
- Partida de dois minutos.
- Câmera em terceira pessoa.
- Movimento: andar, correr, pular e escalar rotas preparadas.
- Um único queijo, com spawn aleatório no início.
- Coleta, posse, queda na morte e disputa.
- Duas armas que causam dano real.
- Vida, morte, eliminações e respawn aleatório seguro.
- Bots com nomes únicos e comportamentos distintos.
- HUD, feed de eliminações e ranking com Tab.
- Tela de resultado e opção de jogar novamente.
- Build empacotado testado fora do Editor.
- Perfil gráfico ajustável e medição de desempenho.

### Fora do escopo por enquanto
- Gato e sua IA.
- Multiplayer online funcional.
- Novos mapas.
- Muitas armas ou itens.
- Loja, moedas, cosméticos ou progressão.
- Sistema complexo de inventário.
- Partidas ranqueadas, matchmaking e servidores.
- Cutscenes ou menus elaborados.

---

## 13. Etapas de desenvolvimento e validação

1. **Auditoria do projeto:** verificar versão, estrutura, plugins, configurações, build e estado do repositório. Não apagar ou sobrescrever trabalho existente sem explicar.
2. **Protótipo de movimento:** personagem temporário em terceira pessoa, câmera, corrida, salto e colisões.
3. **Greybox da cozinha:** blocos e plataformas simples, rotas, escalada, pontos de spawn e locais de queijo.
4. **Queijo e regras:** spawn único aleatório, coleta, posse, queda na morte e condição de vitória.
5. **Combate:** duas armas, dano, vida, morte, crédito de eliminação e feedback.
6. **Bots:** nomes, navegação, busca pelo queijo, combate, respawn e perfis.
7. **Interface e fluxo:** menu, configuração, HUD, Tab, feed, resultado e reinício.
8. **Arte:** substituir assets temporários por modelos e materiais de qualidade, mantendo orçamento de desempenho.
9. **Empacotamento e testes:** gerar executável, testar sem Editor/MCP, medir FPS e VRAM, corrigir bugs e validar partidas completas.
10. **Fundamentos de rede:** garantir autoridade do servidor, replicação e separação correta entre GameMode, GameState, PlayerState, PlayerController, personagens, armas, queijo e bots.
11. **Teste multiplayer local:** iniciar uma sessão Listen Server e conectar pelo menos um cliente adicional em processo separado. Validar os fluxos críticos nos dois lados.
12. **Builds e teste entre computadores:** gerar builds de desenvolvimento para host e cliente e testar conexão em LAN. Se possível, testar também entre duas redes diferentes antes de iniciar a integração Steam.

Cada etapa deve terminar com um teste reproduzível antes de avançar. Não construir todo o jogo de uma vez e só descobrir no final que movimento, câmera, bots ou combate não funcionam.

---

## 14. Critérios de aceitação da primeira versão

A versão só pode ser considerada funcional quando:
- O jogador consegue abrir o executável e iniciar uma partida sem abrir o Editor.
- Consegue escolher mapa, número de bots e uma das duas armas.
- A partida começa com exatamente um queijo em local aleatório.
- O jogador consegue andar, correr, pular e escalar as rotas preparadas.
- As duas armas acertam e causam dano em bots.
- Um bot pode eliminar outro bot e o jogador; o jogador pode eliminar bots.
- O feed mostra corretamente quem eliminou quem.
- O queijo é derrubado no local da morte do portador e não duplica.
- O queijo pode ser coletado por outro participante.
- Ao morrer, o participante reaparece em ponto seguro aleatório após o intervalo definido.
- Tab mostra ranking, eliminações e portador do queijo.
- Ao completar dois minutos, vence quem estiver com o queijo; se estiver livre, todos perdem.
- É possível reiniciar a partida sem fechar o executável.
- O build roda de forma estável e o desempenho foi medido na máquina-alvo.
- A partida roda em uma sessão Listen Server, com o host jogando e pelo menos um cliente adicional conectado.
- Host e cliente observam o mesmo estado do queijo, cronômetro, vida, eliminações, mortes, respawns e resultado.
- O servidor valida dano, posse/soltura do queijo, respawn e fim da partida; o cliente não decide esses resultados.
- Bots são controlados no servidor e aparecem corretamente para todos os clientes.
- O projeto mantém uma separação que permita adicionar Steam lobbies/convites depois sem reescrever as regras centrais.

---

## 15. Decisões que devem permanecer configuráveis

Não travar valores de balanceamento sem teste. Expor em variáveis:
- Vida máxima.
- Dano de cada arma.
- Cadência e tempo de recarga.
- Alcance e dispersão.
- Velocidade de caminhada/corrida.
- Força e altura do salto.
- Duração do respawn.
- Tempo de invulnerabilidade após respawn.
- Quantidade máxima de bots.
- Duração da rodada (padrão: 120 segundos).
- Intervalo e regras de interação com queijo.

O desenvolvedor/Claude deve registrar os valores iniciais adotados e justificar decisões que não tenham sido especificadas.

---

## 16. Diretriz final

Prioridade absoluta: entregar um **pequeno jogo de verdade**, com uma partida completa e repetível, e não apenas uma cena bonita ou uma demonstração de tecnologia.

A experiência desejada é simples de explicar e divertida de testar: entrar na cozinha, disputar o único queijo, correr, pular, escalar, atirar, eliminar rivais e descobrir quem termina com o queijo quando o relógio zera.

A qualidade visual deve ser tratada como parte central do projeto, mas sempre acompanhada de validação no executável. Não sacrificar a identidade visual por medo antecipado do hardware; também não ignorar medições reais de desempenho.
