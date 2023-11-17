FROM mcr.microsoft.com/devcontainers/rust:latest

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -qq install -y \
    htop \
    cmake \
    ninja-build \
    neovim \
    tmux \
    # for tmux-cpu
    sysstat \
    # for coc.nvim
    nodejs npm \
    # for ctrlp
    silversearcher-ag \
    >/dev/null

USER vscode
RUN whoami
WORKDIR /home/vscode

RUN git clone https://github.com/mo-xiaoming/ubuntu-setup.git && \
    mkdir -p ~/.config/tmux/plugins && \
    git clone https://github.com/tmux-plugins/tpm ~/.config/tmux/plugins/tpm && \
    cp $PWD/ubuntu-setup/_tmux.conf ~/.config/tmux/tmux.conf && \
    mkdir -p ~/.config/Code/User && \
    cp $PWD/ubuntu-setup/code_settings.json ~/.config/Code/User/settings.json && \
    mkdir -p ~/.config/nvim/syntax && \
    cp $PWD/ubuntu-setup/vim-files/hobbes.vim ~/.config/nvim/syntax/hobbes.vim && \
    cp $PWD/ubuntu-setup/vim-files/init.vim ~/.config/nvim/init.vim && \
    rm -rf ubuntu-setup && \
    curl -fLo "${XDG_DATA_HOME:-$HOME/.local/share}"/nvim/site/autoload/plug.vim --create-dirs https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim && \
    nvim --headless +PlugInstall +qa && \
    nvim +UpdateRemotePlugins +qa && \
    nvim --headless +'CocInstall coc-json coc-spell-checker coc-clangd coc-explorer coc-rust-analyzer' +qa && \
    nvim --headless a.cpp +'CocCommand clangd.install' +qa

RUN python3 <<EOF
import os

with open(os.path.expanduser('~/.bashrc'), 'a') as f:
    f.write('\n')
    f.write('alias vi="nvim"\n')
    f.write('alias vim="nvim"\n')
    f.write('alias vimdiff="nvim -d"\n')
    f.write('alias view="nvim -R"\n')
    f.write('\n')
    f.write('export EDITOR=nvim\n')

EOF

RUN cargo install \
    cargo-watch \
    cargo-nextest \
    # show overview of a crate's modules
    cargo-modules \
    # create security vulnerabilities report
    cargo-audit \
    # code coverage
    cargo-tarpaulin \
    # macro expansion
    cargo-expand
