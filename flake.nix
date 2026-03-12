{
  description = "infix-parser development environment";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:nixos/nixpkgs/master";
    foolnotion.url = "github:foolnotion/nur-pkg";
    foolnotion.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    inputs@{
      self,
      flake-parts,
      foolnotion,
      nixpkgs,
    }:
    flake-parts.lib.mkFlake { inherit inputs; } rec {
      systems = [
        "x86_64-linux"
        "x86_64-darwin"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      perSystem =
        { pkgs, system, ... }:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [ foolnotion.overlay ];
          };
          stdenv_ = pkgs.llvmPackages_latest.stdenv;

          parser = stdenv_.mkDerivation {
            pname = "infix-parser";
            version = "0.1.0";
            src = self;

            cmakeFlags = [
              "-DBUILD_SHARED_LIBS=${if pkgs.stdenv.hostPlatform.isStatic then "OFF" else "ON"}"
              "-DBUILD_TESTING=OFF"
              "-DCMAKE_BUILD_TYPE=Release"
            ];

            nativeBuildInputs = with pkgs; [
              cmake
              git
            ];

            buildInputs = with pkgs; [
              fast-float
              fmt
              lexy
            ];
          };

          enableTesting = true;
        in
        rec {
          packages = {
            default = parser.overrideAttrs (old: {
              cmakeFlags = old.cmakeFlags ++ [
                "--preset ${
                  if pkgs.stdenv.isLinux then
                    "release-linux"
                  else if pkgs.stdenv.isDarwin then
                    "release-darwin"
                  else
                    ""
                }"
              ];
            });

            compat = parser.overrideAttrs (_: {
              cmakeFlags = [
                "-DBUILD_SHARED_LIBS=OFF"
                "-DBUILD_TESTING=OFF"
                "-DCMAKE_BUILD_TYPE=Release"
                "-DCMAKE_CXX_STANDARD=20"
                "-DCMAKE_CXX_EXTENSIONS=OFF"
              ];
            });
          };

          devShells.default = stdenv_.mkDerivation {
            name = "infix-parser";

            nativeBuildInputs = parser.nativeBuildInputs ++ (with pkgs; [ clang-tools ]);

            buildInputs =
              parser.buildInputs
              ++ (
                with pkgs;
                pkgs.lib.optionals pkgs.stdenv.isLinux [
                  gcc15
                  gdb
                  hotspot
                  perf
                  valgrind
                ]
              )
              ++ (
                with pkgs;
                pkgs.lib.optionals enableTesting [
                  catch2_3
                  nanobench
                ]
              );
          };
        };
    };
}
