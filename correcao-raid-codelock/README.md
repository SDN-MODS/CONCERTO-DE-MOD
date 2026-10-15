# SDN_RaidManager — correção de raid

Fontes do seu gerenciador de raid com proteção de construções, CodeLock e integração opcional com BreachingCharge **no mesmo PBO**. Não é necessário complemento separado. Compilação e testes no DayZ ainda estão pendentes.

## Instalação

1. Faça backup do PBO e do JSON atuais.
2. Empacote a pasta `SDN_RaidManager/` com prefixo `SDN_RaidManager`, mantendo `config.cpp` e `scripts/` na raiz interna do PBO. O arquivo `SDN_RaidManager-corrigido-fontes.zip` contém as fontes, não um PBO pronto.
3. Substitua o PBO principal inteiro. Atualize clientes e servidor com a mesma versão: o formato do RPC de estado foi ampliado para incluir a opção do BreachingCharge.
4. Remova o antigo complemento `SDN_RaidManager_BreachingCharge`, caso instalado, tanto dos PBOs quanto da lista de mods de clientes/servidor. Não carregue duas cópias do gerenciador ou dos overrides.
5. Carregue o gerenciador via `-mod` nos clientes e no servidor. Sem BreachingCharge, não há dependência obrigatória dele. Com BreachingCharge/CodeLock, carregue-os antes do gerenciador, mantendo as demais dependências do seu servidor. Exemplo: `-mod="@CodeLock;@BreachingCharge;@SDN_RaidManager"`.
6. Preserve os horários e outras configurações do JSON `$profile:SDN_MODS/SDN_RaidManager/SDN_RaidManagerConfig.json`.

## Opção do BreachingCharge no JSON

Adicione ao objeto principal do JSON existente, ajustando as vírgulas e preservando os demais campos:

```json
"BlockBreachingChargeOutsideRaid": false
```

Esse trecho é um campo, não um arquivo JSON completo.

| Configuração | Comportamento |
| --- | --- |
| BreachingCharge ausente, opção falsa ou verdadeira | Integração não é compilada; o gerenciador não referencia as classes ausentes |
| BreachingCharge instalado, campo ausente ou `false` | Gerenciador não restringe o plantio; condições originais do BreachingCharge continuam valendo |
| BreachingCharge instalado, campo `true`, fora do raid | Bloqueia opção, continuidade e conclusão do plantio |
| BreachingCharge instalado, campo `true`, raid ativo | Delega às condições e efeitos originais do BreachingCharge |

O padrão no código é **`false`**. Em JSON antigo sem o campo, o objeto recém-carregado conserva esse padrão. A adição não exige recriar o arquivo, nem altera automaticamente as configurações existentes. A opção desativada desliga somente o bloqueio específico de plantio; não desliga a proteção das construções ou do CodeLock.

A configuração é carregada no servidor e sincronizada com o cliente pelo RPC existente, junto com o estado de raid. Mudanças são aplicadas no ciclo de recarga `AutoReloadSeconds` (padrão de 15 segundos), seguido da atualização dos clientes. Antes da sincronização inicial, a opção do cliente também começa em `false`; a conclusão é sempre revalidada pelo estado do servidor.

## Implementação

### BreachingCharge

A fonte enviada confirma `#define BREACHINGCHARGE` em `scripts/Common/Define.c`. Todo `BreachingChargeRaidBlocker.c`, incluindo a referência a `HDSN_ActionDeployBreachingCharge`, está dentro de `#ifdef BREACHINGCHARGE`. O JSON controla o comportamento em execução; a macro protege a compilação quando o mod está ausente. Não foi adicionado um addon obrigatório ao `config.cpp`.

A ação completa enviada aplica o plantio em `OnFinishProgressServer`, incluindo posicionamento e armamento automático. A integração verifica `CanDeployBreachingCharge()` antes de chamar esse método. Também verifica `ActionCondition`, `ActionConditionContinue` e `OnFinishProgressClient` (predição de posicionamento). Os callbacks originais de encerramento/cancelamento permanecem disponíveis para a limpeza da ação interrompida.

### CodeLock

As classes corretas são `ActionDestroyCodeLockOnFence` e `ActionDestroyCodeLockOnTent`, confirmadas nas fontes enviadas. Elas usam `CAContinuousRepeat` e aplicam HP diretamente em `OnFinishProgressServer`. `CodeLockRaidBlocker.c` esconde as ações fora do raid e revalida no servidor antes de cada ciclo de dano. Durante o raid, mantém as condições e efeitos originais. A integração é condicionada à macro `CodeLock`, presente nas fontes enviadas.

No JSON do CodeLock, `CanRaidGates`/`CanRaidTents` precisam permitir raid para as ações correspondentes aparecerem. Na versão enviada esses campos são strings (`"true"`). O gerenciador não força permissões que o próprio CodeLock proíbe.

### Construções e explosivos nativos

`BuildingDamageBlocker.c` recusa dano em `EEOnDamageCalculated` fora do raid, sem distinguir tiros e explosões, para estruturas derivadas de `BaseBuildingBase`. Reaplica `SetAllowDamage` depois de construir e carregar, além das atualizações periódicas. O DayZ reativa o dano durante `Construction.BuildPartServer`, daí a necessidade dessa reaplicação.

`ConstructionRaidBlocker.c` também protege a entrada de `DestroyPartServer` antes da remoção de materiais/partes, fora do raid. A desmontagem por `DismantlePartServer` não foi modificada; rotinas de limpeza que usam destruição de partes também ficam bloqueadas fora do horário.

## Limites

- Impedir o plantio não desarma cargas já plantadas. A detonação posterior de BreachingCharge deve ser testada separadamente.
- Mods que apagam objetos diretamente, alteram HP sem os callbacks ou usam outras classes de construção podem exigir integração própria. Bloquear a destruição de partes não restaura HP já retirado diretamente por outro script.
- A atualização visual depende da recarga e do RPC; o servidor consulta seu próprio horário/configuração para autorizar o efeito.
- A segurança sem o BreachingCharge depende da macro original: não a defina manualmente nem distribua cópias soltas do `Define.c` do outro mod.

## Verificações realizadas

Conferidos a macro real, a classe e os quatro métodos de integração na fonte completa do BreachingCharge. Pré-processamento do arquivo de integração verificado com a macro presente e ausente: sem a macro, nenhuma classe/tipo do BreachingCharge é emitido; com ela, os overrides estão presentes. Conferidos a escrita/leitura dos dois campos do RPC, o padrão falso e o conteúdo do ZIP de entrega.

Essas verificações são estáticas/pré-processamento. **Não foram executados compilação Enforce Script, empacotamento PBO, inicialização nem testes no jogo.**

## Testes necessários no DayZ

1. Iniciar sem BreachingCharge, com a opção falsa e verdadeira: não deve ocorrer erro de tipo ausente.
2. Com BreachingCharge e campo ausente/`false`: verificar plantio original dentro e fora do raid.
3. Com BreachingCharge e campo `true`: fora do raid, a opção fica ausente e não há plantio; durante o raid, funciona conforme o mod original.
4. Encerrar o raid durante o plantio: não concluir nem consumir indevidamente a carga; conferir retorno às mãos e limpeza da projeção.
5. Alterar o JSON com jogadores conectados: verificar recarga e sincronização; reconectar durante raid ativo/inativo.
6. Testar a serra contra CodeLock em portão/tenda, dentro e fora do raid, inclusive encerrando o horário durante a ação.
7. Comparar HP/partes antes e depois de tiros, granadas e explosivo plástico nativos, dentro/fora do raid, antes/depois de construir e reiniciar.
8. Detonar fora do raid uma carga plantada antes e repetir com estruturas de mods, registrando qualquer caminho que ainda ignore a proteção.

## Referências preservadas

`original/SDN_RaidManager.zip` e `original/deploybc.c` preservam os primeiros uploads. O pacote completo de terceiros foi usado como referência, sem redistribuir seus scripts nesta entrega.

SHA-256 do `HDSN_BreachingCharge.zip` de referência: `619563f2de3cb638c7344bd2ec8e62ba9ffcc47b6e143d71235a278015f0208a`.
