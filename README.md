# Girassol v3.1.0 — fundação do Lote 2

Este repositório inicia o Lote 2 sobre a branch `copilot/lote-2-biosphere-engine`. O motor nativo implementa uma simulação determinística, discreta e acoplada de clima/água/solo/mata, grupos funcionais de fauna, exposição a patógenos e necessidades dos cidadãos. O esqueleto Android inclui JNI protegido, persistência SQLite em WAL e tela Compose da aba Mundo.

## Limites científicos e de escopo

O motor é um **modelo educacional agregado**, não uma previsão meteorológica, recomendação médica, modelo epidemiológico calibrado nem representação detalhada de todas as espécies. As variáveis ambientais são normalizadas para intervalos documentados no código. Infecção é uma dinâmica probabilística determinística por passo, sem agentes patogênicos reais, e “remédios” são itens abstratos de jogo, sem receita ou instrução médica. O catálogo de guildas é extensível; não significa simular literalmente todos os táxons.

O projeto ainda não inclui síntese de voz embutida, modelo GGUF/llama.cpp vendorizado, mapa, motor 24/7 de serviço Android, persistência de migrações em produção, nem os sistemas completos de caça/construção/romance descritos na visão. A interface de diálogo tem fallback determinístico e acessibilidade por texto; a integração com TTS do Android requer configuração de voz PT-BR no dispositivo. Isso evita alegar funcionalidades ou dependências que não estão presentes.

## Estrutura

```text
native/include/girassol/world.hpp   Estado e contrato público do motor
native/src/world.cpp                Atualização acoplada e serialização
native/src/jni_bridge.cpp           Guardas de exceção e fronteira JNI
native/tests/world_tests.cpp        Testes determinísticos e invariantes
app/src/main/...                    Activity Compose, SQLite WAL, JNI e telas Mundo
LOTE_2_CONSOLIDADO.md               Equações, premissas, testes e trilha do Lote 3
```

## Construção e testes nativos

```sh
cmake -S . -B build -DGIRASSOL_ENABLE_SANITIZERS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Para Android, abra o projeto no Android Studio com Android SDK/NDK instalados e sincronize o Gradle. A biblioteca nativa é construída pelo CMake do módulo `app`. A execução contínua em background não é prometida: Android pode suspender processos; um produto 24/7 precisa de política explícita de bateria e mecanismo permitido pela plataforma.

## Estado inicial

`WorldState::pristine()` cria solo, água e mata nativa sem cidadãos ou animais domesticados; a fauna inicia em densidades agregadas ambientais. O avanço é explícito via `step(hours)`, limitado a 24h por chamada para evitar passos inválidos. Para uma simulação contínua, o chamador deve agendar passos e persistir snapshots.
