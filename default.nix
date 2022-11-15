{pkgs ? (import <nixpkgs> {})}:
pkgs.stdenv.mkDerivation rec {
  name = "xkxprint-kle";
  src = ./.;
  
  nativeBuildInputs = with pkgs; [
    pkg-config
    autoconf
    autogen
    automake
    libxkbcommon
    xorg-autoconf
    xorg.xkbprint
    xorg.xkbcomp
    xorg.libxkbfile
    xorg.libX11
    xorg.xorgproto
    json_c
  ];

  installPhase = ''
    mkdir -p $out/bin
    ./autogen.sh
    make
    cp xkbprint-kle $out/bin/
  '';
}
