# Lote 2 — consolidação técnica e transição ao Lote 3

## Escopo entregue

O núcleo implementa um passo determinístico agregado para clima, balanço hídrico, solo e cobertura florestal; seis guildas ecológicas funcionais; risco de exposição por contato/água/solo/vetor; impulsos de fome, sede e curiosidade; recuperação imune e suporte de item terapêutico abstrato. O módulo Android oferece fronteira JNI com captura de exceções, SQLiteOpenHelper com WAL e visualização Compose. O fallback de diálogo é local e determinístico.

**Não implementado neste lote:** integrações reais com `llama.cpp`/GGUF; TTS neural ou pacote de voz; serviços 24/7; agente individual completo; caça e processamento de fauna; receitas de armas/remédios; construção faseada; namoro; catálogo exaustivo de espécies; climatologia espacial ou calibração por bioma. Estes itens permanecem requisitos de produto, não capacidades já entregues.

## Modelo e equações

As grandezas do motor estão normalizadas no intervalo [0,1], exceto temperatura (°C), pressão (hPa), vento (m/s), horas e população agregada (unidades de densidade). `dt` é limitado a 24h. `clamp(x)` denota projeção em [0,1].

- Temperatura: `T' = clamp_C(T + (T_eq - T) * (1-exp(-dt/36)))`, onde `T_eq = 24 + 7 sin(2π(day_fraction - 0.25)) - 3 cloud`.
- Umidade relativa agregada: `H' = clamp(H + 0.08(P-E) - 0.025 max(T-25,0))`; a implementação usa o mesmo sinal físico: chuva eleva estoque de água e evapotranspiração reduz umidade do solo. Umidade atmosférica é mantida em [0,1] como variável fenomenológica, não como razão de mistura termodinâmica.
- Pressão e vento são aproximações suaves: pressão acompanha um ciclo barométrico de pequena amplitude; vento relaxa ao alvo determinístico de clima local. Não resolvem Navier–Stokes.
- Precipitação: `P = clamp(0.08 + 0.20*cloud + 0.08*humidity - 0.10*wind)` por dia normalizado. Evapotranspiração: `ET = clamp(0.02 + 0.09*max(T,0)/40 + 0.08*canopy + 0.02*wind)`.
- Água superficial: `W' = clamp(W + P*infiltration - ET*(0.35+0.65*canopy) - runoff)`. Umidade do solo recebe infiltração e perde ET; decomposição e cobertura alteram fertilidade/canopy, que realimentam ET e habitat.
- Guildas: atualização logística com capacidade de suporte derivada de cobertura/água; herbívoros dependem de plantas; predadores de presas; polinizadores de flores/canopy; decompositores de detrito; vetores respondem a água parada. A versão atual usa taxa agregada pequena por passo e não conserva indivíduos entre guildas.
- Risco de exposição: `p = clamp(1-exp(-λ*dt*(contact + water*W + soil*(1-fertility) + vector*vectors)))`; exposição reduz susceptíveis e eleva infectados. Recuperação transfere infectados a recuperados; imunidade reduz susceptibilidade. Não representa linhagens, incubação, mutação ou resistência.
- Impulsos: fome e sede crescem por hora; água e alimento disponíveis reduzem suas taxas; curiosidade cresce com bem-estar e cai com exploração abstrata. Necessidades são normalizadas, não kcal ou litros por pessoa.

### Invariantes e lema de estabilidade do passo

1. `clamp` projeta todos os reservatórios e variáveis de estado em [0,1]; densidades e contagens são não negativas.
2. Para entrada finita válida e `dt ∈ (0,24]`, o passo produz componentes finitas; validação rejeita estado não finito antes de atualizar.
3. Por indução no número de passos, os intervalos permanecem invariantes: o passo aplica projeção a cada atualização de reservatório/densidade e as transições epidemiológicas limitam transferências ao estoque disponível.

Isto é um lema de invariância do código, não uma afirmação de estabilidade ecológica ou verdade empírica. Não há conservação exata de energia/massa porque o modelo é agregado e usa fluxos fenomenológicos.

## Testes

`native/tests/world_tests.cpp` verifica estado inicial de mata sem população humana, rejeição de passos inválidos, repetibilidade do mesmo estado/tempo, limites dos reservatórios, acoplamento após chuva/tempo e evolução de necessidades/doença. Execute com CMake/CTest e ASan/UBSan; sanitizadores só podem ser habilitados quando o toolchain host os suporta. Não se afirma que os testes foram executados neste ambiente remoto.

## Persistência, JNI e inferência

A classe `WorldDatabase` ativa WAL no SQLite Android, guarda snapshots como JSON com timestamp e remove versões antigas por retenção configurável. JNI captura exceções C++ e não as deixa atravessar a ABI. O fallback de fala é texto local determinístico; nenhuma chamada de rede é feita. Não há backend llama.cpp ligado: o próximo passo deve adicionar uma versão upstream fixada, checagem de licença, adapter e seleção de contexto conforme memória disponível reportada pelo sistema, sem limite fixo artificial de 4 GB. Modelos GGUF são escolhidos pelo usuário e por política de memória/armazenamento; nunca carregue modelo inteiro sem orçamento.

## Referências científicas e técnicas

- Allen, R. G. et al. (1998), *Crop Evapotranspiration: Guidelines for Computing Crop Water Requirements*, FAO Irrigation and Drainage Paper 56 — referência para evapotranspiração de Penman–Monteith; não implementada integralmente aqui.
- Thornthwaite, C. W. (1948), “An Approach toward a Rational Classification of Climate”, *Geographical Review*, 38(1), 55–94 — contexto histórico de índices climáticos agregados.
- Begon, M., Townsend, C. R., Harper, J. L. (2006), *Ecology: From Individuals to Ecosystems*, 4th ed. — interações ecológicas e capacidade de suporte.
- Anderson, R. M. & May, R. M. (1991), *Infectious Diseases of Humans: Dynamics and Control* — enquadramento de modelos compartimentais, não calibração do modelo do jogo.
- SQLite Documentation, “Write-Ahead Logging” — https://www.sqlite.org/wal.html.
- Android Developers, “JNI tips” — https://developer.android.com/training/articles/perf-jni.
- ISO/IEC 14882:2020 — linguagem C++20.

## Guia de transição para Lote 3

1. Definir formato versionado de mundo e migrações SQL; manter snapshot atomicamente associado a `schema_version` e controlar checkpoint WAL.
2. Introduzir testes golden e property-based para balanços e cenários climáticos; calibrar parâmetros por bioma e registrar incertezas.
3. Implementar subsistemas de agentes e sociedade como comandos/eventos determinísticos, com inventário e trilha de auditoria; separar conteúdo cultural de mecânica.
4. Adicionar `llama.cpp` como dependência upstream fixada e opcional, execução offline, cancelamento e limites calculados do orçamento de memória reportado; nunca confundir fallback textual com inferência.
5. Definir política Android de execução em background, persistência resiliente, notificações e consentimento; não prometer execução contínua contra as regras do sistema.
6. Só então expandir GiraOS: `GirassolVFS`/mmap com limites e validação, `GirassolShell`, `GiraIdeSuite`, codecs offline, navegador offline, instalador ZIP com verificação de integridade e `GiraTriggerEngine`. LoRA/workflows de CI exigem dataset, avaliação, proveniência e controles de segurança próprios.
