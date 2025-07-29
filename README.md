# Gerador de Sinal com Frequências Combinadas (Custom Chip - Wokwi)

Este projeto implementa um chip customizado para a plataforma [Wokwi](https://wokwi.com) que gera um sinal de saída baseado na soma de até **5 senoidais** com frequências e amplitude configuráveis.

## 🔧 Funcionamento

O chip utiliza a função `chip_tick()` para atualizar o sinal de saída em cada instante de simulação com base no tempo simulado (`get_sim_nanos()`). Ele gera um sinal do tipo:

```
y(t) = A * (sin(2πf1t) + sin(2πf2t) + ... + sin(2πf5t))
```

Somente as frequências diferentes de zero são consideradas na composição do sinal.

## ⚙️ Parâmetros disponíveis (`chip.json`)

| Nome      | Tipo     | Unidade | Descrição                          |
|-----------|----------|---------|------------------------------------|
| freq1     | float    | Hz      | Frequência da primeira senóide     |
| freq2     | float    | Hz      | Frequência da segunda senóide      |
| freq3     | float    | Hz      | Frequência da terceira senóide     |
| freq4     | float    | Hz      | Frequência da quarta senóide       |
| freq5     | float    | Hz      | Frequência da quinta senóide       |
| amplitude | float    | Volts   | Amplitude comum a todas as senóides|

## 🧠 Exemplo prático

Com os parâmetros:

```json
"freq1": 10,
"freq2": 110,
"freq3": 0,
"freq4": 0,
"freq5": 0,
"amplitude": 1.0
```

O chip irá gerar o sinal:

```
y(t) = sin(2π·10·t) + sin(2π·110·t)
```

## 🔌 Conexão recomendada

- **OUT**: conecte a entrada de outro chip analógico como um filtro (ex: Sallen-Key)
- **Osciloscópio**: visualize a forma de onda gerada e a resposta do filtro

## 📁 Estrutura do projeto

```
/multi_freq_signal_gen
├── chip.json         # Define atributos e pinos
├── signal_gen.c      # Código C do chip
├── chip.svg          # Ícone do chip (opcional)
└── README.md         # Este arquivo
```

## ✅ Compatibilidade

- 100% compatível com `wokwi-chip-clang-action`
- Requer `"clock": "10us"` no `chip.json`
- Utiliza API oficial da Wokwi (`wokwi-api.h`)

---

Desenvolvido para simulação de sinais e testes de filtros digitais/analógicos.