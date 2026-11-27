# Lote 3 — Omega e cognição: escopo implementado e limites

## Escopo e separação

Os novos módulos são exclusivamente da aba **Mundo**. Não adicionam recursos ao GiraOS. A implementação nesta entrega fornece um núcleo C++20 testável para hidrologia de quatro reservatórios, propagação de fogo reduzida, mortalidade Gompertz–Makeham, inversão de Leontief, FABRIK, envelope TLV/CRC32, um gate estatístico MB-v1.0 e tiering gráfico por RAM/temperatura. A interface de observação é separada do sistema operacional.

Este commit não deve ser interpretado como ecossistema completo V1–V10 ou como produto pronto para operação 24/7: os modelos são funções determinísticas de referência e precisam de calibração e integração com o estado do Lote 2. `llama.cpp`, SQLite/WAL, Google Filament, shaders, GiraSuperResolution e GiraFrameGen não são dependências incorporadas aqui; a cognição é um fallback local de regras. Sem essas bibliotecas e sem uma matriz espacial/entidades no modelo, microscopia, câmera, sonhos persistidos, áudio, incêndios de estruturas, agentes e frame generation são interfaces/etapas futuras, não recursos já realizados. O suporte ao hardware é baseado na RAM informada pelo chamador, não em benchmark por fabricante.

## Equações

### Hidrologia e erosão (`GiraStrata`)

Estados normalizados `S=(W_s,W_t,W_b,G,F)` representam água superficial, topsoil, subsolo, lençol e fertilidade. Para precipitação `P`, evapotranspiração `E`, cobertura `C`, inclinação normalizada `z`, infiltrabilidade `k_i` e drenagem `k_d`, o passo calcula fluxos limitados pelo estoque disponível:

`I=min(W_s+dt*P*(0.15+0.65C), dt*k_i*(1-W_t)*(1-0.5z))`

`D_t=min(W_t,dt*k_d W_t)`, `D_b=min(W_b,dt*k_d W_b/2)`, `R=min(W_s+dtP-I,dtP*(0.1+0.8z)*(1-C))`, `ET_t=min(W_t+I,dt E)`.

Atualizações: `W_s'=W_s+dtP-I-R`, `W_t'=W_t+I-D_t-ET_t`, `W_b'=W_b+D_t-D_b`, `G'=G+D_b`. Fertilidade usa uma fonte de serapilheira/umidade e perda erosiva simplificada: `F'=clamp(F+dt*(0.01 W_t C-0.006 z(1-C)),0,1)`. A erosão é um termo fenomenológico agregado, não uma equação calibrada de perda de solo. Reservatórios normalizados não equivalem a balanço dimensional em mm.

### Fogo — redução inspirada em Rothermel

A velocidade de propagação computada é `R=clamp(R0 exp(-3m)(1+0.08 min(U,40)+3 tan²θ)(0.25+0.75C),0,20)`. `m` é umidade relativa do combustível, `U` vento, `θ` inclinação, `C` continuidade de combustível/copa, e `R0` ROS base fornecida. Trata-se de uma redução monotônica inspirada nos fatores de umidade, vento e declive de Rothermel, **não** da equação completa com carga/tamanho de combustível, razão superfície-volume, densidade aparente, número de propagação e calor de ignição. A interface não deduz ignição nem prediz incêndio real.

### Demografia Gompertz–Makeham

Hazard instantâneo anualizado `μ(a)=A+B exp(ca)+h_env` para idade `a≥0`, parâmetros não negativos. A API recebe `B` como escala de Gompertz e implementa `A+B*expm1(ca)+h_env` (o termo `-B` é absorvido em `A` quando se deseja a forma clássica). Para intervalo `Δt` anos, probabilidade de morte é `p=1-exp(-μΔt)`. A função expõe hazard, não aplica nascimento, parentesco nem mortalidade de cidadãos/fauna.

### Economia insumo-produto

`x=(I-A)^(-1)d`, equivalente a `(I-A)x=d`, onde `A` é matriz de coeficientes técnicos e `d` demanda final. O solver usa eliminação de Gauss com pivoteamento parcial e rejeita singularidade e saída negativa. O acoplamento termodinâmico requer colunas de energia/exergia e balanços de estoque, ausentes nesta API; não se afirma conservação de energia na economia simulada.

### FABRIK

Para cadeia de pontos `p_i` e comprimentos invariantes `l_i=||p_{i+1}-p_i||`, FABRIK alterna backward reaching (fixa alvo e recoloca cada junta a distância `l_i`) e forward reaching (fixa raiz e recoloca as juntas). Se alvo excede o alcance total, estende a cadeia em linha reta. O algoritmo preserva comprimentos salvo arredondamento; não implementa limites anatômicos, colisão ou esqueleto 3D.

### Promoção de adaptadores MB-v1.0

Com `n` ensaios independentes Bernoulli, `k` falsos positivos e `p̂=k/n`, limite unilateral conservador Hoeffding: `U=min(1,p̂+sqrt(ln(2/δ)/(2n)))`. O gate exige `U≤α`. Para distribuições Bernoulli de referência `r` e candidata `q`, `D_KL(q||r)=q ln(q/r)+(1-q)ln((1-q)/(1-r))≤κ`. Também são obrigatórios limites de bytes, latência e temperatura, mais permissão explícita do scheduler. Isso só fornece garantia sob amostragem i.i.d. representativa e protocolo de avaliação pré-registrado; não certifica segurança/qualidade geral de um LoRA. O scheduler/controle térmico é uma entrada booleana fornecida, não há controlador Android implementado.

### Tier gráfico

Tiers são determinados por `floor(RAM_bytes/2^30)` em 2/4/8/12+ GiB. Escala base `s={0.65,0.78,0.90,1.00}`, distância `{80,140,240,400}` e densidade `{0.35,0.55,0.78,1}`; em `T≥42°C`, escala/distância/densidade multiplicam por `{0.85,0.8,0.8}`. Frame-generation é elegível para `RAM≥8 GiB` e `T<40°C`; isso é política de capacidade, não geração temporal implementada. Sem limite artificial de 4 GiB; o orçamento efetivo de memória e thermal throttling dependem do Android/SoC.

## Teoremas e argumentos

1. **Invariância hidrológica.** Com entradas finitas normalizadas, `dt∈(0,1]`, cada fluxo de saída é limitado pelo estoque do reservatório e cada estado final é projetado em `[0,1]`. Logo, se `S_n∈[0,1]^5`, então `S_{n+1}∈[0,1]^5`; por indução vale para qualquer número finito de passos. Isso prova invariância numérica, não estabilidade de uma discretização de PDE, conservação de massa (a projeção e capacidade normalizada podem descartar excedente) nem validade ambiental.
2. **Estabilidade térmica limitada do modelo reduzido de fogo.** Para entradas limitadas `m∈[0,1]`, `U≤40`, `|θ|≤1.2`, `C∈[0,1]`, o fator antes do clamp é finito e não negativo; o clamp garante `R∈[0,20]`. Continuidade em entradas interiores decorre de composição de funções contínuas. É limitação de saída, não estabilidade física de um solver de combustão.
3. **Erro de falso positivo.** Pela desigualdade de Hoeffding, `Pr(p>p̂+ε)≤exp(-2nε²)`. Escolhendo `ε=sqrt(ln(2/δ)/(2n))` resulta limite ≤`δ/2` (mais conservador que o alvo unilateral `δ`). Portanto a promoção que exige `U≤α` limita o limite superior de confiança para o protocolo Bernoulli, condicionada às hipóteses de amostra independente/representativa. Não garante contra mudança de distribuição.
4. **Integridade TLV.** O parser valida magic, versão, limites de comprimento, consumo exato do payload e CRC32 antes de aceitar campos. Qualquer corrupção que altere CRC é detectada. CRC32 é detecção acidental, não autenticação; colisões existem e adversário pode recalcular o checksum. Não alegamos prova criptográfica nem recuperação de snapshot.

## TLV + CRC32

Codificação little-endian: magic `GSO3` (4 bytes), versão `u16=1`, contagem `u16`; repetição de `tag:u16, length:u32, value[length]`; CRC32 IEEE refletido sobre todos os bytes anteriores ao CRC (polinômio `0xEDB88320`, init/final XOR `0xffffffff`), armazenado `u32` little-endian. Evoluções incompatíveis devem incrementar versão e preservar decodificador/migração explícitos.

## Testes e execução

Os testes do alvo `girassol_core_tests` exercitam monotonicidade do fogo, dependência etária, solve Leontief, limites de hidrologia, round-trip e corrupção TLV, gate LoRA permissivo/rejeição e tiers 2/4/8/12+, além dos testes climáticos já existentes. Execução recomendada em host Linux/NDK host toolchain:

```sh
cmake -S . -B build -DGIRASSOL_ENABLE_SANITIZERS=ON -DGIRASSOL_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Não foi possível executar compilação/testes neste ambiente remoto de edição. Sanitizers exigem toolchain compatível; ausência de relato de falhas não equivale a teste executado.

## Integrações pendentes de produção

- Conectar tipos de solo, bacias, entidades e fluxos aos estados/eventos existentes e validar unidades/energia/massa.
- Rothermel completo e incêndio em estruturas, demografia/multiespécie, dados de microscópio e câmera em cena real.
- SQLite WAL: gravar snapshots TLV como BLOB em transação e metadados/schema version na base existente; criar tabela episódica/sonhos com retenção e checkpoint consciente. Este commit não altera DB Android.
- Cognição: adapter opcional fixado de `llama.cpp`/GGUF, gramática GBNF, Tsetlin, memória episódica e teoria da mente; fallback atual é determinístico e não gera linguagem livre.
- Filament renderer, shaders, super-resolução temporal/espacial e frame generation; o perfil de recurso sozinho não renderiza.
- JNI da aba Mundo, segurança de referências e exceções, instrumentos microscópicos, modo Silent Vigil e telemetria de câmera.

## Referências

- Rothermel, R. C. (1972), *A Mathematical Model for Predicting Fire Spread in Wildland Fuels*, USDA Forest Service Research Paper INT-115.
- Gompertz, B. (1825), “On the nature of the function expressive of the law of human mortality”, *Philosophical Transactions of the Royal Society* 115, 513–585; Makeham, W. M. (1860), “On the law of mortality and the construction of annuity tables”.
- Leontief, W. (1936), “Quantitative Input and Output Relations in the Economic Systems of the United States”, *Review of Economics and Statistics* 18(3), 105–125.
- FABRIK: Aristidou, A. & Lasenby, J. (2011), “FABRIK: A fast, iterative solver for the Inverse Kinematics problem”, *Graphical Models* 73(5), 243–260.
- Hoeffding, W. (1963), “Probability inequalities for sums of bounded random variables”, *Journal of the American Statistical Association* 58(301), 13–30.
- Cover, T. M. & Thomas, J. A. (2006), *Elements of Information Theory*, 2nd ed. — divergência KL.
- CRC32: ISO/IEC 13239 (HDLC/CRC-32); CRC não é MAC/assinatura.
- Google Filament documentation: https://google.github.io/filament/ ; llama.cpp: https://github.com/ggml-org/llama.cpp ; SQLite WAL: https://sqlite.org/wal.html.

## Transição ao Lote 4

Lote 4 deve permanecer estritamente no domínio GiraOS: VFS/mmap com limites e validação, shell, IDE, codecs, navegador offline, instalador ZIP com verificação de integridade e gatilhos de sistema. Não mover microscopia, ciência, câmera ou manipulação da simulação ao GiraOS. Antes, completar CI host/Android com ASan/UBSan, fuzzing do parser TLV e testes property-based; integrar SQLite WAL/versionamento e dependências offline com versões fixadas. Estética OLED puro, tipografia, Liquid Glass, Kodak 70mm/Dehancer são camada de apresentação separada, sem acoplar filtros visuais ao estado físico.
