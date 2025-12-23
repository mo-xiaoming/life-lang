{
  description = "life-lang C++ compiler development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils/v1.0.0";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # Get the unwrapped GCC 15 package to access gcov
        # pkgs.gcc15 is the wrapper, .cc gets the unwrapped package
        gcc-unwrapped = pkgs.gcc15.cc;

        # Custom script to run clang-tidy
        run-tidy = pkgs.writeShellScriptBin "run-tidy" ''
          run-clang-tidy -p=. -warnings-as-errors=* -quiet -extra-arg=-Wno-unknown-warning-option -j $(nproc)
        '';

        # Custom script to generate coverage
        generate-coverage = pkgs.writeShellScriptBin "generate-coverage" ''
          ${pkgs.lcov}/bin/lcov --gcov-tool ${gcc-unwrapped}/bin/gcov --capture --directory . --output-file coverage.info -rc geninfo_unexecuted_blocks=1 --ignore-errors mismatch --demangle-cpp
          ${pkgs.lcov}/bin/lcov --remove coverage.info --ignore-errors unused "/usr/*" "/nix/store/*" "*/tests/*" "*/build/*" "*/third_party/*" --output-file coverage.info
          ${pkgs.lcov}/bin/lcov --list coverage.info
        '';

        # Custom script to show versions
        show-versions = pkgs.writeShellScriptBin "show-versions" ''
          set -e  # Exit on error

          echo '================================================'
          echo 'Tool Versions and Paths:'
          echo '================================================'
          echo "  GCC:       $(gcc --version | head -n1)"
          echo "    Path:    $(which gcc)"
          echo "  G++:       $(g++ --version | head -n1)"
          echo "    Path:    $(which g++)"
          echo "  gcov:      $(gcov --version | head -n1)"
          echo "    Path:    $(which gcov)"
          echo "  Clang:     $(clang --version | head -n1)"
          echo "    Path:    $(which clang)"
          echo "  CMake:     $(cmake --version | head -n1)"
          echo "    Path:    $(which cmake)"
          echo "  Ninja:     $(ninja --version)"
          echo "    Path:    $(which ninja)"
          echo "  lcov:      $(lcov --version | head -n1)"
          echo "    Path:    $(which lcov)"
          echo '================================================'
          echo 'Environment Variables:'
          echo "  CC:        $CC"
          echo "  CXX:       $CXX"
          echo '================================================'

          # Verify consistency: environment variables must match PATH
          echo 'Consistency Checks:'

          GCC_PATH=$(which gcc)
          if [ "$CC" != "$GCC_PATH" ]; then
            echo "  ❌ FAIL: \$CC ($CC) != \$(which gcc) ($GCC_PATH)"
            exit 1
          else
            echo "  ✓ \$CC matches \$(which gcc)"
          fi

          GXX_PATH=$(which g++)
          if [ "$CXX" != "$GXX_PATH" ]; then
            echo "  ❌ FAIL: \$CXX ($CXX) != \$(which g++) ($GXX_PATH)"
            exit 1
          else
            echo "  ✓ \$CXX matches \$(which g++)"
          fi

          echo '================================================'
        '';

      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            pkgs.gcc15  # GCC 15.2.0 (provides gcc, g++)
            gcc-unwrapped  # Provides gcov from same GCC 15 version
            pkgs.clang_21
            pkgs.cmake
            pkgs.ninja
            pkgs.lcov
            pkgs.clang-tools  # provides clang-tidy, run-clang-tidy (matches clang_21 version)
            pkgs.git

            # Custom scripts
            run-tidy
            generate-coverage
            show-versions
          ];

          shellHook = ''
            echo 'life-lang development environment ready'

            # Ensure GCC 15 is first in PATH (override any stdenv gcc)
            export PATH="${pkgs.gcc15}/bin:${gcc-unwrapped}/bin:$PATH"

            # Set compiler environment variables to use GCC 15
            export CC="${pkgs.gcc15}/bin/gcc"
            export CXX="${pkgs.gcc15}/bin/g++"
          '';
        };
      }
    );
}
