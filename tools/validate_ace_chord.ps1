$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$keymap = Get-Content (Join-Path $root 'boards/shields/ace/ace.keymap') -Raw
$cmake = Get-Content (Join-Path $root 'CMakeLists.txt') -Raw
$kconfig = Get-Content (Join-Path $root 'Kconfig') -Raw
$resolver = Get-Content (Join-Path $root 'src/behavior_ace_chord.c') -Raw

$requiredFiles = @(
    'src/behavior_ace_chord.c',
    'dts/bindings/behaviors/zmk,behavior-ace-chord.yaml'
)

foreach ($file in $requiredFiles) {
    if (-not (Test-Path (Join-Path $root $file))) {
        throw "missing $file"
    }
}

foreach ($needle in @(
    'CONFIG_ZMK_BEHAVIOR_ACE_CHORD',
    'src/behavior_ace_chord.c'
)) {
    if (-not ($cmake.Contains($needle) -or $kconfig.Contains($needle))) {
        throw "missing build hook $needle"
    }
}

foreach ($needle in @(
    '#define CHORD_TIMEOUT_TYPING 33',
    '#define COMBO_TIMEOUT_ACCESS 66',
    'achord: ace_chord',
    '&achord AC_BASE 3',
    '&achord AC_NUM 3',
    '&achord AC_SYM 3',
    '&achord AC_FUNC 3'
)) {
    if (-not $keymap.Contains($needle)) {
        throw "missing keymap resolver marker $needle"
    }
}

foreach ($forbidden in @(
    'TCOMBO_SIDE(alpha_',
    'TCOMBO_SIDE(num_',
    'TCOMBO_SIDE(sym_',
    'TCOMBO_SIDE(func_'
)) {
    if ($keymap.Contains($forbidden)) {
        throw "stale ZMK chord combo remains $forbidden"
    }
}

foreach ($forbidden in @(
    '#include <dt-bindings/zmk/keys.h>',
    '{BIT(4) | BIT(3), Q}'
)) {
    if ($resolver.Contains($forbidden)) {
        throw "resolver contains forbidden marker $forbidden"
    }
}

if ($resolver -match '(?<!ACE_)LS\(') {
    throw 'resolver contains raw LS() macro'
}

'ace chord validation ok'
