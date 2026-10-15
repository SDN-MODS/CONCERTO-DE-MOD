# Correção do SDN_RaidManager para CodeLock

A correção foi implementada nos scripts do gerenciador de raid existente, com base nos scripts do CodeLock fornecidos pelo usuário (Workshop [1646187754](https://steamcommunity.com/sharedfiles/filedetails/?id=1646187754)). Ainda é necessário compilar/empacotar e validar no DayZ: este ambiente não tem o runtime nem as ferramentas do jogo.

## O que foi corrigido

O bloqueio original tratava `ActionDestroyCombinationLock`, a fechadura numérica padrão do DayZ. O bloco comentado mencionava `ActionDestroyCodeLock`, mas os scripts enviados usam **`ActionDestroyCodeLockOnFence`** e **`ActionDestroyCodeLockOnTent`**.

Essas duas ações usam `CAContinuousRepeat` e retiram HP diretamente com `AddHealth` em `OnFinishProgressServer`. Bloquear apenas o dano da construção não impede essa operação.

O novo `CodeLockRaidBlocker.c` modifica as duas classes corretas:

- Raid fora do horário: `ActionCondition` retorna `false`, ocultando a ação no cliente.
- Raid ativo: a condição original do CodeLock é executada, preservando as restrições de portão/tenda e o cálculo de HP.
- Antes de cada ciclo de dano no servidor: o horário é consultado novamente. Fora do raid, o método original não é chamado; logo, esse ciclo não retira HP, não solta/apaga a fechadura e não desgasta a ferramenta.
- O fim do horário bloqueia também uma ação que já estava em andamento.

A integração usa `#ifdef CodeLock`, macro confirmada em `scripts/Common/Common.c` do CodeLock enviado. Não modifica nem redistribui os scripts do CodeLock.

`CanDestroyRaidLock()` centraliza a decisão: servidor usa a configuração/horário; cliente usa o estado recebido por RPC. Sem estado válido, a ação fica bloqueada. A sincronização na entrada e na mudança de estado foi mantida; um reenvio a cada `AutoReloadSeconds` cobre clientes que ainda não estavam prontos para receber a mensagem inicial. A interface acompanha esse intervalo (padrão de 15 segundos); o servidor verifica o horário em cada ciclo de dano. Alterações do JSON dependem da recarga desse arquivo.

## Arquivos

- `SDN_RaidManager/`: fontes do gerenciador principal, sem dependência do BreachingCharge.
- `opcionais/SDN_RaidManager_BreachingCharge/`: complemento separado, apenas para servidores com BreachingCharge.
- `original/SDN_RaidManager.zip`: upload original, preservado sem alterações.
- `SDN_RaidManager-corrigido-fontes.zip`: fontes do gerenciador principal.
- `SDN_RaidManager-BreachingCharge-opcional-fontes.zip`: fontes do complemento opcional.
- Os dois ZIPs são fontes; **não são PBOs prontos**.

Arquivos alterados em relação ao gerenciador original:

1. `scripts/3_Game/SDN_RaidManager/SDN_RaidManagerManager.c`: permissão compartilhada e reenvio periódico do estado.
2. `scripts/4_World/SDN_RaidManager/CodeLockRaidBlocker.c`: novo bloqueio para portões e tendas.
3. `scripts/4_World/SDN_RaidManager/LockDamageBlocker.c`: mantém a proteção da fechadura padrão e revalida antes do efeito.
4. `scripts/4_World/SDN_RaidManager/PlayerBase.c`: recebe o estado de raid somente no cliente.

## Como instalar

1. Faça backup do PBO instalado e da configuração atual. Use a pasta `SDN_RaidManager` desta entrega como fonte para atualizar o seu mod existente. Não carregue duas cópias do gerenciador.
2. Empacote a pasta com o Addon Builder/DayZ Tools, com prefixo **`SDN_RaidManager`**. Dentro do PBO devem estar `config.cpp` e `scripts/`, correspondendo aos caminhos já existentes em `config.cpp`.
3. Substitua o PBO do gerenciador e assine/distribua a atualização conforme o procedimento habitual do seu servidor. O CodeLock original permanece instalado.
4. Carregue o gerenciador **nos clientes e no servidor em `-mod`**, após o CodeLock. Exemplo de trecho da lista, mantendo as outras dependências do seu servidor: `-mod="@CodeLock;@SDN_RaidManager"`. Usar somente `-serverMod` não oculta a opção no cliente. Todos precisam receber a mesma versão atualizada.
5. Preserve o JSON `$profile:SDN_MODS/SDN_RaidManager/SDN_RaidManagerConfig.json`. Não é necessário recriar horários. No JSON do próprio CodeLock, `CanRaidGates` precisa permitir raid (`"true"`) para a ação aparecer no horário ativo; para tendas, vale `CanRaidTents`. Essas opções são strings na versão enviada. A correção respeita as restrições do CodeLock, sem forçá-las a permitir raid.
6. Reinicie servidor/clientes e confira os logs de compilação antes de testar.

## Validação necessária no jogo

| Situação | Resultado esperado |
| --- | --- |
| Raid desativado; serra apontada para CodeLock no portão | Ação “Destroy Codelock” ausente; HP preservado |
| Raid ativo e `CanRaidGates` permitindo raid | Ação aparece e retira HP nos incrementos originais |
| Raid termina durante a serragem | Nenhum novo ciclo de dano é aplicado após o fim do horário |
| Raid ativo, mas CodeLock proíbe raid no alvo | Ação continua indisponível |
| CodeLock em tenda | Mesmas regras, respeitando `CanRaidTents` |
| Jogador entra durante raid ativo | Recebe o estado na entrada; reenvio periódico cobre sincronização inicial perdida |
| Jogadores conectados durante mudança de horário | Interface atualiza com o RPC, conforme intervalo de recarga |
| Abrir porta/gerenciar fechadura | Comportamento original preservado |

## Verificações realizadas aqui

Comparadas as classes, assinaturas dos métodos, macro e pontos de aplicação de dano com `scripts.zip` fornecido. Confirmado que os overrides delegam às condições e efeitos originais apenas depois da permissão de raid. O ZIP de fontes foi conferido contra a pasta da entrega, e o original do gerenciador foi preservado byte a byte.

Não foram executados compilação Enforce Script, empacotamento PBO nem testes de comportamento no DayZ. A correção de código está implementada; a validação no jogo permanece pendente.

SHA-256 do `scripts.zip` de referência: `8aa7c233d8de6f69f527d6c10f7240c17d981bf3179059c6b45a095ae9e13e15`.

## Reforço da proteção contra explosivos

Após relato de destruição por explosivos fora do raid, foram adicionadas estas proteções:

- Reaplicar `SetAllowDamage` após construir uma parte e após carregar a estrutura persistida. O `Construction.BuildPartServer` padrão reativa o dano durante a construção.
- Reaplicar a permissão de dano das estruturas registradas em cada verificação periódica, mesmo sem mudança de horário.
- Bloquear dano nos callbacks existentes também quando a configuração ainda não está disponível.
- Novo `ConstructionRaidBlocker.c`: negar `DestroyPartServer` fora do raid antes da remoção de materiais e das partes. A desmontagem por `DismantlePartServer` não foi alterada. Rotinas de limpeza que usam destruição de partes também ficam bloqueadas fora do raid.

As assinaturas e a ordem dos eventos foram conferidas nas fontes públicas do DayZ: `BohemiaInteractive/DayZ-Script-Diff`, arquivos `scripts/4_world/classes/basebuilding/construction.c` e `scripts/4_world/entities/itembase/basebuildingbase.c`. Isso confirma os pontos de integração, não o comportamento no servidor do usuário.

**Diagnóstico do explosivo ainda pendente:** é necessário informar o explosivo e a classe/mod da estrutura. Mods que chamam `Delete`, alteram HP diretamente ou usam construções que não derivam de `BaseBuildingBase`/`Construction` podem exigir integração específica. O bloqueio de `DestroyPartServer` não restaura HP que outro script já tenha retirado diretamente.

Testar: tiros e explosões fora/dentro do raid, estrutura recém-construída, após reinício e mudança de horário; conferir HP, partes, anexos e materiais. Repetir os testes do CodeLock. Compilação e testes no jogo permanecem pendentes.

## Bloqueio de colocação do BreachingCharge

O complemento em `opcionais/SDN_RaidManager_BreachingCharge/` contém `BreachingChargeRaidBlocker.c`, que modifica a classe `HDSN_ActionDeployBreachingCharge`, confirmada no `deploybc.c` enviado, preservado em `original/deploybc.c`.

- Fora do raid, a condição da ação retorna falso e a opção não é disponibilizada.
- Uma colocação em andamento deixa de atender à condição de continuidade quando o raid termina.
- O callback padrão `OnFinishProgressServer` também revalida a permissão antes de delegar ao mod original.
- Durante o raid, todas as condições originais continuam valendo. Não se usa `IsDestructionUN()`: essa função pertence ao trecho enviado, e não foi definida nas fontes do gerenciador.

### Instalação opcional

O gerenciador principal **não contém nenhuma referência à classe do BreachingCharge**. Pode ser empacotado e carregado sozinho, com ou sem esse mod instalado. O bloqueio de CodeLock e a proteção das estruturas permanecem no gerenciador principal.

- **Sem BreachingCharge:** instale somente o PBO principal. Não instale/carregue o complemento opcional.
- **Com BreachingCharge e desejando bloquear o plantio fora do raid:** empacote também `opcionais/SDN_RaidManager_BreachingCharge/`, com prefixo `SDN_RaidManager_BreachingCharge`. Carregue esse complemento no cliente e no servidor, depois do BreachingCharge e do gerenciador. Exemplo: `-mod="@CodeLock;@BreachingCharge;@SDN_RaidManager;@SDN_RaidManager_BreachingCharge"`.
- **Com BreachingCharge, mas sem o complemento:** o gerenciador funciona, porém o bloqueio específico de plantio não fica ativo.

O complemento exige a classe `HDSN_ActionDeployBreachingCharge`. Se o BreachingCharge for removido, remova também o complemento da lista de mods/PBOs carregados. Ele tem `config.cpp` e caminho de scripts próprios, fora dos módulos do gerenciador principal. `requiredAddons` declara a dependência confirmada `SDN_RaidManager`; o `CfgPatches` do BreachingCharge não foi fornecido, portanto não foi inventado um nome de addon para ele.

**Atualização da versão anterior:** recompile e substitua o PBO principal inteiro com as fontes atuais. Confira que ele não contém mais `BreachingChargeRaidBlocker.c`. Apenas acrescentar o novo PBO opcional e manter o principal antigo não remove a antiga dependência e pode duplicar os overrides.


O arquivo enviado é uma adaptação da ação, não sua implementação completa. Foram conferidas as assinaturas herdadas de `ActionBase`/`ActionContinuousBase` nas fontes públicas do DayZ. Ainda é preciso testar a versão instalada para confirmar que o plantio ocorre pelo callback padrão, sem outros efeitos em callbacks não fornecidos.

### Explosivos nativos e limites

A proteção de `BaseBuildingBase` já recusa todo dano em `EEOnDamageCalculated` fora do raid, sem filtro por `damageType`, arma ou fonte. Isso inclui dano nativo de explosão que passa por esse callback; não se desativam explosões nem o dano a jogadores. `SetAllowDamage` e o bloqueio de `Construction.DestroyPartServer` continuam como camadas adicionais. Não foram adicionadas exceções específicas para granadas.

O bloqueio de colocação não desarma cargas plantadas durante o raid. Se elas detonarem depois do encerramento, o mod de explosivo pode usar destruição direta que exige integração específica. Estruturas de mods que não usam as classes protegidas também exigem revisão própria.

### Testes adicionais no servidor

1. Fora do raid, tentar plantar a carga: a ação deve estar ausente e nenhuma carga deve ser posicionada/consumida.
2. Durante o raid, concluir o plantio normalmente.
3. Iniciar o plantio próximo do fim do raid: após encerrar o horário, não deve concluir nem consumir a carga.
4. Testar granada e explosivo plástico nativos contra cerca/torre padrão, comparando HP e partes antes/depois, fora e dentro do raid.
5. Repetir em estrutura recém-construída e após reinício; testar separadamente estruturas de mods.
6. Detonar fora do raid uma carga plantada antes: registrar o resultado para identificar eventual necessidade de proteger o código de detonação do BreachingCharge.

Estes testes e a compilação no DayZ ainda não foram executados neste ambiente.

## Verificação da separação opcional

Conferido que o gerenciador principal e seu ZIP não contêm `HDSN_ActionDeployBreachingCharge`, nem `BreachingChargeRaidBlocker.c`. O script da integração aparece somente no complemento e no ZIP opcional. Conferidos os caminhos de `CfgMods`, a dependência do gerenciador e a integridade dos dois arquivos ZIP.

Validar no DayZ as combinações: gerenciador sem BreachingCharge; gerenciador com BreachingCharge sem complemento; os três juntos. Nas duas primeiras, não deve haver referência ao tipo ausente; na terceira, testar o bloqueio de plantio dentro/fora do raid. Compilação e inicialização reais ainda não foram executadas neste ambiente.
