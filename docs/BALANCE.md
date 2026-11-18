# Valores iniciais de balanceamento (protótipo)

Todos os valores são `UPROPERTY` editáveis e podem ser ajustados em Blueprint ou nos Data Assets sem mexer no código. São pontos de partida e serão calibrados nos testes.

## Escala do mundo
| Item | Valor | Justificativa |
|---|---|---|
| Cápsula do rato | raio 30, meia-altura 34 (≈68 cm) | Tamanho confortável para a câmera e a colisão da Unreal. Escalas muito pequenas pioram a física e o near-plane. |
| Escala da cozinha | ~8x a real (bancada ≈ 7,5 m, assento da cadeira ≈ 3,6 m, rodapé ≈ 80 cm) | O rato tem cerca de 1/8 da altura do assento. Assim a cozinha parece gigante. |

## Movimento (`URatMovementComponent`)
| Parâmetro | Valor |
|---|---|
| Caminhada (`MaxWalkSpeed`) | 440 cm/s |
| Corrida (`SprintSpeed`) | 760 cm/s |
| Aceleração andando / correndo | 2600 / 3200 cm/s² |
| Frenagem no chão | 2600 cm/s², atrito 8 |
| Pulo (`JumpZVelocity`) | 640 cm/s → ≈1,3 m de altura com gravidade ×1,6 |
| Gravidade | ×1,6 (pulo mais "seco" e responsivo) |
| Controle aéreo | 0,45 |
| Degrau máximo / rampa máxima | 30 cm / 50° |
| Escalada: velocidade / lateral | 280 cm/s / 70% |
| Topo da escalada (mantle) | 320 para frente, 380 para cima |
| Pulo de parede | 480 de afastamento, 520 para cima |

## Rodada (`ARatuchoGameMode`, Config=Game)
| Parâmetro | Valor | Nota |
|---|---|---|
| Duração | 120 s | `RoundDuration` |
| Preparação | 3 s | Sem tiros e sem queijo, mas já dá para se mover |
| Respawn | 3 s | `RespawnDelay` |
| Invulnerabilidade pós-respawn | 2,5 s | O rato pisca e o HUD mostra "PROTEGIDO" |
| Bots | padrão 5, **máximo 7** | Limite inicial conservador. Medido com 6 bots + 2 processos de jogo: ~50 FPS no editor-game. Revisar com o build empacotado. |
| Distância mínima do respawn a inimigos | 9 m | Também penaliza pontos com linha de visão de inimigos e perto do portador |
| Vida | 100 | `MaxHealth` |

## Armas (`RatuchoDefaults::MakeWeaponStats`, editáveis no componente `Weapon`)
| | Estilingue de Rolhas | Lançador de Tampinhas |
|---|---|---|
| Papel | preciso, média/longa distância | rajada, curta/média distância |
| Dano | 34 por rolha (3 acertos eliminam) | 11 por tampinha × 3 por rajada |
| Cadência | 0,62 s | 0,5 s entre rajadas (0,075 s entre tiros) |
| Pente / recarga | 6 / 1,6 s | 18 (6 rajadas) / 2,0 s |
| Projétil | 3600 cm/s, gravidade 0,35 (arco visível) | 5200 cm/s, gravidade 0,08 (reto) |
| Dispersão | 0,4° | 3,2° |
| Alcance | 45 m | 26 m |
| DPS teórico | ~55 | ~66 de perto, mas cai com a dispersão |

## Queijo (`ACheese`)
| Parâmetro | Valor |
|---|---|
| Alcance para pegar (E) | 1,9 m |
| Bots pegam automaticamente a | 1,1 m |
| Lançamento (G) | 750 cm/s para frente + 420 para cima + metade da velocidade do rato |
| Recoleta por quem soltou | bloqueada por 0,8 s |

## Bots (`RatuchoDefaults::MakeBotRoster`, sorteados sem repetir)
| Nome | Traço | Agressão | Queijo | Oportunismo | Flanco | Foge com | Imprevisível | Arma |
|---|---|---|---|---|---|---|---|---|
| Pipoca | agressivo | 0,95 | 0,35 | 0,3 | 0,1 | 10% | 0,10 | tampinhas |
| Queijudo | caçador de queijo | 0,35 | 1,0 | 0,2 | 0 | 20% | 0,05 | estilingue |
| Fubá | oportunista | 0,6 | 0,55 | 1,0 | 0,2 | 25% | 0,10 | estilingue |
| Bigode | flanqueador | 0,7 | 0,5 | 0,4 | 1,0 | 20% | 0,10 | tampinhas |
| Nina | cautelosa | 0,45 | 0,6 | 0,4 | 0,3 | 50% | 0,05 | estilingue |
| Parafuso | imprevisível | 0,6 | 0,6 | 0,3 | 0,5 | 20% | 0,60 | tampinhas |
| Farofa | equilibrado | 0,55 | 0,55 | 0,5 | 0,3 | 30% | 0,15 | estilingue |
| Tampinha | veloz | 0,65 | 0,7 | 0,3 | 0,6 | 30% | 0,20 | tampinhas |

Percepção: visão de 38 m com cone de 150° e linha de visão, "audição" a 6,5 m, memória de 4 s e tempo de reação de 0,35 s. Também sabem quem os atacou. A posição do queijo é pública, porque o HUD de todo jogador mostra a direção e a distância.

## Câmera (`ARatCharacter`)
| Parâmetro | Valor |
|---|---|
| Distância do braço | 320 cm |
| Offset (ombro direito, acima) | Y 45, Z 55 |
| FOV | 85° |
| Colisão do braço | sonda de 14 cm no canal Camera |
