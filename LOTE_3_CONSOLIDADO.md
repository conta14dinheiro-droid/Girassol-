# Lote 3 — consolidação

## Incluído
- C++20 para hidrologia de reservatórios normalizados, ROS de fogo reduzida inspirada em Rothermel, hazard Gompertz–Makeham, solver Leontief, FABRIK 2D, serialização TLV determinística com CRC32 IEEE, gate Hoeffding/KL e tiering gráfico 2/4/8/12+ GiB.
- Fallback semântico offline determinístico simples para inspeção/observação.
- Testes nativos ampliados e opção CMake ASan/UBSan herdada do projeto.
- Documento matemático `DOCS_LOTE3_OMEGA_E_COGNICAO.md` com hipóteses, equações, argumentos de invariância e limites do que está implementado.

## Não incluído / não alegado
Este lote não implementa os subsistemas V1–V10 completos, acoplamento com agentes do Lote 2, SQLite WAL/sonhos, integração llama.cpp/GGUF, Tsetlin/GBNF, validador real de adaptadores, controlador térmico, Filament/shaders, upscaling ou frame generation. Ainda não há JNI/Kotlin específico para Omega, microscópio/câmera real, áudio procedural ou Silent Vigil. Esses pontos exigem interfaces/dependências e dados de mundo que não existem neste commit. A política gráfica é apenas uma função de recomendação, não benchmark por fabricante. Execução autônoma 24/7 requer desenho Android explícito (WorkManager/serviço foreground conforme política e consentimento); não é garantida por uma biblioteca C++.

## Verificação
Testes definidos no executável `girassol_core_tests`. A execução de build/testes não foi feita neste ambiente de alteração remota. Consulte DOCS para comandos e limites científicos. Recomenda-se executar CI com Clang/GCC, ASan/UBSan e fuzzing de `tlv_decode` antes de usar snapshots externos.

## Transição
Lote 4: GiraOS apenas como OS virtual (VFS, shell, IDE, codecs, navegador offline, instalador e gatilhos). Ferramentas científicas e controles da biosfera continuam exclusivamente na aba Mundo. Estética OLED/Kodak/Dehancer deve ser uma camada visual independente.
