project('vaxp-os-builder', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++20',
    'optimization=2',
    'b_ndebug=if-release'
  ]
)

cpp = meson.get_compiler('cpp')

# Required commercial-grade libraries
threads_dep = dependency('threads')
openssl_dep = dependency('openssl', required : true)
json_dep = dependency('nlohmann_json', required : true)
filesystem_dep = cpp.find_library('stdc++fs', required : false)

fs = import('fs')

# Verify GRUB target platform boot assets (grub-pc-bin & grub-efi-amd64)
assert(fs.is_file('/usr/lib/grub/i386-pc/cdboot.img'), 'Missing /usr/lib/grub/i386-pc/cdboot.img! Please install grub-pc-bin (sudo apt install -y grub-pc-bin)')
assert(fs.is_dir('/usr/lib/grub/x86_64-efi'), 'Missing /usr/lib/grub/x86_64-efi! Please install grub-efi-amd64 (sudo apt install -y grub-efi-amd64)')

# Host system runtime utilities verification
host_tools = [
  'debootstrap',
  'xorriso',
  'mksquashfs',
  'unsquashfs',
  'grub-mkstandalone',
  'grub-install',
  'mkfs.vfat',
  'mcopy',
  'dd'
]

search_dirs = ['/usr/bin', '/usr/sbin', '/bin', '/sbin', '/usr/local/bin', '/usr/local/sbin']

foreach tool : host_tools
  find_program(tool, dirs : search_dirs, required : true)
endforeach

inc_dirs = include_directories('include')

src_files = files(
  # Core
  'src/core/Logger.cpp',
  'src/core/Process.cpp',
  'src/core/SignalHandler.cpp',
  'src/core/Config.cpp',
  
  # Filesystem & Storage
  'src/fs/FileOps.cpp',
  'src/fs/MountManager.cpp',
  'src/fs/SquashFs.cpp',
  
  # Engine
  'src/engine/BootstrapEngine.cpp',
  'src/engine/StepPipeline.cpp',
  'src/engine/IsoGenerator.cpp',
  
  # Modular Steps (All 47 modules mapped 1:1)
  'src/steps/Steps00to08.cpp',
  'src/steps/Steps10to20.cpp',
  'src/steps/Steps21to39.cpp',
  'src/steps/Steps40to88.cpp',

  # Main Entry
  'src/main.cpp'
)

executable('vaxp-builder',
  src_files,
  include_directories : inc_dirs,
  dependencies : [threads_dep, openssl_dep, json_dep, filesystem_dep],
  install : true
)
