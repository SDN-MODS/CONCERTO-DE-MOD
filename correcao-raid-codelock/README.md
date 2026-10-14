# Correção do controle de raid — CodeLock

## Conteúdo salvo

- `original/SDN_RaidManager.zip`: arquivo enviado, preservado sem alterações.
- `SDN_RaidManager/`: fontes extraídas, com a preparação descrita abaixo. Os backups `.rar` continuam dentro do ZIP original.

## Diagnóstico

O arquivo original `LockDamageBlocker.c` modifica apenas `ActionDestroyCombinationLock`, a ação da fechadura numérica padrão do DayZ. O bloco que menciona `ActionDestroyCodeLock` estava inteiramente comentado. Por isso, ele não bloqueia a ação “Destroy Codelock” da imagem.

O ZIP não inclui as fontes do CodeLock que define essa ação. Não é seguro simplesmente descomentar o bloco: o nome da classe e os métodos de aplicação do dano precisam corresponder à versão instalada. Uma classe inexistente impede o carregamento dos scripts.

## Preparação implementada

- Método compartilhado `CanDestroyRaidLock()` no gerenciador existente: no servidor consulta o horário/configuração; no cliente usa o estado recebido pelo RPC existente. Antes da sincronização, o cliente mantém o bloqueio.
- A ação da fechadura numérica padrão usa essa regra para sua visibilidade.
- A mesma ação revalida a permissão em `OnFinishProgressServer`, antes de executar o efeito, inclusive se o horário de raid terminar durante a tentativa.
- Horários, configuração JSON, proteção de construções e notificações existentes foram preservados.

## Pendente para corrigir a ação da imagem

Fornecer o link/versão do CodeLock e, se possível, os scripts que definem a ação “Destroy Codelock”, incluindo seu callback e o código que aplica dano/remove a fechadura. A integração precisa bloquear a condição exibida no cliente e o ponto real de destruição no servidor. Se o CodeLock aplica dano a cada ciclo, cada ciclo deve ser protegido.

Não há integração ativa com uma classe presumida do CodeLock nesta entrega. Esta versão é uma preparação parcial, não uma correção validada da ação da imagem.

## Instalação e validação após concluir a integração

1. Empacotar `SDN_RaidManager/` com prefixo de PBO `SDN_RaidManager`, preservando os caminhos de `config.cpp`.
2. Carregar o mod nos clientes e no servidor por `-mod`, pois ocultar a ação exige o script no cliente. Carregar apenas por `-serverMod` não atualiza a interface do jogador.
3. Conferir a ordem/dependência de carregamento conforme o CodeLock instalado. Reiniciar clientes e servidor com o mesmo PBO atualizado.
4. Com raid desativado, mirar na fechadura com a serra: a ação deve estar ausente e a destruição deve ser recusada no servidor.
5. Ativar o raid e verificar que a ação aparece e funciona nas condições originalmente permitidas.
6. Desativar o raid durante a destruição e verificar que nenhum efeito posterior ao encerramento é aplicado.
7. Testar conexão/reconexão com raid ativo e inativo e mudanças de estado com jogadores conectados. A atualização visual depende do intervalo de recarga configurado e da entrega do RPC; a autorização no servidor deve ser reavaliada no momento do efeito.
8. Verificar que abrir a porta e gerenciar a fechadura continuam respeitando as permissões existentes.

## Verificações neste ambiente

Original preservado e comparado byte a byte com o upload; comparação das fontes limita as mudanças ao gerenciador e ao bloqueio da fechadura padrão. Não há compilador Enforce Script, DayZ Server ou CodeLock neste ambiente: compilação e comportamento no jogo não foram testados.
