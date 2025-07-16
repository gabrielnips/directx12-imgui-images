# imgui-images

Este projeto é um exemplo de aplicação em C++ para Windows que utiliza DirectX 12 e ImGui para exibir imagens na interface gráfica. O usuário pode carregar imagens de arquivos e visualizá-las em tempo real usando a interface do ImGui.

## Funcionalidades

- Renderização de interface gráfica com ImGui usando DirectX 12.
- Carregamento e exibição de imagens (usando stb_image).
- Interface simples para selecionar e visualizar imagens.
- Exemplo de integração entre ImGui, DirectX 12 e carregamento de texturas.

## Estrutura

- `src/` - Código-fonte principal.
- `include/` - Headers do projeto.
- `thirdparty/include/imgui` - Biblioteca ImGui e backends (DX12, Win32).
- `thirdparty/include/stb` - Biblioteca stb_image para leitura de imagens.

## Dependências

- [ImGui](https://github.com/ocornut/imgui)
- [stb_image](https://github.com/nothings/stb)
- DirectX 12 SDK

## Licença

MIT. Veja o arquivo LICENSE para mais detalhes.
