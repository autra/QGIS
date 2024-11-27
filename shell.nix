{ pkgs ? import <nixpkgs> { } }:
let
  py = pkgs.python311.override {
    self = py;
    packageOverrides = self: super: {
      pyqt5 = super.pyqt5.override {
        withLocation = true;
        withSerialPort = true;
      };
    };
  };

  pythonBuildInputs = with py.pkgs; [
    chardet
    gdal
    jinja2
    numpy
    owslib
    psycopg2
    pygments
    pyqt5
    pyqt-builder
    python-dateutil
    pytz
    pyyaml
    qscintilla-qt5
    requests
    setuptools
    sip
    six
    urllib3
  ];
  qtbase = pkgs.libsForQt5.qtbase;
  in
pkgs.mkShell rec {
  name = "qgis shell";

  buildInputs = with pkgs; with libsForQt5; [
    draco
    exiv2
    fcgi
    geos
    gsl
    hdf5
    libspatialindex
    libspatialite
    libzip
    netcdf
    openssl
    pdal
    postgresql
    proj
    protobuf
    qca-qt5
    qscintilla
    qt3d
    qtbase
    qtkeychain
    qtlocation
    qtmultimedia
    qtsensors
    qtserialport
    qtxmlpatterns
    qwt
    sqlite
    txt2tags
    zstd
    grass
    # qtwebkit # currently marked as insecure
    pythonBuildInputs
  ];

  nativeBuildInputs = with pkgs; with libsForQt5; [
    makeWrapper
    wrapGAppsHook3
    wrapQtAppsHook

    bison
    cmake
    flex
    ninja
  ];


  shellHook = ''
    set -h #remove "bash: hash: hashing disabled" warning !
    # https://nixos.org/manual/nixpkgs/stable/#python-setup.py-bdist_wheel-cannot-create-.whl
    SOURCE_DATE_EPOCH=$(date +%s)
    QT_PLUGIN_PATH=${qtbase}/${qtbase.qtPluginPrefix}
    QT_QPA_PLATFORM_PLUGIN_PATH="$QT_PLUGIN_PATH/platforms"
    # Let's allow compiling stuff
    # TODO
    # export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath (with pkgs; [ ])}":LD_LIBRARY_PATH;
  '';

# TODO build with grass, and webkit
#   cmakeFlags = [
#     "-DWITH_3D=True"
#     "-DWITH_PDAL=True"
#     "-DENABLE_TESTS=False"
#     "-DQT_PLUGINS_DIR=${qtbase}/${qtbase.qtPluginPrefix}"
#   ] ++ lib.optional (!withWebKit) "-DWITH_QTWEBKIT=OFF"
#     ++ lib.optional withGrass (let
#         gmajor = lib.versions.major grass.version;
#         gminor = lib.versions.minor grass.version;
#       in "-DGRASS_PREFIX${gmajor}=${grass}/grass${gmajor}${gminor}"
#     );

}

# { lib
# , fetchFromGitHub
# , makeWrapper
# , mkDerivation
# , substituteAll
# , wrapGAppsHook3
# , wrapQtAppsHook

# , withGrass
# , withWebKit

# , bison
# , cmake
# , draco
# , exiv2
# , fcgi
# , flex
# , geos
# , grass
# , gsl
# , hdf5
# , libspatialindex
# , libspatialite
# , libzip
# , netcdf
# , ninja
# , openssl
# , pdal
# , postgresql
# , proj
# , protobuf
# , python311
# , qca-qt5
# , qscintilla
# , qt3d
# , qtbase
# , qtkeychain
# , qtlocation
# , qtmultimedia
# , qtsensors
# , qtserialport
# , qtwebkit
# , qtxmlpatterns
# , qwt
# , sqlite
# , txt2tags
# , zstd
# }:

# in mkDerivation rec {

#   patches = [
#     (substituteAll {
#       src = ./set-pyqt-package-dirs.patch;
#       pyQt5PackageDir = "${py.pkgs.pyqt5}/${py.pkgs.python.sitePackages}";
#       qsciPackageDir = "${py.pkgs.qscintilla-qt5}/${py.pkgs.python.sitePackages}";
#     })
#   ];

#   qtWrapperArgs = [
#     "--set QT_QPA_PLATFORM_PLUGIN_PATH ${qtbase}/${qtbase.qtPluginPrefix}/platforms"
#   ];

#   dontWrapGApps = true; # wrapper params passed below

#   postFixup = lib.optionalString withGrass ''
#     # GRASS has to be availble on the command line even though we baked in
#     # the path at build time using GRASS_PREFIX.
#     # Using wrapGAppsHook also prevents file dialogs from crashing the program
#     # on non-NixOS.
#     for program in $out/bin/*; do
#       wrapProgram $program \
#         "''${gappsWrapperArgs[@]}" \
#         --prefix PATH : ${lib.makeBinPath [ grass ]}
#     done
#   '';
# }
