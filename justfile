build-dir := "build-debug"

configure:
    meson setup {{build-dir}} --buildtype=debug

build: configure
    meson compile -C {{build-dir}}

release:
    meson setup build-release --buildtype=release
    meson compile -C build-release

debug: build
    ./{{build-dir}}/noctalia-greeter --debug

run: build
    sudo ./{{build-dir}}/noctalia-greeter

check: build
    ./{{build-dir}}/noctalia-greeter --help

test: build
    meson test -C {{build-dir}}

format:
    clang-format -i $(find src tests -type f \( -name '*.cpp' -o -name '*.h' \))

clean:
    meson compile -C {{build-dir}} --clean
