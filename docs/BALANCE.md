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

## Câmera (`ARatCharacter`)
| Parâmetro | Valor |
|---|---|
| Distância do braço | 320 cm |
| Offset (ombro direito, acima) | Y 45, Z 55 |
| FOV | 85° |
| Colisão do braço | sonda de 14 cm no canal Camera |
