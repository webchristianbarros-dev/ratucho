# Perfil de hardware de desenvolvimento

| Componente | Máquina | Impacto |
|---|---|---|
| GPU | NVIDIA GTX 1050 Ti 4 GB (notebook, com Intel HD 630) | Gargalo principal: pouca VRAM e sem ray tracing |
| CPU | i7-7700HQ (4 núcleos / 8 threads) | A compilação de shaders e de C++ fica lenta |
| RAM | 32 GB | OK |
| Disco | C: 776 GB livres | OK (o DDC fica no C:) |

## O que já está configurado (`Config/DefaultEngine.ini`)
- Lumen **desligado** (GI = None, reflexos = SSR). Use luz estática (Lightmass na CPU) e SkyLight.
- Virtual Shadow Maps **desligados**: usa shadow maps clássicos.
- Ray tracing e Mesh Distance Fields desligados.
- Virtual Texturing desligado.
- Anti-aliasing TAA (mais leve que TSR) e motion blur desligado.
- Pool de streaming de texturas em 1200 MB, limitado ao tamanho da VRAM.
- O compilador de shaders reserva 2 threads e roda com prioridade baixa, para o editor não congelar.
- O editor reduz o uso de CPU quando está em segundo plano (útil ao alternar com o Blender).
- DX12 com SM6.

## Faça você mesmo (configurações do Windows)
1. **Forçar a GPU NVIDIA:** Configurações → Sistema → Tela → Gráficos → adicionar
   `C:\Users\Chris\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` e `blender.exe` como **Alto desempenho**.
   Sem isso, o notebook pode rodar a Unreal na Intel HD 630.
2. Deixar o notebook **na tomada** e no modo de energia **Melhor desempenho**.
3. Driver NVIDIA atual: 32.0.15.8278. Mantenha atualizado pelo GeForce Experience ou NVIDIA App.

## No editor
- Escalabilidade do viewport em **Medium** (Settings → Engine Scalability Settings).
- Evite deixar abertos ao mesmo tempo o editor, o PIE e o Blender com cenas pesadas.
- Na primeira abertura, a Unreal compila milhares de shaders e pode levar de 20 a 60 minutos. Isso é normal e não se repete.

## Orçamento de conteúdo recomendado
- Texturas de no máximo **2048×2048** para heróis e 1024 para props, com compressão padrão e mipmaps.
- Personagem principal com até ~30–50k triângulos. Props entre 500 e 5k, com LODs.
- Poucos materiais-mestre, usando Material Instances.
- Evite volumetric fog, muitas luzes dinâmicas com sombra e partículas com GPU sim pesadas.
- Nanite está habilitado no projeto, mas a arte **não deve depender dele**.

## Limitações conhecidas
- Lumen e Nanite com qualidade alta ficam inviáveis para iterar nesta GPU. Se o jogo final precisar de Lumen, isso só poderá ser testado em outra máquina.
- MetaHumans e cenas grandes com World Partition ficam no limite da VRAM.
- Compilar C++ (quando houver) leva vários minutos. Use Live Coding.
