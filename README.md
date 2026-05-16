# XFCE4 GPU & CPU Monitor Plugins

Dois plugins para o painel XFCE4: monitor de GPU e monitor de CPU.

### GPU Monitor

Exibe uso da GPU (%), temperatura, e no tooltip: VRAM usada/total, clocks do chip e VRAM.

Compatível com **AMDGPU** (sysfs) e **NVIDIA** (nvidia-smi).

### CPU Monitor

Exibe uso da CPU (%) e temperatura (package), com tooltip detalhado.

Compatível com sensores **coretemp**, **k10temp** e **zenpower**.

---

## Dependências

| Pacote (Debian/Ubuntu) | Finalidade |
|---|---|
| `build-essential` | gcc, make |
| `autoconf automake libtool` | autotools |
| `xfce4-dev-tools` | macros XDT |
| `libxfce4panel-2.0-dev` | headers do painel XFCE4 |
| `libxfce4util-dev` | libxfce4util |
| `libxfce4ui-2-dev` | libxfce4ui |
| `libgtk-3-dev` | GTK3 |

Instale tudo de uma vez:

```bash
sudo apt install -y build-essential autoconf automake libtool \
  xfce4-dev-tools libxfce4panel-2.0-dev libxfce4util-dev \
  libxfce4ui-2-dev libgtk-3-dev
```

---

## Instalação

### GPU Plugin

```bash
cd xfce4-gpu-plugin
./autogen.sh
./configure --prefix=/usr --disable-dependency-tracking
make -j$(nproc)
sudo make install
```

### CPU Plugin

```bash
cd xfce4-cpu-plugin
./autogen.sh
./configure --prefix=/usr --disable-dependency-tracking
make -j$(nproc)
sudo make install
```

### Pós-instalação

Reinicie o painel para carregar os plugins:

```bash
xfce4-panel -r
```

Depois clique com botão direito no painel → **Adicionar Novos Itens** → procure por **GPU Monitor** e **CPU Monitor**.

---

## Desinstalação

```bash
sudo rm /usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/libxfce4-gpu-plugin.*
sudo rm /usr/share/xfce4/panel/plugins/xfce4-gpu-plugin.desktop
sudo rm /usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/libxfce4-cpu-plugin.*
sudo rm /usr/share/xfce4/panel/plugins/xfce4-cpu-plugin.desktop
xfce4-panel -r
```
