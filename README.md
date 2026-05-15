# noctalia-greeter

Standalone Noctalia greeter prototype for Linux. It uses Meson, C++20, EGL/GLES2, embedded shaders, Cairo/Pango text, and Tabler glyphs.

## Build

```sh
meson setup build-debug --buildtype=debug
meson compile -C build-debug
```

Run inside an existing graphical session:

```sh
./build-debug/noctalia-greeter --debug
```

`--debug` skips VT switching and real session handoff so the greeter can be developed from inside a running desktop.

## Void Linux Setup

This project is not ready to replace a production display manager yet: the debug Wayland backend is usable, while the DRM/KMS backend is still being wired up. When the real backend is ready, the Void/runit shape should look like this.

Install the binary and PAM file:

```sh
sudo meson install -C build-release
```

Disable `greetd` if it is enabled through runit:

```sh
sudo sv down greetd
sudo rm -f /var/service/greetd
```

Create a runit service:

```sh
sudo mkdir -p /etc/sv/noctalia-greeter
sudo tee /etc/sv/noctalia-greeter/run >/dev/null <<'EOF'
#!/bin/sh
exec /usr/local/bin/noctalia-greeter
EOF
sudo chmod +x /etc/sv/noctalia-greeter/run
```

Enable it:

```sh
sudo ln -s /etc/sv/noctalia-greeter /var/service/noctalia-greeter
```

To roll back:

```sh
sudo sv down noctalia-greeter
sudo rm -f /var/service/noctalia-greeter
sudo ln -s /etc/sv/greetd /var/service/greetd
```

Keep another TTY logged in while testing a display manager. If the greeter fails, switch VTs and disable the runit service from there.
